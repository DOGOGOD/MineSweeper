#include <iostream>
#include "cgt.h"
#include "game.h"
using namespace std;
#include <string>
#include <climits>

static char ch = '\0';
int rows, cols, mineCount;

// 通过 cgt 的按键接口读取一个非负整数并在指定位置回显
int read_int_at(int x, int y, int color, int min_val = 1, int max_val = INT_MAX) {
    string s;
    char k = '\0';
    while (true) {
        cgt_wait_key();
        cgt_get_key(k);
        if (k == '\n') {
            if (s.empty()) {
                // 空输入，视为错误，提示并继续读取
                cgt_print_str("输入格式错误，请重新输入!", 1, 1, COLOR_RED);
                cgt_msleep(1000);
                cgt_print_str("                           ", 1, 1, COLOR_BLACK);
                // 清除已回显的字符
                for (size_t i = 0; i < s.size(); ++i) cgt_print_char(' ', x + (int)i, y, COLOR_BLACK, COLOR_BLACK);
                s.clear();
                continue;
            }
            int val = -1;
            try {
                val = stoi(s);
            } catch (...) {
                // 解析失败
                cgt_print_str("输入格式错误，请重新输入!", 1, 1, COLOR_RED);
                cgt_msleep(1000);
                cgt_print_str("                            ", 1, 1, COLOR_BLACK);
                for (size_t i = 0; i < s.size(); ++i) cgt_print_char(' ', x + (int)i, y, COLOR_BLACK, COLOR_BLACK);
                s.clear();
                continue;
            }
            if (val < min_val || val > max_val) {
                cgt_print_str("输入格式错误，请重新输入!", 1, 1, COLOR_RED);
                cgt_msleep(1000);
                cgt_print_str("                            ", 1, 1, COLOR_BLACK);
                for (size_t i = 0; i < s.size(); ++i) cgt_print_char(' ', x + (int)i, y, COLOR_BLACK, COLOR_BLACK);
                s.clear();
                continue;
            }
            return val;
        }
        if (k >= '0' && k <= '9') {
            s.push_back(k);
            cgt_print_char(k, x + (int)s.size() - 1, y, color);
        }
        else if (k == '\b' || k == 127) {
            if (!s.empty()) {
                int pos = (int)s.size() - 1;
                s.pop_back();
                cgt_print_char(' ', x + pos, y, COLOR_BLACK, COLOR_BLACK);
            }
        }
    }
}

void wait_input() {
    while (true) {
        if (cgt_has_key()){
            cgt_get_key(ch);
        if (ch == '1'){  
            rows = 6;
            cols = 6;
            mineCount = 6;
            return;
        }else if (ch == '2'){ 
            rows = 9;
            cols = 16;
            mineCount = 20;
            return;
        }else if (ch == '3'){
            rows = 12;
            cols = 30;
            mineCount = 80;
            return;
        }else if (ch == 'x' || ch == 'X'){
            while (true){
                cgt_clear_screen();
                cgt_print_str("请输入自定义模式的行数和列数(按回车结束每项):", 10, 5, COLOR_LIGHT_BLUE);
                cgt_print_str("rows: ", 11, 7, COLOR_YELLOW);
                cgt_print_str("cols: ", 11, 8, COLOR_YELLOW);
                rows = read_int_at(17, 7, COLOR_YELLOW);
                cols = read_int_at(17, 8, COLOR_YELLOW); 
                break;
            }
            while (true){
                cgt_print_str("请输入雷数:", 10, 11, COLOR_LIGHT_BLUE);
                cgt_print_str("mineCount: ", 11, 13, COLOR_RED);
                mineCount = read_int_at(22, 13, COLOR_RED, 1, rows * cols);
                break;
            }
                cgt_clear_screen();
                cgt_print_str("正在生成地雷...", 10, 5, COLOR_LIGHT_BLUE);
                cgt_msleep(1200);
                return;
        }else if (ch == 'q' || ch == 'Q'){
            return;
        }else{
            cgt_print_str("无效的选择，请重新输入!", 1, 1, COLOR_RED);
            cgt_msleep(1000);
            cgt_print_str("                       ", 1, 1, COLOR_BLACK);
            continue;
        }
        }
        cgt_msleep(50);
    }
}


void print_menu() {
    while (true) {
        cgt_clear_screen();
        cgt_print_str("扫雷小游戏", 10, 5, COLOR_YELLOW);
        cgt_print_str("按 1 进入简单模式", 10, 7, COLOR_LIGHT_BLUE);
        cgt_print_str("按 2 进入困难模式", 10, 8, COLOR_LIGHT_BLUE);
        cgt_print_str("按 3 进入专家模式", 10, 9, COLOR_LIGHT_BLUE);
        cgt_print_str("按 X 进入自定义模式", 10, 10, COLOR_LIGHT_BLUE);
        cgt_print_str("按 Q 退出游戏", 10, 12, COLOR_RED);
        wait_input();
        if (ch == 'q' || ch == 'Q'){
            break;
        }
        Game();
    }
}

int main() {
    cgt_init();
    cgt_clear_screen();
    print_menu();
    cgt_clear_screen();
    cgt_close();
    return 0;
}

