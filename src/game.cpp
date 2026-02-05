#include "cgt.h"
#include <iostream>
#include <cstdlib>
#include <ctime>
using namespace std;

// ================= 全局变量与基础辅助函数 =================

void wait_for_enter() {
    // 清空键盘输入事件缓冲。
    while (cgt_has_key()) {
        char ch;
        cgt_get_key(ch);
    }

    // 等待任意按键。
    while (true) {
        if (cgt_has_key()) {
            char ch;
            cgt_get_key(ch);
            // 读到任意按键，都可结束。
            break;
        }
        // 适当休息，防止占满电脑 CPU。
        cgt_msleep(50);
    }
}

int randomInt(int low, int high) {
    static bool initialized = false;
    if (!initialized) {
        srand((unsigned int) time(nullptr));
        initialized = true;
    }
    return low + rand() % (high - low + 1);
}

bool isMine(int cell) {
    return cell == '*';
}

void updateAdjacentCount(int row, int col, int rows, int cols, int** mines) {
    if (isMine(mines[row][col]))
        return;
 
    int count = 0;
    for (int dr = -1; dr <= 1; dr++) {
        for (int dc = -1; dc <= 1; dc++) {
            int nr = row + dr;
            int nc = col + dc;
            if (nr >= 0 && nr < rows && nc >= 0 && nc < cols)
                count += !!isMine(mines[nr][nc]);
        }
    }
    mines[row][col] = count;
}

void generateMines(int rows, int cols, const int mineCount, int** mine) {
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            mine[r][c] = 0;
        }
    }

    int generated = 0;
    while (generated < mineCount) {
        int mineIdx = randomInt(0, rows * cols - 1 - generated);

        int count = 0;
        for (int r = 0; r < rows; r++) {
            for (int c = 0; c < cols; c++) {
                if (isMine(mine[r][c]))
                    continue;

                if (count == mineIdx) {
                    mine[r][c] = '*';
                    generated++;
                    count++;
                    break;
                }
                else {
                    count++;
                }
            }
        }
    }

    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            updateAdjacentCount(r, c, rows, cols, mine);
        }
    }
}

// ================= 游戏状态管理 =================

int rows = 0;
int cols = 0;
int mineCount = 0;
int** mine = NULL;

void Mode(int mode) {
    if (mode == 1) {
        rows = 6;
        cols = 6;
        mineCount = 6;
    }else if(mode == 2) {
        rows = 9;
        cols = 16;
        mineCount = 20;
    }else if(mode == 3) {
        rows = 12;
        cols = 30;
        mineCount = 80;
    }
}

void initializeGame() {
    mine = new int*[rows];
    if (!mine) exit(1);  

    for (int r = 0; r < rows; r++) {
        mine[r] = new int[cols];
        if (!mine[r]) exit(1);
    }
    generateMines(rows, cols, mineCount, mine);
}

void cleanupGame() {
    cgt_clear_screen();
    for (int i = 0; i < rows; i++) {
        delete[] mine[i];
    }
    delete[] mine;
    mine = NULL;
}

// ================= 游戏核心逻辑 (展开、高亮、双击) =================

void AutoSwitch(int x, int y, int** mine) {
    int r = (y - 5) / 3;
    int c = (x - 5) / 4;

    if (r < 0 || r >= rows || c < 0 || c >= cols) return;
    if (mine[r][c] < 0) return;

    int val = mine[r][c];
    mine[r][c] = -(val+1); 

    cgt_print_char(' ', x - 1, y, COLOR_LIGHT_WHITE, COLOR_LIGHT_WHITE);
    cgt_print_char(' ', x + 1, y, COLOR_LIGHT_WHITE, COLOR_LIGHT_WHITE);
    cgt_print_char(' ', x - 1, y - 1, COLOR_LIGHT_WHITE, COLOR_LIGHT_WHITE);
    cgt_print_char(' ', x, y - 1, COLOR_LIGHT_WHITE, COLOR_LIGHT_WHITE);
    cgt_print_char(' ', x + 1, y - 1, COLOR_LIGHT_WHITE, COLOR_LIGHT_WHITE);

    if (val > 0 && val <= 8) {
        int color;
        switch (val) {
            case 1: color = COLOR_LIGHT_BLUE; break;
            case 2: color = COLOR_LIGHT_GREEN; break;
            case 3: color = COLOR_LIGHT_MAGENTA; break;
            case 4: color = COLOR_MAGENTA; break;
            case 5: color = COLOR_BLUE; break;
            case 6: color = COLOR_LIGHT_RED; break;
            case 7: color = COLOR_LIGHT_RED; break;
            case 8: color = COLOR_RED; break;
            default: color = COLOR_LIGHT_WHITE; break;
        }
        cgt_print_int(val, x, y, color, COLOR_LIGHT_WHITE);
    }
    else if (val == 0) {
        cgt_print_char(' ', x, y, COLOR_LIGHT_WHITE, COLOR_LIGHT_WHITE);
        AutoSwitch(x - 4, y, mine);
        AutoSwitch(x + 4, y, mine);
        AutoSwitch(x, y - 3, mine);
        AutoSwitch(x, y + 3, mine);
        AutoSwitch(x - 4, y - 3, mine);
        AutoSwitch(x + 4, y - 3, mine);
        AutoSwitch(x - 4, y + 3, mine);
        AutoSwitch(x + 4, y + 3, mine);
    }
}

bool TryChord(int x, int y, int* userMine) {
    int r = (y - 5) / 3;
    int c = (x - 5) / 4;
    if (r < 0 || r >= rows || c < 0 || c >= cols) return false;

    if (mine[r][c] > 0) return false;

    int realMineCount = - mine[r][c] - 1;

    int flagCount = 0;
    for (int i = -1; i <= 1; ++i) {
        for (int j = -1; j <= 1; ++j) {
            if (i == 0 && j == 0) continue;
            int nr = r + i;
            int nc = c + j;
            if (nr >= 0 && nr < rows && nc >= 0 && nc < cols) {
                if (userMine[nr * cols + nc] == -1) {
                    flagCount++;
                }
            }
        }
    }

    if (flagCount == realMineCount) {
        for (int i = -1; i <= 1; ++i) {
            for (int j = -1; j <= 1; ++j) {
                if (i == 0 && j == 0) continue;
                int nr = r + i;
                int nc = c + j;

                if (nr >= 0 && nr < rows && nc >= 0 && nc < cols) {
                    if (userMine[nr * cols + nc] == -1 || mine[nr][nc] < 0) continue;

                    int sx = 5 + nc * 4;
                    int sy = 5 + nr * 3;

                    if (mine[nr][nc] == '*') {
                        cgt_print_char('*', sx, sy, COLOR_BLACK, COLOR_MAGENTA);
                        cgt_print_char(' ', sx-1, sy, COLOR_MAGENTA, COLOR_MAGENTA);
                        cgt_print_char(' ', sx+1, sy, COLOR_MAGENTA, COLOR_MAGENTA);
                        cgt_print_char(' ', sx-1, sy-1, COLOR_MAGENTA, COLOR_MAGENTA);
                        cgt_print_char(' ', sx, sy-1, COLOR_MAGENTA, COLOR_MAGENTA);
                        cgt_print_char(' ', sx+1, sy-1, COLOR_MAGENTA, COLOR_MAGENTA);
                        cgt_print_str("游戏结束！你踩到雷了！按任意键退出。", 1, 2, COLOR_RED, COLOR_BLACK);
                        return true;
                    } else {
                        AutoSwitch(sx, sy, mine);
                    }
                }
            }
        }
    }
    return false;
}

void UpdateHover(int x, int y, int& lastR, int& lastC, int** internalMine, int* userMine, int rows, int cols, const char** Mine_lines) {
    if (x < 3 || x > 3 + cols * 4 || y < 3 || y > 3 + rows * 3) {
        if (lastR != -1) {
            if (internalMine[lastR][lastC] >= 0) {
                int sx = 5 + lastC * 4;
                int sy = 5 + lastR * 3;
                if (userMine[lastR * cols + lastC] == -1) {
                    cgt_print_char('F', sx, sy, COLOR_BLACK, COLOR_RED);
                    cgt_print_char(' ', sx-1, sy, COLOR_RED, COLOR_RED);
                    cgt_print_char(' ', sx+1, sy, COLOR_RED, COLOR_RED);
                    cgt_print_char(' ', sx-1, sy-1, COLOR_RED, COLOR_RED);
                    cgt_print_char(' ', sx, sy-1, COLOR_RED, COLOR_RED);
                    cgt_print_char(' ', sx+1, sy-1, COLOR_RED, COLOR_RED);
                } else {
                    cgt_print_char(' ', sx, sy, COLOR_YELLOW, COLOR_YELLOW);
                    cgt_print_char(' ', sx-1, sy, COLOR_YELLOW, COLOR_YELLOW);
                    cgt_print_char(' ', sx+1, sy, COLOR_YELLOW, COLOR_YELLOW);
                    cgt_print_char(' ', sx-1, sy-1, COLOR_YELLOW, COLOR_YELLOW);
                    cgt_print_char(' ', sx, sy-1, COLOR_YELLOW, COLOR_YELLOW);
                    cgt_print_char(' ', sx+1, sy-1, COLOR_YELLOW, COLOR_YELLOW);
                }
            }
            lastR = -1;
            lastC = -1;
            return;
        } else {
            return;
        }
    }
        
    if (Mine_lines[(y - 3)][(x - 3)] == '*') {
        while (Mine_lines[(y - 4)][(x - 3)] == ' ' || Mine_lines[(y - 3)][(x - 4)] == ' ' || Mine_lines[(y - 3)][(x - 2)] == ' '){
            if (Mine_lines[(y - 4)][(x - 3)] == ' ') {                        
                y = y + 1;
            }else if (Mine_lines[(y - 3)][(x - 4)] == ' ') {
                x = x + 1;
            }else if (Mine_lines[(y - 3)][(x - 2)] == ' ') {
                x = x - 1;
            }
        }
    }

    bool isInside = Mine_lines[(y - 3)][(x - 3)] == '*';
    int r = (y - 5) / 3;
    int c = (x - 5) / 4;

    if (isInside && internalMine[r][c] < 0) {
        isInside = false;
    }

    if (isInside && r == lastR && c == lastC) return;
    if (!isInside && lastR == -1) return;

    if (lastR != -1) {
        if (internalMine[lastR][lastC] >= 0) {
            int sx = 5 + lastC * 4;
            int sy = 5 + lastR * 3;
            if (userMine[lastR * cols + lastC] == -1) {
                cgt_print_char('F', sx, sy, COLOR_BLACK, COLOR_RED);
                cgt_print_char(' ', sx-1, sy, COLOR_RED, COLOR_RED);
                cgt_print_char(' ', sx+1, sy, COLOR_RED, COLOR_RED);
                cgt_print_char(' ', sx-1, sy-1, COLOR_RED, COLOR_RED);
                cgt_print_char(' ', sx, sy-1, COLOR_RED, COLOR_RED);
                cgt_print_char(' ', sx+1, sy-1, COLOR_RED, COLOR_RED);
            } else {
                cgt_print_char(' ', sx, sy, COLOR_YELLOW, COLOR_YELLOW);
                cgt_print_char(' ', sx-1, sy, COLOR_YELLOW, COLOR_YELLOW);
                cgt_print_char(' ', sx+1, sy, COLOR_YELLOW, COLOR_YELLOW);
                cgt_print_char(' ', sx-1, sy-1, COLOR_YELLOW, COLOR_YELLOW);
                cgt_print_char(' ', sx, sy-1, COLOR_YELLOW, COLOR_YELLOW);
                cgt_print_char(' ', sx+1, sy-1, COLOR_YELLOW, COLOR_YELLOW);
            }
        }
        lastR = -1;
        lastC = -1;
    }

    if (isInside) {
        int sx = 5 + c * 4;
        int sy = 5 + r * 3;
        
        if (userMine[r * cols + c] == -1) {
            cgt_print_char('F', sx, sy, COLOR_BLACK, COLOR_LIGHT_RED);
            cgt_print_char(' ', sx-1, sy, COLOR_LIGHT_RED, COLOR_LIGHT_RED);
            cgt_print_char(' ', sx+1, sy, COLOR_LIGHT_RED, COLOR_LIGHT_RED);
            cgt_print_char(' ', sx-1, sy-1, COLOR_LIGHT_RED, COLOR_LIGHT_RED);
            cgt_print_char(' ', sx, sy-1, COLOR_LIGHT_RED, COLOR_LIGHT_RED);
            cgt_print_char(' ', sx+1, sy-1, COLOR_LIGHT_RED, COLOR_LIGHT_RED);
        } else {
            cgt_print_char(' ', sx, sy, COLOR_LIGHT_YELLOW, COLOR_LIGHT_YELLOW);
            cgt_print_char(' ', sx-1, sy, COLOR_LIGHT_YELLOW, COLOR_LIGHT_YELLOW);
            cgt_print_char(' ', sx+1, sy, COLOR_LIGHT_YELLOW, COLOR_LIGHT_YELLOW);
            cgt_print_char(' ', sx-1, sy-1, COLOR_LIGHT_YELLOW, COLOR_LIGHT_YELLOW);
            cgt_print_char(' ', sx, sy-1, COLOR_LIGHT_YELLOW, COLOR_LIGHT_YELLOW);
            cgt_print_char(' ', sx+1, sy-1, COLOR_LIGHT_YELLOW, COLOR_LIGHT_YELLOW);
        }
        lastR = r;
        lastC = c;
    }
}

// ================= 游戏主逻辑函数 =================

/**
 * 绘制棋盘背景
 */
void DrawBoard(const char** Mine_lines, int nlines) {
    const long offset_x = 3;
    const long offset_y = 3;

    for (long row = 0; row < nlines; row++) {
        const char* line = Mine_lines[row];
        for (long col = 0; line[col] != '\0'; col++) {
            if (line[col] == '*'){
                cgt_print_char(' ', col + offset_x, row + offset_y, COLOR_YELLOW, COLOR_YELLOW);
            }else{
                cgt_print_char(' ', col + offset_x, row + offset_y, COLOR_WHITE, COLOR_WHITE);
            }
        }
    }
}

/**
 * 游戏主循环：处理鼠标事件、胜负判定
 * userMine: 用户状态数组 (一维化指针)
 * Mine_lines: 棋盘字符画
 * titleStr: 标题文字
 * winTarget: 胜利所需的插旗数 (即初始雷数)
 */
void ProcessGameLoop(int* userMine, const char** Mine_lines, const char* titleStr, int winTarget) {
    // 清空鼠标缓冲
    while (cgt_has_mouse()) {
        int x, y, button, event;
        cgt_get_mouse(x, y, button, event);
    }
    
    int lastR = -1;
    int lastC = -1;
    int flag = 0;

    //记录游戏开始的时间戳
    time_t startTime = time(nullptr);

    while (true) {
        cgt_gotoxy(0, 0);
        cgt_print_str(titleStr, 1, 1, COLOR_WHITE, COLOR_BLACK);
        cgt_print_int(mineCount);
        cgt_print_str("  "); // 清除雷数后面可能残留的字符

        // 计算并显示已用时间
        // time(nullptr) 获取当前时间戳，减去开始时间即为经过的秒数
        int elapsed = (int)(time(nullptr) - startTime);
        cgt_print_str("用时 : ", -1, -1, COLOR_WHITE, COLOR_BLACK);
        cgt_print_int(elapsed, -1, -1, COLOR_LIGHT_CYAN, COLOR_BLACK); // 使用青色高亮时间
        cgt_print_str(" 秒    "); // 补空格防止数字变短时残留

        if (!cgt_has_mouse()) {
            cgt_msleep(50);
            continue;
        }

        int x, y, button, event;
        cgt_get_mouse(x, y, button, event);
        
        // 调用通用 Hover 函数
        UpdateHover(x, y, lastR, lastC, mine, userMine, rows, cols, Mine_lines);
        
        // 边界检查
        if (x < 3 || x > 3 + cols * 4 || y < 3 || y > 3 + rows * 3) {
            continue;
        }
        
        // 坐标修正逻辑 (将坐标位置映射到扫雷棋盘上)
        if (Mine_lines[(y - 3)][(x - 3)] == '*') {
            while (Mine_lines[(y - 4)][(x - 3)] == ' ' || Mine_lines[(y - 3)][(x - 4)] == ' ' || Mine_lines[(y - 3)][(x - 2)] == ' '){
                if (Mine_lines[(y - 4)][(x - 3)] == ' ') {                        
                    y = y + 1;
                }else if (Mine_lines[(y - 3)][(x - 4)] == ' ') {
                    x = x + 1;
                }else if (Mine_lines[(y - 3)][(x - 2)] == ' ') {
                    x = x - 1;
                }
            }
        }

        if (event == MOUSE_CLICK) {
            if (button == MOUSE_BUTTON_LEFT) {
                if (Mine_lines[(y - 3)][(x - 3)] == '*') {
                    int r = (y-5)/3;
                    int c = (x-5)/4;
                    // 踩雷判断
                    if (mine[r][c] == '*'){
                        cgt_print_char('*', x, y, COLOR_BLACK, COLOR_MAGENTA);
                        cgt_print_char(' ', x-1, y, COLOR_MAGENTA, COLOR_MAGENTA);
                        cgt_print_char(' ', x+1, y, COLOR_MAGENTA, COLOR_MAGENTA);
                        cgt_print_char(' ', x-1, y-1, COLOR_MAGENTA, COLOR_MAGENTA);
                        cgt_print_char(' ', x, y-1, COLOR_MAGENTA, COLOR_MAGENTA);
                        cgt_print_char(' ', x+1, y-1, COLOR_MAGENTA, COLOR_MAGENTA);
                        cgt_print_str("游戏结束！你踩到雷了！按任意键退出。", 1, 2, COLOR_RED, COLOR_BLACK);
                        wait_for_enter();
                        cleanupGame();
                        return;
                    }else{                                          
                        AutoSwitch(x, y, mine);
                        continue;
                    }
                }
            } else if (button == MOUSE_BUTTON_RIGHT) {
                if (Mine_lines[(y - 3)][(x - 3)] == '*') {
                    int r = (y-5)/3;
                    int c = (x-5)/4;
                    int idx = r * cols + c; // 一维数组索引

                    // 插旗逻辑
                    if (userMine[idx] == 0 && (mine[r][c] >= 0 || mine[r][c] == '*')){
                        userMine[idx] = -1;
                        cgt_print_char('F', x, y, COLOR_BLACK, COLOR_RED);
                        cgt_print_char(' ', x-1, y, COLOR_RED, COLOR_RED);
                        cgt_print_char(' ', x+1, y, COLOR_RED, COLOR_RED);
                        cgt_print_char(' ', x-1, y-1, COLOR_RED, COLOR_RED);
                        cgt_print_char(' ', x, y-1, COLOR_RED, COLOR_RED);
                        cgt_print_char(' ', x+1, y-1, COLOR_RED, COLOR_RED);
                        mineCount--;
                        if (mine[r][c] == '*'){
                            flag++;
                        }
                        continue;
                    }
                    // 取消插旗逻辑
                    else if(userMine[idx] == -1 && (mine[r][c] >= 0 || mine[r][c] == '*')){
                        userMine[idx] = 0;
                        cgt_print_char(' ', x, y, COLOR_YELLOW, COLOR_YELLOW);
                        cgt_print_char(' ', x-1, y, COLOR_YELLOW, COLOR_YELLOW);
                        cgt_print_char(' ', x+1, y, COLOR_YELLOW, COLOR_YELLOW);
                        cgt_print_char(' ', x-1, y-1, COLOR_YELLOW, COLOR_YELLOW);
                        cgt_print_char(' ', x, y-1, COLOR_YELLOW, COLOR_YELLOW);
                        cgt_print_char(' ', x+1, y-1, COLOR_YELLOW, COLOR_YELLOW);
                        mineCount++;
                        if (mine[r][c] == '*'){
                            flag--;
                        }
                        continue;
                    }
                }
            }
        } else if (event == MOUSE_DOUBLECLICK && button == MOUSE_BUTTON_LEFT){
            if (Mine_lines[(y - 3)][(x - 3)] == '*') {
                if (TryChord(x, y, userMine)){
                    wait_for_enter();
                    cleanupGame();
                    return;
                }
            }
        }

        if (flag == winTarget){
            // [可选优化] 胜利时显示最终用时
            int finalTime = (int)(time(nullptr) - startTime);
            cgt_print_str("游戏结束！你成功清除所有雷！最终用时: ", 1, 2, COLOR_GREEN, COLOR_BLACK);
            cgt_print_int(finalTime, -1, -1, COLOR_LIGHT_CYAN, COLOR_BLACK);
            cgt_print_str(" 秒。按任意键退出。");
            
            wait_for_enter();
            cleanupGame();
            return;
        }
    }
}

// ================= 主入口函数 =================

void Game(int mode) {
    Mode(mode);
    initializeGame();
    cgt_clear_screen();

    // 根据模式定义不同的用户状态数组和字符画
    // 使用局部数组以保持原始逻辑的内存分配方式，但统一传递指针
    
    if (mode == 1) {
        int Mine[6][6] = {0};
        const char* Mine_lines[] = {
            "                         ",
            " *** *** *** *** *** *** ",
            " *** *** *** *** *** *** ",
            "                         ",
            " *** *** *** *** *** *** ",
            " *** *** *** *** *** *** ",
            "                         ",
            " *** *** *** *** *** *** ",
            " *** *** *** *** *** *** ",
            "                         ",
            " *** *** *** *** *** *** ",
            " *** *** *** *** *** *** ",
            "                         ",
            " *** *** *** *** *** *** ",
            " *** *** *** *** *** *** ",
            "                         ",
            " *** *** *** *** *** *** ",
            " *** *** *** *** *** *** ",
            "                         ",
        };
        long nlines = sizeof(Mine_lines) / sizeof(Mine_lines[0]);
        DrawBoard(Mine_lines, nlines);
        ProcessGameLoop((int*)Mine, Mine_lines, "扫雷 : 6x6 , 雷数 : 6 , 剩余 : ", 6);
    }
    else if (mode == 2) {
        int Mine[9][16] = {0};
        const char* Mine_lines[] = {
            "                                                                 ",
            " *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ",
            " *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ",
            "                                                                 ",
            " *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ",
            " *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ",
            "                                                                 ",
            " *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ",
            " *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ",
            "                                                                 ",
            " *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ",
            " *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ",
            "                                                                 ",
            " *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ",
            " *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ",
            "                                                                 ",
            " *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ",
            " *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ",
            "                                                                 ",
            " *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ",
            " *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ",
            "                                                                 ",
            " *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ",
            " *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ",
            "                                                                 ",
            " *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ",
            " *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ",
            "                                                                 ",
        };
        long nlines = sizeof(Mine_lines) / sizeof(Mine_lines[0]);
        DrawBoard(Mine_lines, nlines);
        ProcessGameLoop((int*)Mine, Mine_lines, "扫雷 : 9x16 , 雷数 : 20 , 剩余 : ", 20);
    }
    else if (mode == 3) {
        int Mine[12][30] = {0};
        const char* Mine_lines[] = {
            "                                                                                                                         ",
            " *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ",
            " *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ",
            "                                                                                                                         ",
            " *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ",
            " *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ",
            "                                                                                                                         ",
            " *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ",
            " *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ",
            "                                                                                                                         ",
            " *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ",
            " *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ",
            "                                                                                                                         ",
            " *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ",
            " *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ",
            "                                                                                                                         ",
            " *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ",
            " *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ",
            "                                                                                                                         ",
            " *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ",
            " *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ",
            "                                                                                                                         ",
            " *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ",
            " *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ",
            "                                                                                                                         ",
            " *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ",
            " *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ",
            "                                                                                                                         ",
            " *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ",
            " *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ",
            "                                                                                                                         ",
            " *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ",
            " *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ",
            "                                                                                                                         ",
            " *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ",
            " *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ",
            "                                                                                                                         ",
        };
        long nlines = sizeof(Mine_lines) / sizeof(Mine_lines[0]);
        DrawBoard(Mine_lines, nlines);
        ProcessGameLoop((int*)Mine, Mine_lines, "扫雷 : 12x30 , 雷数 : 80 , 剩余 : ", 80);
    }
}