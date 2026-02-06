#ifndef GAME_H
#define GAME_H

// 初始化并开始游戏，根据传入的模式 (1=简单, 2=困难, 3=专家)
void Game();

// ================= 全局变量与基础辅助函数 =================
void wait_for_enter();
int randomInt(int low, int high);
bool isMine(int cell);
void updateAdjacentCount(int row, int col, int rows, int cols, int** mines);
void generateMines(int rows, int cols, const int mineCount, int** mine);

// ================= 游戏状态管理 =================
extern int rows;
extern int cols;
extern int mineCount;
extern int** mine;

void Mode(int mode);
void initializeGame();
void cleanupGame();

// ================= 游戏核心逻辑 (展开、高亮、双击) =================
void AutoSwitch(int x, int y, int** mine);
bool TryChord(int x, int y, int* userMine);
void UpdateHover(int x, int y, int& lastR, int& lastC, int** internalMine, int* userMine,
				 int rows, int cols, const char** Mine_lines);

// ================= 游戏主逻辑函数 =================
void DrawBoard(const char** Mine_lines, int nlines);
void ProcessGameLoop(int* userMine, const char** Mine_lines, const char* titleStr, int winTarget);

#endif // GAME_H