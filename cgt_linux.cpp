/*
    Console Graphic Tools on Apple macOS
    
    by gty (adapted for macOS by GZQ with Gemini)

    Learnt from Shen Jian at Tongji University
*/

/*
    Console Graphic Tools on Linux (High Performance + Double Click Fix)
    
    Optimized by Gemini
    Features:
    1. State Caching: Eliminates redundant ANSI codes.
    2. Zero-Copy Parsing: Fast manual pointer arithmetic.
    3. Output Buffering: Minimizes syscalls.
    4. Synthetic Double-Click: Simulates Windows-like double click events.
*/

#ifdef __linux__

#include "./cgt.h"

#include <vector>
#include <cstdio>       
#include <unistd.h>     // read, write
#include <termios.h>    
#include <csignal>      
#include <cstring>      
#include <cstdlib>      
#include <sys/time.h>   // For gettimeofday (Double click detection)

using namespace std;

// --- Performance Constants ---
#define INPUT_BUF_SIZE 4096
#define OUTPUT_BUF_SIZE 65536 
#define DOUBLE_CLICK_THRESHOLD_MS 400 // 双击判定阈值 (毫秒)

// --- Globals ---

static struct termios original_termios;
static bool cgt_initialized = false;

// --- State Caching ---
static int _g_cur_fg = -1;
static int _g_cur_bg = -1;
static int _g_cur_x  = -1;
static int _g_cur_y  = -1;

// --- Double Click State ---
static long long _g_last_click_time = 0;
static int _g_last_click_x = -1;
static int _g_last_click_y = -1;
static int _g_last_click_btn = -1;

// --- Input Buffer ---
static char __cgt_partial_input_buffer[INPUT_BUF_SIZE] = {0};
static int  __cgt_buffer_len = 0;

enum CgtEventType {
    CGT_EVENT_KEY,
    CGT_EVENT_MOUSE
};

struct CgtInputEvent {
    CgtEventType type;
    char ch;
    int x, y, button, event;
};

static vector<CgtInputEvent> inputBuffer;


// --- Helper: Get Current Time in ms ---
static long long _cgt_get_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

// --- Helper: Fast Integer Parsing ---
static int fast_parse_int(const char*& p) {
    int val = 0;
    while (*p >= '0' && *p <= '9') {
        val = val * 10 + (*p - '0');
        p++;
    }
    return val;
}

// --- Signal Handler ---
static void __cgt_signal_handler(int sig) {
    cgt_close();
    exit(sig); 
}

// --- ANSI Color Logic ---
static int cgt_color_to_ansi_fg(int cgt_color) {
    switch(cgt_color) {
        case COLOR_BLACK:   return 30; case COLOR_RED:     return 31;
        case COLOR_GREEN:   return 32; case COLOR_YELLOW:  return 33;
        case COLOR_BLUE:    return 34; case COLOR_MAGENTA: return 35;
        case COLOR_CYAN:    return 36; case COLOR_WHITE:   return 37;
        case COLOR_LIGHT_BLACK:   return 90; case COLOR_LIGHT_RED:     return 91;
        case COLOR_LIGHT_GREEN:   return 92; case COLOR_LIGHT_YELLOW:  return 93;
        case COLOR_LIGHT_BLUE:    return 94; case COLOR_LIGHT_MAGENTA: return 95;
        case COLOR_LIGHT_CYAN:    return 96; case COLOR_LIGHT_WHITE:   return 97;
        default: return 37; 
    }
}

static int cgt_color_to_ansi_bg(int cgt_color) {
    switch(cgt_color) {
        case COLOR_BLACK:   return 40; case COLOR_RED:     return 41;
        case COLOR_GREEN:   return 42; case COLOR_YELLOW:  return 43;
        case COLOR_BLUE:    return 44; case COLOR_MAGENTA: return 45;
        case COLOR_CYAN:    return 46; case COLOR_WHITE:   return 47;
        case COLOR_LIGHT_BLACK:   return 100; case COLOR_LIGHT_RED:     return 101;
        case COLOR_LIGHT_GREEN:   return 102; case COLOR_LIGHT_YELLOW:  return 103;
        case COLOR_LIGHT_BLUE:    return 104; case COLOR_LIGHT_MAGENTA: return 105;
        case COLOR_LIGHT_CYAN:    return 106; case COLOR_LIGHT_WHITE:   return 107;
        default: return 40; 
    }
}

// --- API Implementation ---

void cgt_init() {
    if (cgt_initialized) return;

    tcgetattr(STDIN_FILENO, &original_termios);

    struct termios new_termios = original_termios;
    new_termios.c_lflag &= ~(ICANON | ECHO); 
    new_termios.c_iflag &= ~(ICRNL);         
    new_termios.c_cc[VMIN] = 0;  
    new_termios.c_cc[VTIME] = 0; 
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);

    setvbuf(stdout, NULL, _IOFBF, OUTPUT_BUF_SIZE);

    printf("\033[?25l"); 
    // ?1000h: Click, ?1003h: Hover, ?1006h: SGR Coords
    printf("\033[?1000h\033[?1003h\033[?1006h"); 
    fflush(stdout);

    signal(SIGINT, __cgt_signal_handler);
    signal(SIGTERM, __cgt_signal_handler);

    _g_cur_fg = -1; _g_cur_bg = -1;
    _g_cur_x = -1; _g_cur_y = -1;
    
    // Init double click state
    _g_last_click_time = 0;

    cgt_initialized = true;
}

void cgt_reset_color(); 

void cgt_close() {
    if (!cgt_initialized) return;

    printf("\033[?1006l\033[?1003l\033[?1000l"); 
    printf("\033[?25h"); 
    cgt_reset_color(); 
    fflush(stdout);

    tcsetattr(STDIN_FILENO, TCSANOW, &original_termios);
    signal(SIGINT, SIG_DFL);
    signal(SIGTERM, SIG_DFL);

    cgt_initialized = false;
}

void cgt_msleep(int milliseconds) {
    usleep(milliseconds * 1000);
}

void cgt_clear_screen(int color) {
    printf("\033[%dm", cgt_color_to_ansi_bg(color));
    printf("\033[2J\033[H\033[0m");
    fflush(stdout);
    
    _g_cur_fg = -1; _g_cur_bg = -1;
    _g_cur_x = 0; _g_cur_y = 0;
}

// --- High Performance Input Parser ---

static int __cgt_parse_input(char* buf, int len) {
    int i = 0;
    while (i < len) {
        if (buf[i] == '\033') { 
            if (i + 1 >= len) return i;
            if (buf[i+1] == '[') {
                if (i + 2 >= len) return i;

                // Mouse SGR: \033[<b;x;yM
                if (buf[i+2] == '<') {
                    int p = i + 3;
                    const char* ptr = buf + p;
                    
                    while (p < len && buf[p] != 'M' && buf[p] != 'm') p++;
                    if (p == len) return i; 

                    int b = fast_parse_int(ptr);
                    if (*ptr == ';') ptr++;
                    int x = fast_parse_int(ptr);
                    if (*ptr == ';') ptr++;
                    int y = fast_parse_int(ptr);
                    char type = buf[p]; 

                    CgtInputEvent ev;
                    ev.type = CGT_EVENT_MOUSE;
                    ev.x = x - 1; ev.y = y - 1;

                    if (type == 'M') { 
                        if (b >= 32) { // Drag 
                             ev.event = MOUSE_MOVE;
                             int btn = b - 32;
                             if (btn == 0) ev.button = MOUSE_BUTTON_LEFT;
                             else if (btn == 1) ev.button = MOUSE_BUTTON_MIDDLE;
                             else if (btn == 2) ev.button = MOUSE_BUTTON_RIGHT;
                             else ev.button = 0;
                        } else if (b == 3) {
                            ev.event = MOUSE_RELEASE; ev.button = 0;
                        } else { // Click (0, 1, 2)
                            // === Double Click Logic ===
                            long long now = _cgt_get_time_ms();
                            bool isDouble = false;
                            
                            // 检查是否在指定时间内，在相同位置，按下了相同的键
                            if (b == _g_last_click_btn && 
                                abs(ev.x - _g_last_click_x) <= 1 && // 容许1个字符的微小抖动
                                abs(ev.y - _g_last_click_y) <= 0 && 
                                (now - _g_last_click_time) < DOUBLE_CLICK_THRESHOLD_MS) 
                            {
                                isDouble = true;
                                _g_last_click_time = 0; // 重置防止触发三击
                            } else {
                                // 更新上一次点击状态
                                _g_last_click_time = now;
                                _g_last_click_x = ev.x;
                                _g_last_click_y = ev.y;
                                _g_last_click_btn = b;
                            }

                            if (isDouble) {
                                ev.event = MOUSE_DOUBLECLICK;
                            } else {
                                ev.event = MOUSE_CLICK;
                            }
                            // ==========================

                            if (b == 0) ev.button = MOUSE_BUTTON_LEFT;
                            else if (b == 1) ev.button = MOUSE_BUTTON_MIDDLE;
                            else if (b == 2) ev.button = MOUSE_BUTTON_RIGHT;
                            else ev.button = 0;
                        }
                    } else { // 'm' Release
                        ev.event = (b >= 32) ? MOUSE_MOVE : MOUSE_RELEASE;
                        ev.button = 0;
                    }
                    inputBuffer.push_back(ev);
                    i = p + 1;
                    continue;
                }
                
                // Other CSI (Skip)
                int j = i + 2;
                while (j < len && !((buf[j] >= 0x40 && buf[j] <= 0x7E))) j++;
                if (j == len) return i;
                i = j + 1;
                continue;
            } 
            i++; 
        } else {
            CgtInputEvent ev;
            ev.type = CGT_EVENT_KEY;
            ev.ch = (buf[i] == '\r') ? '\n' : buf[i];
            inputBuffer.push_back(ev);
            i++;
        }
    }
    return i; 
}

static void __cgt_read_input_to_buffer() {
    char readBuf[2048]; 
    int bytesRead = read(STDIN_FILENO, readBuf, 2048);

    if (bytesRead > 0) {
        if (__cgt_buffer_len + bytesRead < INPUT_BUF_SIZE) {
            memcpy(__cgt_partial_input_buffer + __cgt_buffer_len, readBuf, bytesRead);
            __cgt_buffer_len += bytesRead;
        } else {
             __cgt_buffer_len = 0; 
        }
    }
    if (__cgt_buffer_len == 0) return;

    int consumed = __cgt_parse_input(__cgt_partial_input_buffer, __cgt_buffer_len);
    if (consumed > 0) {
        if (consumed < __cgt_buffer_len) {
            memmove(__cgt_partial_input_buffer, &__cgt_partial_input_buffer[consumed], __cgt_buffer_len - consumed);
            __cgt_buffer_len -= consumed;
        } else {
            __cgt_buffer_len = 0;
        }
    }
}

bool cgt_has_mouse() {
    __cgt_read_input_to_buffer();
    for (const auto& ev : inputBuffer) if (ev.type == CGT_EVENT_MOUSE) return true;
    return false;
}

void cgt_get_mouse(int& x, int& y, int& button, int& event) {
    for (auto it = inputBuffer.begin(); it != inputBuffer.end(); ++it) {
        if (it->type == CGT_EVENT_MOUSE) {
            x = it->x; y = it->y; button = it->button; event = it->event;
            inputBuffer.erase(it); return;
        }
    }
}

bool cgt_has_key() {
    __cgt_read_input_to_buffer();
    for (const auto& ev : inputBuffer) if (ev.type == CGT_EVENT_KEY) return true;
    return false;
}

void cgt_get_key(char& ch) {
    for (auto it = inputBuffer.begin(); it != inputBuffer.end(); ++it) {
        if (it->type == CGT_EVENT_KEY) {
            ch = it->ch; inputBuffer.erase(it); return;
        }
    }
}

// --- Intelligent Rendering Control ---

void cgt_set_color(int foreground, int background) {
    if (foreground == _g_cur_fg && background == _g_cur_bg) {
        return;
    }
    printf("\033[%d;%dm", cgt_color_to_ansi_fg(foreground), cgt_color_to_ansi_bg(background));
    _g_cur_fg = foreground;
    _g_cur_bg = background;
}

void cgt_reset_color() {
    printf("\033[0m");
    _g_cur_fg = -1; _g_cur_bg = -1;
}

void cgt_getxy(int &x, int &y) {
    fflush(stdout); 
    printf("\033[6n");
    fflush(stdout); 

    char buf[128] = {0};
    int r = 0;
    fd_set fds;
    struct timeval tv;
    
    for(int tries=0; tries<5; ++tries) {
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);
        tv.tv_sec = 0; tv.tv_usec = 50000; 

        if (select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0) {
            int n = read(STDIN_FILENO, buf + r, 127 - r);
            if (n > 0) r += n;
            if (strchr(buf, 'R')) break; 
        } else {
            break; 
        }
    }
    
    int row = 0, col = 0;
    char* p = strrchr(buf, '[');
    if (p && sscanf(p, "[%d;%dR", &row, &col) == 2) {
        x = col - 1; y = row - 1;
        _g_cur_x = x; _g_cur_y = y; 
    }
}

void cgt_gotoxy(int x, int y) {
    int currentX = _g_cur_x;
    int currentY = _g_cur_y;
    
    if (x == -1) x = (currentX == -1) ? 0 : currentX;
    if (y == -1) y = (currentY == -1) ? 0 : currentY;

    if (x == currentX && y == currentY) {
        return;
    }
    printf("\033[%d;%dH", y + 1, x + 1);
    _g_cur_x = x;
    _g_cur_y = y;
}

// --- Print Functions ---

void cgt_print_str(const char* str, int x, int y, int foreground, int background) {
    cgt_set_color(foreground, background); 
    cgt_gotoxy(x, y);                      
    fputs(str, stdout); 
    if (_g_cur_x != -1) _g_cur_x += strlen(str);
    fflush(stdout); 
}

void cgt_print_char(char ch, int x, int y, int foreground, int background) {
    cgt_set_color(foreground, background);
    cgt_gotoxy(x, y);
    putchar(ch);
    if (_g_cur_x != -1) _g_cur_x++;
    fflush(stdout);
}

void cgt_print_int(int num, int x, int y, int foreground, int background) {
    cgt_set_color(foreground, background);
    cgt_gotoxy(x, y);
    int len = printf("%d", num);
    if (_g_cur_x != -1) _g_cur_x += len;
    fflush(stdout);
}

void cgt_print_double(double num, int x, int y, int foreground, int background) {
    cgt_set_color(foreground, background);
    cgt_gotoxy(x, y);
    int len = printf("%g", num);
    if (_g_cur_x != -1) _g_cur_x += len;
    fflush(stdout);
}

#endif // __linux__