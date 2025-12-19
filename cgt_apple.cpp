/*
    Console Graphic Tools on Apple macOS
    
    by gty (adapted for macOS by JYQ with Gemini)

    Learnt from Shen Jian at Tongji University
*/


#ifdef __APPLE__

#include "./cgt.h"

#include <iostream>
#include <vector>
#include <cstdio>      // For printf, sscanf, EOF
#include <unistd.h>    // For read, write, STDIN_FILENO, STDOUT_FILENO, usleep
#include <termios.h>   // For terminal settings
#include <csignal>     // For signal handling (Ctrl+C)
#include <sys/select.h> // For non-blocking read check
#include <cstring>     // For strchr

using namespace std;

// --- Globals for restoring terminal state ---

static struct termios original_termios;
static bool cgt_initialized = false;

// --- 添加一个静态缓冲区来处理部分读取
static char __cgt_partial_input_buffer[256] = {0};


// --- Internal Input Event Buffering ---

enum CgtEventType {
    CGT_EVENT_KEY,
    CGT_EVENT_MOUSE
};

struct CgtInputEvent {
    CgtEventType type;
    
    // For KEY
    char ch;
    
    // For MOUSE
    int x, y, button, event;
};

static vector<CgtInputEvent> inputBuffer;


// --- Signal Handler for clean exit ---

static void __cgt_signal_handler(int sig) {
    // Catch signals like SIGINT (Ctrl+C) and SIGTERM
    cgt_close();
    exit(sig); // Exit with the signal number
}


// --- API Implementation ---

void cgt_init() {
    if (cgt_initialized) {
        return;
    }

    // Get and store original terminal settings
    tcgetattr(STDIN_FILENO, &original_termios);

    // Set new terminal settings (cbreak mode)
    struct termios new_termios = original_termios;
    new_termios.c_lflag &= ~(ICANON | ECHO); // Disable canonical mode (line buffering) and echo
    new_termios.c_iflag &= ~(ICRNL); // Don't map carriage return ('\r') to newline ('\n')
    new_termios.c_cc[VMIN] = 0;  // Non-blocking read (return immediately)
    new_termios.c_cc[VTIME] = 0; // No read timeout
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);

    // Use ANSI escape codes to initialize
    printf("\033[?25l"); // Hide cursor
    // Enable SGR mouse reporting (easier to parse)
    // ?1000h: Enable basic press/release (standard)
    // ?1003h: Report on move (extends ?1000h)
    // ?1006h: Use SGR format
    printf("\033[?1000h\033[?1003h\033[?1006h"); 
    fflush(stdout);

    // Register signal handler to restore terminal on exit
    signal(SIGINT, __cgt_signal_handler);
    signal(SIGTERM, __cgt_signal_handler);

    cgt_initialized = true;
}


void cgt_close() {
    if (!cgt_initialized) {
        return;
    }

    // Use ANSI escape codes to clean up
    printf("\033[?1006l\033[?1003l\033[?1000l"); // Disable mouse reporting
    printf("\033[?25h"); // Show cursor
    cgt_reset_color(); // Reset colors

    // Restore original terminal settings
    tcsetattr(STDIN_FILENO, TCSANOW, &original_termios);

    // Restore default signal handlers
    signal(SIGINT, SIG_DFL);
    signal(SIGTERM, SIG_DFL);

    cgt_initialized = false;
}


void cgt_msleep(int milliseconds) {
    // usleep takes microseconds
    usleep(milliseconds * 1000);
}


// --- ANSI Color Helpers ---

static int cgt_color_to_ansi_fg(int cgt_color) {
    // cgt.h: 0:BLK, 1:BLU, 2:GRN, 3:CYA, 4:RED, 5:MAG, 6:YEL, 7:WHI
    // ansi:  30:BLK, 31:RED, 32:GRN, 33:YEL, 34:BLU, 35:MAG, 36:CYA, 37:WHI
    switch(cgt_color) {
        case COLOR_BLACK:   return 30;
        case COLOR_RED:     return 31;
        case COLOR_GREEN:   return 32;
        case COLOR_YELLOW:  return 33;
        case COLOR_BLUE:    return 34;
        case COLOR_MAGENTA: return 35;
        case COLOR_CYAN:    return 36;
        case COLOR_WHITE:   return 37;
        case COLOR_LIGHT_BLACK:   return 90;
        case COLOR_LIGHT_RED:     return 91;
        case COLOR_LIGHT_GREEN:   return 92;
        case COLOR_LIGHT_YELLOW:  return 93;
        case COLOR_LIGHT_BLUE:    return 94;
        case COLOR_LIGHT_MAGENTA: return 95;
        case COLOR_LIGHT_CYAN:    return 96;
        case COLOR_LIGHT_WHITE:   return 97;
        default: return 37; // default white
    }
}

static int cgt_color_to_ansi_bg(int cgt_color) {
    // Same as fg, but +10 for 0-7 and 8-15
    switch(cgt_color) {
        case COLOR_BLACK:   return 40;
        case COLOR_RED:     return 41;
        case COLOR_GREEN:   return 42;
        case COLOR_YELLOW:  return 43;
        case COLOR_BLUE:    return 44;
        case COLOR_MAGENTA: return 45;
        case COLOR_CYAN:    return 46;
        case COLOR_WHITE:   return 47;
        case COLOR_LIGHT_BLACK:   return 100;
        case COLOR_LIGHT_RED:     return 101;
        case COLOR_LIGHT_GREEN:   return 102;
        case COLOR_LIGHT_YELLOW:  return 103;
        case COLOR_LIGHT_BLUE:    return 104;
        case COLOR_LIGHT_MAGENTA: return 105;
        case COLOR_LIGHT_CYAN:    return 106;
        case COLOR_LIGHT_WHITE:   return 107;
        default: return 40; // default black
    }
}


void cgt_clear_screen(int color) {
    // Set background color
    printf("\033[%dm", cgt_color_to_ansi_bg(color));
    // Clear entire screen (fills with current background)
    printf("\033[2J");
    // Move cursor to home (0, 0)
    printf("\033[H");
    // Reset attributes so future text isn't affected
    printf("\033[0m");
    fflush(stdout);
}


// --- Internal Input Parsing ---

/**
 * Checks if there is data available to be read on STDIN.
 */
static bool __cgt_has_data_on_stdin() {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    
    // Use select to poll
    return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) == 1;
}

/**
 * Parses a given buffer and adds events to the internal inputBuffer.
 */
static int __cgt_parse_input(char* buf, int bytesRead) {
    int i = 0;
    while (i < bytesRead) {
        if (buf[i] == '\033') { // 潜在的转义序列
            
            if (i + 1 >= bytesRead) {
                return i; // 停止解析，保留 "\033"
            }
            
            if (buf[i+1] == '[') {
                // 是 "\033["
                
                // 检查部分 "\033["
                if (i + 2 >= bytesRead) {
                    return i; // 停止解析，保留 "\033["
                }

                if (buf[i+2] == '<') {
                    // SGR 鼠标事件: \033[<b;x;yM 或 \033[<b;x;ym
                    int b, x, y;
                    char type;

                    // 必须找到序列的 *末尾*
                    int j = i + 3;
                    while (j < bytesRead && buf[j] != 'M' && buf[j] != 'm') {
                        j++;
                    }

                    if (j == bytesRead) {
                        return i; // 部分SGR，没有找到 M/m
                    }

                    // 找到了一个完整的序列，尝试解析
                    type = buf[j];
                    if (sscanf(&buf[i+3], "%d;%d;%d%c", &b, &x, &y, &type) == 4 && (type == 'M' || type == 'm')) {
                        CgtInputEvent ev;
                        ev.type = CGT_EVENT_MOUSE;
                        ev.x = x - 1; // ANSI 是 1-indexed, cgt 是 0-indexed
                        ev.y = y - 1;

                        if (type == 'M') { // Button Press / Release / Drag
                            if (b >= 32 && b <= 34) { // Drag event
                                ev.event = MOUSE_MOVE; // cgt.h has no DRAG, so we map to MOVE
                                int button_code = b - 32;
                                if (button_code == 0) ev.button = MOUSE_BUTTON_LEFT;
                                else if (button_code == 1) ev.button = MOUSE_BUTTON_MIDDLE;
                                else if (button_code == 2) ev.button = MOUSE_BUTTON_RIGHT;
                                else ev.button = 0;
                            }
                            else if (b == 3) { // Release
                                ev.event = MOUSE_RELEASE;
                                ev.button = 0; // cgt API seems to only report button on press
                            }
                            else if (b >= 0 && b <= 2) { // Press
                                ev.event = MOUSE_CLICK;
                                int button_code = b;
                                if (button_code == 0)
                                    ev.button = MOUSE_BUTTON_LEFT;
                                else if (button_code == 1)
                                    ev.button = MOUSE_BUTTON_MIDDLE;
                                else if (button_code == 2)
                                    ev.button = MOUSE_BUTTON_RIGHT;
                                else
                                    ev.button = 0;
                            } else {
                                // 未知的 'M' 事件 (例如 b=35, 纯移动)
                                // 将其视为 MOUSE_MOVE
                                ev.event = MOUSE_MOVE;
                                ev.button = 0;
                            }
                        }
                        else if (type == 'm') { // Move (no button down) or Release
                            if (b >= 32) { // 移动 (例如 b=35)
                                ev.event = MOUSE_MOVE;
                                ev.button = 0;
                            } else { // 释放 (b=0, 1, 2)
                                ev.event = MOUSE_RELEASE;
                                ev.button = 0; // cgt API 不太关心释放的是哪个键
                            }
                        }

                        inputBuffer.push_back(ev);
                        
                        // 推进 'i' 越过这个已解析的序列
                        i = j + 1; // 越过 'M' 或 'm'
                        continue;
                    }
                    // 如果 sscanf 失败，这是一个未知的 SGR. 跳过它.
                    i = j + 1;
                    continue;
                }
                
                // 其他 ANSI (如箭头键)
                int j = i + 2;
                while (j < bytesRead && !( (buf[j] >= 'A' && buf[j] <= 'Z') || (buf[j] >= 'a' && buf[j] <= 'z') || buf[j] == '~' )) {
                    j++;
                }

                // 检查是否跑到了末尾 (部分序列)
                if (j == bytesRead) {
                    // 这是一个部分序列。
                    return i; // --- 停止解析 ---
                } else {
                    // 找到了终止符，跳过它
                    i = j + 1;
                    continue;
                }

            } else {
                // 不是 "\033[" (例如, "\033A"), 视为 ESC 键
                // 掉落到下面的 "Regular key press" 逻辑
            }
        } else {
            // 常规按键
            CgtInputEvent ev;
            ev.type = CGT_EVENT_KEY;
            ev.ch = buf[i];
            
            // 映射 '\r' 到 '\n'
            if (ev.ch == '\r') {
                ev.ch = '\n';
            }
            
            inputBuffer.push_back(ev);
            i++;
        }
    }
    return i; // 消耗完毕
}


/**
 * Reads all available data from STDIN and parses it into
 * the internal inputBuffer.
 */
static void __cgt_read_input_to_buffer() {
    // 仅当没有数据且缓冲区为空时才返回
    if (!__cgt_has_data_on_stdin() && __cgt_partial_input_buffer[0] == '\0') {
        return;
    }
    
    // 如果有数据，读取它
    if (__cgt_has_data_on_stdin()) {
        char readBuf[64];
        int bytesRead = read(STDIN_FILENO, readBuf, 63);
        if (bytesRead > 0) {
            readBuf[bytesRead] = '\0';
            // 附加到我们的部分缓冲区
            strcat(__cgt_partial_input_buffer, readBuf);
        }
    }

    int totalLen = strlen(__cgt_partial_input_buffer);
    if (totalLen == 0) {
        return; // 没有可解析的
    }

    int consumed = __cgt_parse_input(__cgt_partial_input_buffer, totalLen);

    if (consumed < totalLen) {
        // 保存未消耗的部分数据以备下次使用
        memmove(__cgt_partial_input_buffer, &__cgt_partial_input_buffer[consumed], totalLen - consumed);
        __cgt_partial_input_buffer[totalLen - consumed] = '\0';
    } else {
        // 全部消耗
        __cgt_partial_input_buffer[0] = '\0';
    }
}


bool cgt_has_mouse() {
    __cgt_read_input_to_buffer();
    for (const CgtInputEvent& ev : inputBuffer) {
        if (ev.type == CGT_EVENT_MOUSE) {
            return true;
        }
    }
    return false;
}


void cgt_get_mouse(int& x, int& y, int& button, int& event) {
    // Note: __cgt_read_input_to_buffer() is NOT called here.
    // User must call cgt_has_mouse() first.
    for (auto it = inputBuffer.begin(); it != inputBuffer.end(); ++it) {
        if (it->type == CGT_EVENT_MOUSE) {
            x = it->x;
            y = it->y;
            button = it->button;
            event = it->event;
            inputBuffer.erase(it); // Consume the event
            return;
        }
    }
}


bool cgt_has_key() {
    __cgt_read_input_to_buffer();
    for (const CgtInputEvent& ev : inputBuffer) {
        if (ev.type == CGT_EVENT_KEY) {
            return true;
        }
    }
    return false;
}


void cgt_get_key(char& ch) {
    // Note: __cgt_read_input_to_buffer() is NOT called here.
    // User must call cgt_has_key() first.
    for (auto it = inputBuffer.begin(); it != inputBuffer.end(); ++it) {
        if (it->type == CGT_EVENT_KEY) {
            ch = it->ch;
            inputBuffer.erase(it); // Consume the event
            return;
        }
    }
}


// --- Cursor and Color Control ---

void cgt_set_color(int foreground, int background) {
    printf("\033[%d;%dm", cgt_color_to_ansi_fg(foreground), cgt_color_to_ansi_bg(background));
    fflush(stdout); // Make color change immediate
}


void cgt_reset_color() {
    printf("\033[0m"); // Reset all attributes
    fflush(stdout);
}


void cgt_getxy(int &x, int &y) {
    // 发送 DSR 请求
    printf("\033[6n");
    fflush(stdout);

    char buf[256] = {0};
    const long BUF_THRESHOLD = sizeof(buf) - 1;
    
    int bytesRead = 0;
    char* r_pos = NULL;
    
    // --- 首先清空并使用我们的部分缓冲区 ---
    int partialLen = strlen(__cgt_partial_input_buffer);
    if (partialLen > 0) {
        // 假设缓冲区不会溢出

        if (partialLen < BUF_THRESHOLD) {
            strcpy(buf, __cgt_partial_input_buffer);
            bytesRead = partialLen;
        } else {
             // 不太可能，但为了安全
             strncpy(buf, __cgt_partial_input_buffer, BUF_THRESHOLD);
             buf[BUF_THRESHOLD] = '\0';
             bytesRead = BUF_THRESHOLD;
        }
        __cgt_partial_input_buffer[0] = '\0'; // 清空它
    }
    
    
    // 读回报: \033[<row>;<col>R
    // 轮询最多 100ms, 累积数据
    for (int timeout = 0; timeout < 100; timeout++) {
        if (__cgt_has_data_on_stdin()) {
            int n = read(STDIN_FILENO, &buf[bytesRead], BUF_THRESHOLD - bytesRead);
            if (n > 0) {
                bytesRead += n;
                buf[bytesRead] = '\0'; // Null-terminate
            }
        }
        
        // 检查我们是否有 *完整* 的回报
        // 我们查找 \033[...R 模式
        r_pos = strrchr(buf, 'R'); // 查找 *最后* 的 'R'
        if (r_pos != NULL) {
            char* esc_pos = r_pos;
            // 向后扫描以查找匹配的 \033[
            while (esc_pos > buf && *esc_pos != '[') {
                esc_pos--;
            }
            if (*esc_pos == '[' && esc_pos > buf && *(esc_pos - 1) == '\033') {
                 // 找到了一个潜在的 \033[...R
                 break; // 假设这是我们的回报
            }
        }
        
        if (bytesRead >= BUF_THRESHOLD)
            break; // 缓冲区满
        usleep(1000); // 1ms
    }

    // 现在, 解析 buf. 它可能包含鼠标事件 *和* 回报.
    x = 0; y = 0; // 默认
    bool foundReport = false;
    int reportStart = -1;
    int reportEnd = -1;

    // 我们在 r_pos 找到了候选的 'R'. 让我们 *向后* 解析
    if (r_pos != NULL) {
        char* start = r_pos - 1;
        int row = 0, col = 0;

        // 查找 ';'
        while (start > buf && *start != ';') start--;
        
        if (*start == ';') {
            // 找到 ';', 尝试从它后面解析 col
            if (sscanf(start + 1, "%d", &col) == 1) {
                // 现在查找 '['
                while (start > buf && *start != '[') start--;
                if (*start == '[' && start > buf && *(start-1) == '\033') {
                    // 找到 \033[, 尝试从它后面解析 row
                    if (sscanf(start + 1, "%d", &row) == 1) {
                        // 成功! 我们找到了 \033[<row>;<col>R
                        x = col - 1; // 1-indexed to 0-indexed
                        y = row - 1;
                        foundReport = true;
                        reportStart = (int)(start - 1 - buf); // \033 的位置
                        reportEnd = (int)(r_pos + 1 - buf); // R 之后的位置
                    }
                }
            }
        }
    }


    // 现在，解析报告 *周围* 的数据
    if (foundReport) {
        // --- 修复: 使用新的解析器并保存剩余部分 ---
        // 先解析报告 *之后* 的数据
        if (reportEnd < bytesRead) {
            int consumed = __cgt_parse_input(&buf[reportEnd], bytesRead - reportEnd);
            if (consumed < bytesRead - reportEnd) {
                // 保存剩余
                strcat(__cgt_partial_input_buffer, &buf[reportEnd + consumed]);
            }
        }
        // 解析报告 *之前* 的数据
        if (reportStart > 0) {
            int consumed = __cgt_parse_input(buf, reportStart);
             if (consumed < reportStart) {
                // 保存剩余
                strncat(__cgt_partial_input_buffer, &buf[consumed], reportStart - consumed);
             }
        }
    } else {
        // 没找到报告? 解析整个缓冲区.
        if (bytesRead > 0) {
            // --- 修复: 使用新的解析器并保存剩余部分 ---
            int consumed = __cgt_parse_input(buf, bytesRead);
            if (consumed < bytesRead) {
                // 保存剩余
                strcat(__cgt_partial_input_buffer, &buf[consumed]);
            }
        }
    }
}


void cgt_gotoxy(int x, int y) {
    int currentX, currentY;
    if (x == -1 || y == -1) {
        // Get current position if either coord is -1
        cgt_getxy(currentX, currentY);
    }
    
    int finalX = (x == -1) ? currentX : x;
    int finalY = (y == -1) ? currentY : y;

    // ANSI is 1-indexed (row, col)
    printf("\033[%d;%dH", finalY + 1, finalX + 1);
    fflush(stdout); // Make move immediate
}


// --- Print Functions ---

void cgt_print_str(const char* str, int x, int y, int foreground, int background) {
    cgt_set_color(foreground, background); // Flushes
    cgt_gotoxy(x, y); // Flushes
    cout << str;
    cout.flush(); // Flush content
}


void cgt_print_char(char ch, int x, int y, int foreground, int background) {
    cgt_set_color(foreground, background);
    cgt_gotoxy(x, y);
    cout << ch;
    cout.flush();
}


void cgt_print_int(int num, int x, int y, int foreground, int background) {
    cgt_set_color(foreground, background);
    cgt_gotoxy(x, y);
    cout << num;
    cout.flush();
}


void cgt_print_double(double num, int x, int y, int foreground, int background) {
    cgt_set_color(foreground, background);
    cgt_gotoxy(x, y);
    cout << num;
    cout.flush();
}


#endif // __APPLE__