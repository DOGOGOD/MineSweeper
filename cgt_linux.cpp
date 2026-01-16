/*
    Console Graphic Tools on Apple macOS
    
    by gty (adapted for macOS by GZQ with Gemini)

    Learnt from Shen Jian at Tongji University
*/

#ifdef __linux__

#include "./cgt.h"

#include <iostream>
#include <vector>
#include <cstdio>       // For printf, sscanf, EOF
#include <unistd.h>     // For read, write, STDIN_FILENO, STDOUT_FILENO, usleep
#include <termios.h>    // For terminal settings
#include <csignal>      // For signal handling (Ctrl+C)
#include <sys/select.h> // For non-blocking read check
#include <cstring>      // For strchr, memmove, etc.
#include <cstdlib>      // For exit

using namespace std;

// --- Globals for restoring terminal state ---

static struct termios original_termios;
static bool cgt_initialized = false;

// --- 静态缓冲区处理部分读取 (Partial Reads) ---
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

    // 1. 获取原始终端属性
    tcgetattr(STDIN_FILENO, &original_termios);

    // 2. 设置新属性 (进入 Raw/Cbreak 模式)
    struct termios new_termios = original_termios;
    new_termios.c_lflag &= ~(ICANON | ECHO); // 禁用规范模式(行缓冲) 和 回显
    new_termios.c_iflag &= ~(ICRNL);         // 禁用回车映射到换行 (\r -> \n)
    
    // 设置非阻塞读取
    new_termios.c_cc[VMIN] = 0;  
    new_termios.c_cc[VTIME] = 0; 
    
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);

    // 3. 使用 ANSI 转义序列初始化显示
    printf("\033[?25l"); // 隐藏光标
    
    // 启用鼠标报告 (SGR 模式, 现代 Linux 终端通用)
    // ?1000h: 基本点击
    // ?1003h: 移动报告
    // ?1006h: SGR 扩展格式 (避免坐标限制和乱码)
    printf("\033[?1000h\033[?1003h\033[?1006h"); 
    fflush(stdout);

    // 4. 注册信号处理，确保程序意外退出时恢复终端
    signal(SIGINT, __cgt_signal_handler);
    signal(SIGTERM, __cgt_signal_handler);

    cgt_initialized = true;
}


void cgt_close() {
    if (!cgt_initialized) {
        return;
    }

    // 禁用鼠标报告
    printf("\033[?1006l\033[?1003l\033[?1000l"); 
    // 显示光标
    printf("\033[?25h"); 
    // 重置颜色
    cgt_reset_color(); 

    // 恢复原始终端设置
    tcsetattr(STDIN_FILENO, TCSANOW, &original_termios);

    // 恢复默认信号处理
    signal(SIGINT, SIG_DFL);
    signal(SIGTERM, SIG_DFL);

    cgt_initialized = false;
}


void cgt_msleep(int milliseconds) {
    // Linux 下 usleep 单位也是微秒
    usleep(milliseconds * 1000);
}


// --- ANSI Color Helpers ---

static int cgt_color_to_ansi_fg(int cgt_color) {
    // 映射 cgt.h 定义的颜色到 ANSI 前景色
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
    // 映射 cgt.h 定义的颜色到 ANSI 背景色
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
    // 设置背景色
    printf("\033[%dm", cgt_color_to_ansi_bg(color));
    // 清屏 (2J) 并 归位 (H)
    printf("\033[2J\033[H");
    // 重置属性
    printf("\033[0m");
    fflush(stdout);
}


// --- Internal Input Parsing ---

/**
 * 检查 STDIN 是否有数据可读 (非阻塞)
 */
static bool __cgt_has_data_on_stdin() {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    
    return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) == 1;
}

/**
 * 解析输入缓冲区，处理 ANSI 序列和 SGR 鼠标事件
 */
static int __cgt_parse_input(char* buf, int bytesRead) {
    int i = 0;
    while (i < bytesRead) {
        if (buf[i] == '\033') { // ESC, 开始转义序列
            
            if (i + 1 >= bytesRead) return i; // 等待更多数据
            
            if (buf[i+1] == '[') {
                // CSI 序列 "\033["
                
                if (i + 2 >= bytesRead) return i;

                // --- SGR Mouse Event: \033[<b;x;yM ---
                if (buf[i+2] == '<') {
                    int j = i + 3;
                    // 寻找序列结尾的 'M' 或 'm'
                    while (j < bytesRead && buf[j] != 'M' && buf[j] != 'm') {
                        j++;
                    }

                    if (j == bytesRead) return i; // 序列不完整

                    int b, x, y;
                    char type = buf[j];
                    
                    if (sscanf(&buf[i+3], "%d;%d;%d%c", &b, &x, &y, &type) == 4) {
                        CgtInputEvent ev;
                        ev.type = CGT_EVENT_MOUSE;
                        ev.x = x - 1; // ANSI 1-indexed -> CGT 0-indexed
                        ev.y = y - 1;

                        if (type == 'M') { // 按下 或 拖动
                            if (b >= 32 && b <= 34) { // 拖动
                                ev.event = MOUSE_MOVE;
                                int button_code = b - 32;
                                if (button_code == 0) ev.button = MOUSE_BUTTON_LEFT;
                                else if (button_code == 1) ev.button = MOUSE_BUTTON_MIDDLE;
                                else if (button_code == 2) ev.button = MOUSE_BUTTON_RIGHT;
                                else ev.button = 0;
                            }
                            else if (b == 3) { // 某些终端报告的释放
                                ev.event = MOUSE_RELEASE;
                                ev.button = 0; 
                            }
                            else if (b >= 0 && b <= 2) { // 点击
                                ev.event = MOUSE_CLICK;
                                if (b == 0) ev.button = MOUSE_BUTTON_LEFT;
                                else if (b == 1) ev.button = MOUSE_BUTTON_MIDDLE;
                                else if (b == 2) ev.button = MOUSE_BUTTON_RIGHT;
                                else ev.button = 0;
                            } else {
                                // 滚轮或其他 (如 b=64/65) 暂时当作移动处理，或忽略
                                ev.event = MOUSE_MOVE;
                                ev.button = 0;
                            }
                        }
                        else if (type == 'm') { // 释放 或 纯移动
                            if (b >= 32) {
                                ev.event = MOUSE_MOVE;
                                ev.button = 0;
                            } else {
                                ev.event = MOUSE_RELEASE;
                                ev.button = 0;
                            }
                        }

                        inputBuffer.push_back(ev);
                        i = j + 1; // 消耗完该序列
                        continue;
                    }
                    // 解析失败，跳过该序列
                    i = j + 1;
                    continue;
                }
                
                // --- 其他 CSI 序列 (如方向键) ---
                // 简单的跳过逻辑：跳过直到遇到字母或波浪号
                int j = i + 2;
                while (j < bytesRead && !( (buf[j] >= 'A' && buf[j] <= 'Z') || (buf[j] >= 'a' && buf[j] <= 'z') || buf[j] == '~' )) {
                    j++;
                }

                if (j == bytesRead) return i; // 序列不完整

                // 消耗该序列 (暂不处理键盘特殊键，如方向键)
                i = j + 1;
                continue;

            } else {
                // "\033" 后面不是 '['，可能是 Alt+键 或 单独的 ESC
                // 这里简化处理，作为单独字符或忽略
            }
        } else {
            // 常规字符
            CgtInputEvent ev;
            ev.type = CGT_EVENT_KEY;
            ev.ch = buf[i];
            
            if (ev.ch == '\r') ev.ch = '\n';
            
            inputBuffer.push_back(ev);
            i++;
        }
    }
    return i; // 返回已消耗的字节数
}


/**
 * 从 STDIN 读取数据到内部缓冲区并解析
 */
static void __cgt_read_input_to_buffer() {
    // 只有当有新数据 或者 缓冲区里有残留数据时才处理
    if (!__cgt_has_data_on_stdin() && __cgt_partial_input_buffer[0] == '\0') {
        return;
    }
    
    if (__cgt_has_data_on_stdin()) {
        char readBuf[64];
        int bytesRead = read(STDIN_FILENO, readBuf, 63);
        if (bytesRead > 0) {
            readBuf[bytesRead] = '\0';
            strcat(__cgt_partial_input_buffer, readBuf);
        }
    }

    int totalLen = strlen(__cgt_partial_input_buffer);
    if (totalLen == 0) return;

    int consumed = __cgt_parse_input(__cgt_partial_input_buffer, totalLen);

    if (consumed < totalLen) {
        // 移动未消耗的数据到缓冲区头部
        memmove(__cgt_partial_input_buffer, &__cgt_partial_input_buffer[consumed], totalLen - consumed);
        __cgt_partial_input_buffer[totalLen - consumed] = '\0';
    } else {
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
    // 用户必须先调用 cgt_has_mouse() 填充缓冲区
    for (auto it = inputBuffer.begin(); it != inputBuffer.end(); ++it) {
        if (it->type == CGT_EVENT_MOUSE) {
            x = it->x;
            y = it->y;
            button = it->button;
            event = it->event;
            inputBuffer.erase(it); 
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
    for (auto it = inputBuffer.begin(); it != inputBuffer.end(); ++it) {
        if (it->type == CGT_EVENT_KEY) {
            ch = it->ch;
            inputBuffer.erase(it);
            return;
        }
    }
}


// --- Cursor and Color Control ---

void cgt_set_color(int foreground, int background) {
    printf("\033[%d;%dm", cgt_color_to_ansi_fg(foreground), cgt_color_to_ansi_bg(background));
    fflush(stdout); 
}


void cgt_reset_color() {
    printf("\033[0m");
    fflush(stdout);
}


void cgt_getxy(int &x, int &y) {
    // 请求光标位置报告 (Device Status Report)
    printf("\033[6n");
    fflush(stdout);

    char buf[256] = {0};
    const long BUF_THRESHOLD = sizeof(buf) - 1;
    int bytesRead = 0;
    
    // 1. 先把以前残留的数据放进来 (防止把 DSR 回报混在之前的读取中)
    int partialLen = strlen(__cgt_partial_input_buffer);
    if (partialLen > 0) {
        strncpy(buf, __cgt_partial_input_buffer, BUF_THRESHOLD);
        bytesRead = (partialLen < BUF_THRESHOLD) ? partialLen : BUF_THRESHOLD;
        __cgt_partial_input_buffer[0] = '\0';
    }
    
    // 2. 轮询读取，直到找到 'R' (Report 结尾)
    char* r_pos = NULL;
    for (int timeout = 0; timeout < 100; timeout++) {
        if (__cgt_has_data_on_stdin()) {
            int n = read(STDIN_FILENO, &buf[bytesRead], BUF_THRESHOLD - bytesRead);
            if (n > 0) {
                bytesRead += n;
                buf[bytesRead] = '\0';
            }
        }
        
        // 寻找 \033[...R 模式
        r_pos = strrchr(buf, 'R');
        if (r_pos != NULL) {
            break; 
        }
        
        if (bytesRead >= BUF_THRESHOLD) break;
        usleep(1000); 
    }

    x = 0; y = 0;
    bool foundReport = false;
    int reportStart = -1;
    int reportEnd = -1;

    // 3. 解析位置
    if (r_pos != NULL) {
        // 反向寻找 ESC [
        char* start = r_pos - 1;
        int row = 0, col = 0;

        while (start > buf && *start != ';') start--; // 找分号
        
        if (*start == ';') {
            if (sscanf(start + 1, "%d", &col) == 1) {
                while (start > buf && *start != '[') start--; // 找左括号
                if (*start == '[' && start > buf && *(start-1) == '\033') {
                    if (sscanf(start + 1, "%d", &row) == 1) {
                        x = col - 1; 
                        y = row - 1;
                        foundReport = true;
                        reportStart = (int)(start - 1 - buf);
                        reportEnd = (int)(r_pos + 1 - buf);
                    }
                }
            }
        }
    }

    // 4. 将报告之外的数据（如果是鼠标事件等）放回缓冲区
    if (foundReport) {
        if (reportEnd < bytesRead) {
            int consumed = __cgt_parse_input(&buf[reportEnd], bytesRead - reportEnd);
            if (consumed < bytesRead - reportEnd) {
                strcat(__cgt_partial_input_buffer, &buf[reportEnd + consumed]);
            }
        }
        if (reportStart > 0) {
            int consumed = __cgt_parse_input(buf, reportStart);
             if (consumed < reportStart) {
                strncat(__cgt_partial_input_buffer, &buf[consumed], reportStart - consumed);
             }
        }
    } else {
        if (bytesRead > 0) {
            int consumed = __cgt_parse_input(buf, bytesRead);
            if (consumed < bytesRead) {
                strcat(__cgt_partial_input_buffer, &buf[consumed]);
            }
        }
    }
}


void cgt_gotoxy(int x, int y) {
    int currentX, currentY;
    if (x == -1 || y == -1) {
        cgt_getxy(currentX, currentY);
    }
    
    int finalX = (x == -1) ? currentX : x;
    int finalY = (y == -1) ? currentY : y;

    // ANSI 设置光标: \033[row;colH
    printf("\033[%d;%dH", finalY + 1, finalX + 1);
    fflush(stdout);
}


// --- Print Functions ---

void cgt_print_str(const char* str, int x, int y, int foreground, int background) {
    cgt_set_color(foreground, background);
    cgt_gotoxy(x, y);
    cout << str;
    cout.flush();
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

#endif // __linux__