/*
    Console Graphic Tools
    
    by gty

    Learnt from Shen Jian at Tongji University
*/

// 助教注意：发给使用苹果 macOS 的同学前，应将本文件转换成 UTF-8 编码。
//          发给使用 Windows 的同学前，应将本文件转换成 国标 编码。


#pragma once


#define COLOR_BLACK         0
#define COLOR_BLUE          1
#define COLOR_GREEN         2
#define COLOR_CYAN          3
#define COLOR_RED           4
#define COLOR_MAGENTA       5
#define COLOR_YELLOW        6
#define COLOR_WHITE         7

#define COLOR_LIGHT_BLACK   8
#define COLOR_LIGHT_BLUE    9
#define COLOR_LIGHT_GREEN   10
#define COLOR_LIGHT_CYAN    11
#define COLOR_LIGHT_RED     12
#define COLOR_LIGHT_MAGENTA 13
#define COLOR_LIGHT_YELLOW  14
#define COLOR_LIGHT_WHITE   15


#define MOUSE_BUTTON_LEFT   1
#define MOUSE_BUTTON_RIGHT  2
#define MOUSE_BUTTON_MIDDLE 3

#define MOUSE_CLICK       1
#define MOUSE_DOUBLECLICK 2
#define MOUSE_RELEASE     3
#define MOUSE_MOVE        4


/**
 * 初始化 cgt。
 * 
 * 在程序开始时调用一下即可。比如在 main 函数进来后的第一行。
 */
void cgt_init();

/**
 * 关闭 cgt，防止你的控制台乱掉。
 *
 * 在程序结束前调用一下即可。比如在 main 函数 return 之前。
 */
void cgt_close();


/**
 * 程序暂停指定时间。
 * 单位：毫秒
 */
void cgt_msleep(int milliseconds);


/**
 * 清空屏幕内容，并将屏幕颜色设置成你指定的颜色。
 *
 * @param color 颜色，默认是 COLOR_BLACK。
 */
void cgt_clear_screen(int color = COLOR_BLACK);

/**
 * 检查是否有鼠标事件。
 *
 * 返回 true 表示有鼠标事件，可以调用 cgt_get_mouse 获取。
 */
bool cgt_has_mouse();

/**
 * 获取 1 个鼠标事件。
 *
 * 你应该先调用 cgt_has_mouse 检查是否有鼠标事件，仅当有鼠标事件时才调用本函数。 
 */
void cgt_get_mouse(int& x, int& y, int& button, int& event);


/**
 * 检查是否有键盘事件。
 *
 * 返回 true 表示有键盘事件，可以调用 cgt_get_key 获取。
 */
bool cgt_has_key();

/**
 * 获取 1 个键盘输入的字符。
 */
void cgt_get_key(char& ch);


void cgt_set_color(int foreground = COLOR_WHITE, int background = COLOR_BLACK);
void cgt_reset_color();

void cgt_getxy(int &x, int &y);
void cgt_gotoxy(int x, int y);


/**
 * 在 (x, y) 位置打印字符串 str，颜色为 foreground 和 background。
 */
void cgt_print_str(const char* str, int x = -1, int y = -1, int foreground = COLOR_WHITE, int background = COLOR_BLACK);


/**
 * 在 (x, y) 位置打印字符 ch，颜色为 foreground 和 background。
 */
void cgt_print_char(char str, int x = -1, int y = -1, int foreground = COLOR_WHITE, int background = COLOR_BLACK);


/**
 * 在 (x, y) 位置打印浮点数 value，颜色为 foreground 和 background。
 */
void cgt_print_double(double value, int x = -1, int y = -1, int foreground = COLOR_WHITE, int background = COLOR_BLACK);


/**
 * 在 (x, y) 位置打印整数 value，颜色为 foreground 和 background。
 */
void cgt_print_int(int value, int x = -1, int y = -1, int foreground = COLOR_WHITE, int background = COLOR_BLACK);


/**
 * 一直等待，直到检测到存在键盘输入事件。
 * 此函数返回后，可通过 cgt_get_key 获取按下的键。
 */
inline void cgt_wait_key() {
    while (!cgt_has_key()) {
        cgt_msleep(5);
    }
}


/**
 * 一直等待，直到检测到存在鼠标事件。
 * 此函数返回后，可通过 cgt_get_mouse 获取鼠标事件的信息。
 */
inline void cgt_wait_mouse() {
    while (!cgt_has_mouse()) {
        cgt_msleep(5);
    }
}
