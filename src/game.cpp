#include "cgt.h"
#include <iostream>
#include <cstdlib>
#include <ctime>
#include "stdlib.h"

// 使用预处理宏，跨平台兼容播放音频所需库和头文件
#ifdef _WIN32
  #include <windows.h>
    #if defined(_MSC_VER)
        #pragma comment(lib, "winmm.lib")
    #endif
#elif __APPLE__
  // 暂不需要为 system("afplay ...") 引入特殊头文件，使用 stdlib.h 即可
#elif __linux__
  // 暂不需要为 system("aplay ... / mpg123 ...") 引入特殊头文件，使用 stdlib.h 即可
#endif

using namespace std;

// ================= 全局变量与基础辅助函数 =================

void play_bomb_sound() {
#ifdef _WIN32
    mciSendStringA("close resources/Bomb.mp3", NULL, 0, NULL);
    mciSendStringA("play resources/Bomb.mp3", NULL, 0, NULL);
#elif __APPLE__
    system("afplay resources/Bomb.mp3 &");
#elif __linux__
    // 使用 mpg123 在后台播放，确保系统已安装 mpg123 或者修改为其他播放器
    system("mpg123 -q resources/Bomb.mp3 &");
#endif
}

void play_victory_sound() {
#ifdef _WIN32
    mciSendStringA("close resources/Victory.mp3", NULL, 0, NULL);
    mciSendStringA("play resources/Victory.mp3", NULL, 0, NULL);
#elif __APPLE__
    system("afplay resources/Victory.mp3 &");
#elif __linux__
    system("mpg123 -q resources/Victory.mp3 &");
#endif
}

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

void generateMines(int r, int c, int rows, int cols, const int mineCount, int** mine, bool SafeZone) {
    int generated = 0;
    
    // 计算理想状态下，3x3 安全区内的格子数量
    int x = 0;
    if (SafeZone) {
        for (int dr = -1; dr <= 1; dr++) {
            for (int dc = -1; dc <= 1; dc++) {
                int nr = r + dr;
                int nc = c + dc;
                if (nr >= 0 && nr < rows && nc >= 0 && nc < cols) {
                    x++;
                }
            }
        }
    }

    // 判断当前雷数允许哪种级别的保护
    bool protect3x3 = false;
    bool protect1x1 = false;
    int protectedCells = 0;

    if (SafeZone) {
        if (mineCount <= rows * cols - x) {
            // 空间充足，开启完整的 3x3 保护
            protectedCells = x;
            protect3x3 = true;
        } else if (mineCount < rows * cols) {
            // 空间不足以保护 3x3，降级为只保护玩家点击的这 1 个格子
            protectedCells = 1;
            protect1x1 = true;
        }
    }

    // 开始布雷
    while (generated < mineCount) {
        // 计算当前还有多少个“合法且空闲”的格子可以放雷
        int availableSpaces = rows * cols - protectedCells - generated;
        if (availableSpaces <= 0) break; // 安全熔断

        int mineIdx = randomInt(0, availableSpaces - 1);
        int count = 0;
        bool placed = false; 
        
        for (int i = 0; i < rows && !placed; i++) {
            for (int j = 0; j < cols && !placed; j++) {
                // 如果该位置已经有雷，跳过
                if (isMine(mine[i][j])) continue;

                // 检查该格子是否处于被保护的状态
                bool isProtected = false;
                if (protect3x3 && abs(i - r) <= 1 && abs(j - c) <= 1) {
                    isProtected = true;
                } else if (protect1x1 && i == r && j == c) {
                    isProtected = true;
                }

                // 如果被保护，跳过
                if (isProtected) continue;

                // 找到第 mineIdx 个可用空格
                if (count == mineIdx) {
                    mine[i][j] = '*';
                    generated++;
                    placed = true; 
                } else {
                    count++; 
                }
            }
        }
    }

    // 新所有格子的周围雷数
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            updateAdjacentCount(i, j, rows, cols, mine);
        }
    }
}

// ================= 游戏状态管理 =================

extern int rows;
extern int cols;
extern int mineCount;
extern bool SafeZone;
int** mine = NULL;

void initializeGame(int r, int c, bool FirstClick) {
    mine = new int*[rows];
    if (!mine) exit(1);  

    for (int i = 0; i < rows; i++) {
        mine[i] = new int[cols];
        if (!mine[i]) exit(1);
    }

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            mine[i][j] = 0;
        }
    }

    if (FirstClick) {
        generateMines(r, c, rows, cols, mineCount, mine, SafeZone);
    }else{
        generateMines(-1, -1, rows, cols, mineCount, mine, false);
    }
}

void cleanupGame() {
    cgt_clear_screen();
    for (int i = 0; i < rows; i++) {
        delete[] mine[i];
    }
    delete[] mine;
    mine = NULL;
}

char** CreateMineLines(int rows, int cols) {
    char** Mine_lines = new char*[3 * rows + 1];
    for (int i = 0; i < 3 * rows + 1; i++) {
        Mine_lines[i] = new char[4 * cols + 2];
        for (int j = 0; j < 4 * cols + 1; j++) {
            if (i % 3 == 0) {
                Mine_lines[i][j] = ' ';
            } else {
                if (j % 4 == 0) {
                    Mine_lines[i][j] = ' ';
                } else {
                    Mine_lines[i][j] = '*';
                }
            }
        }
        Mine_lines[i][4 * cols + 1] = '\0';
    }
    return Mine_lines;
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
                        play_bomb_sound();
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

void UpdateHover(int x, int y, int& lastR, int& lastC, int** internalMine, int* userMine, int rows, int cols, char** Mine_lines) {
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
void DrawBoard(char** Mine_lines, int nlines) {
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
void ProcessGameLoop(int* userMine, char** Mine_lines, const char* titleStr, int winTarget) {
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
    bool FirstClick = true;

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

        int r = (y-5)/3;
        int c = (x-5)/4;

        if (event == MOUSE_CLICK) {
            if (Mine_lines[(y - 3)][(x - 3)] == '*'){
                if (FirstClick) {
                    initializeGame(r, c, FirstClick);
                    FirstClick = false;
                }
            }
            if (button == MOUSE_BUTTON_LEFT) {
                if (Mine_lines[(y - 3)][(x - 3)] == '*') {
                    // 踩雷判断
                    if (mine[r][c] == '*'){
                        play_bomb_sound();
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
            play_victory_sound();
            //胜利时显示最终用时 
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

void Game() {
    initializeGame(0, 0, false);
    cgt_clear_screen();
    
    char** Mine_lines = CreateMineLines(rows, cols);
    long nlines = 3 * rows + 1;
    DrawBoard(Mine_lines, nlines);

    
    int* userMine = new int[rows * cols];
    for (int i = 0; i < rows * cols; ++i) userMine[i] = 0;

    char titleBuf[128];
    snprintf(titleBuf, sizeof(titleBuf), "扫雷 : %dx%d , 雷数 : %d , 剩余 : ", rows, cols, mineCount);

    ProcessGameLoop(userMine, Mine_lines, titleBuf, mineCount);
    for (long i = 0; i < nlines; ++i) delete[] Mine_lines[i];
    delete[] Mine_lines;
    delete[] userMine;
    return;
}