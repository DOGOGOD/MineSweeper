#include <iostream>
#include "cgt.h"
#include "game.h"
using namespace std;

static char ch = '\0';

void wait_input() {
    while (true) {
        if (cgt_has_key()){
            cgt_get_key(ch);
        if (ch == '1'){  
            Game(1);
            return;
        }else if (ch == '2'){  
            Game(2);
            return;
        }else if (ch == '3'){
            Game(3);
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
        cgt_print_str("按 Q 退出游戏", 10, 11, COLOR_RED);
        wait_input();
        if (ch == 'q' || ch == 'Q'){
            break;
        }
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

