# 💣 C++ Console Minesweeper | C++ 控制台扫雷

[中文](#中文说明) [English](#english-description)

---

## 中文说明

这是一个运行在 Windows 控制台中的经典扫雷游戏实现。它使用了自定义的图形工具包装器（`CGT - Console Graphic Tools`）来处理鼠标输入、颜色渲染和光标控制，在文本环境中提供了丰富的交互体验。（本项目为上海交通大学程序设计（C++）课程设计作业。）

### ✨ 功能特性

* **控制台 GUI 体验：** 完整的鼠标支持，包括点击、双击和悬停高亮。
* **彩色界面：** 数字、旗帜和地雷使用不同的颜色显示，清晰易读。
* **三种难度模式：**
* **简单模式：** 6x6 网格，6 个雷。
* **困难模式：** 9x16 网格，20 个雷。
* **专家模式：** 12x30 网格，80 个雷。

* **自定义模式：** 按 `X` 进入自定义模式，允许玩家指定行数、列数与地雷总数以创建任意难度。


* **高级机制：**
* **自动展开：** 点击空白区域会自动递归清除周围无雷区域（Flood fill）。
* **双击清理 (Chord)：** 如果数字周围已插旗数量符合要求，双击该数字可快速翻开周围其余方块。
* **插旗系统：** 标记疑似地雷的位置。



### 🎮 操作指南

游戏主要依赖鼠标交互。请确保控制台窗口处于激活状态。

| 动作 | 输入方式 | 说明 |
| --- | --- | --- |
| **翻开方块** | **鼠标左键** | 挖掘方块。如果是地雷，游戏结束。 |
| **插旗/取消** | **鼠标右键** | 在怀疑是雷的方块上放置旗帜 (🚩)。 |
| **双击清理** | **鼠标左键双击** | 当已翻开数字周围的旗帜数量与数字相等时，双击该数字可瞬间翻开周围所有未标记方块。 |
| **菜单选择** | **键盘 1-3** | 在主菜单选择难度。 |
| **退出** | **键盘 Q** | 在菜单界面退出游戏。 |

### 📁 项目结构

```
MineSweeper/
├── include/                # 头文件目录
│   ├── cgt.h              # CGT 库接口声明（跨平台控制台图形工具）
│   └── game.h             # 游戏核心逻辑接口声明
├── src/                    # 源文件目录
│   ├── cgt_windows.cpp    # CGT Windows 平台实现
│   ├── cgt_apple.cpp      # CGT macOS 平台实现
│   ├── cgt_linux.cpp      # CGT Linux 平台实现
│   └── game.cpp           # 游戏核心逻辑实现
├── main.cpp               # 程序入口
├── Makefile               # 编译配置文件
└── README.md              # 项目说明文档
```

### 🛠️ 编译与运行

**前置要求：**

* Windows or macOS or Linux。
* C++ 编译器（如 GCC, Clang 等）。

**使用Makefile编译**
请确保所有源文件（`main.cpp`, `game.cpp`, `cgt_windows.cpp`/`cgt_apple.cpp`, `cgt.h`, `game.h`） 和头文件在同一目录下。

```bash
make
```
自动读取Makefile中的编译指令，生成可执行文件 `minesweeper.exe`。

> **注意：** 
> - 代码会自动尝试禁用控制台的“快速编辑模式”以确保鼠标点击生效。如果点击无反应，请检查终端设置。
> - 如果出现乱码，请切换编码格式。Linux和macOS：UTF-8；Windows：国标编码。
               

### 🚀 后续计划

为了进一步提升游戏体验和代码质量，计划在后续版本中实现以下功能：

* [x] **首点击保护机制**：实现玩家第一次点击时百分之百不是地雷。
* [x] **首点击开阔区保护**：实现玩家第一次点击时，不仅该点不是雷，且周围 8 格也均无地雷（确保开局即有一个空白区域）。
* [x] **自定义难度设置**：允许用户根据个人喜好自定义网格大小（行与列）及地雷总数。
* [x] **Linux 支持**：移植到 Linux 平台，实现跨平台兼容。
* [x] **计时器功能**：实时记录游戏耗时，增加挑战性。

---

## English Description

A classic Minesweeper game implementation running entirely in the Windows Console. It utilizes a custom graphics wrapper (`CGT - Console Graphic Tools`) to handle mouse inputs, colors, and cursor manipulation, providing a rich interactive experience within a text-based environment. （This project is a course design for Shanghai Jiao Tong University's CS1501 course.）

### ✨ Key Features

* **GUI-like Experience in Console:** Full mouse support including clicking and hovering.
* **Colorful Interface:** Different colors for numbers, flags, and mines for better visibility.
* **Three Difficulty Levels:**
* **Simple:** 6x6 Grid, 6 Mines.
* **Hard:** 9x16 Grid, 20 Mines.
* **Expert:** 12x30 Grid, 80 Mines.

* **Custom:** Press `X` in the menu to enter a custom mode where the player can set rows, columns and total mines.


* **Advanced Mechanics:**
* **Recursive Expansion:** Automatically clears empty areas (Flood fill).
* **Chording:** Double-click support to quickly clear surrounding cells if flags match.
* **Flag System:** Mark potential mines.



### 🎮 Controls & How to Play

The game relies on mouse interaction. Ensure your console window is active.

| Action | Input | Description |
| --- | --- | --- |
| **Reveal** | **Left Click** | Dig a cell. If it's a mine, Game Over. |
| **Flag/Unflag** | **Right Click** | Place a Flag (🚩) on a suspected mine. |
| **Chord** | **Double Left Click** | If an open number has the correct amount of flags around it, double-click it to open all remaining neighbors instantly. |
| **Menu Selection** | **Keyboard 1-3** | Select difficulty in the main menu. |
| **Quit** | **Keyboard Q** | Exit the game from the menu or during gameplay (if programmed). |

### 📁 Project Structure

```
MineSweeper/
├── include/                # Header files
│   ├── cgt.h              # CGT library interface (cross-platform console graphic tools)
│   └── game.h             # Game core logic interface
├── src/                    # Source files
│   ├── cgt_windows.cpp    # CGT Windows platform implementation
│   ├── cgt_apple.cpp      # CGT macOS platform implementation
│   ├── cgt_linux.cpp      # CGT Linux platform implementation
│   └── game.cpp           # Game core logic implementation
├── main.cpp               # Program entry point
├── Makefile               # Build configuration
└── README.md              # Project documentation
```

### 🛠️ Compilation & Running

**Prerequisites:**

* Windows or macOS or Linux.
* A C++ Compiler (GCC, Clang, etc.).

**Compiling with Makefile:**
Ensure all files (`main.cpp`, `game.cpp`, `cgt_windows.cpp`/`cgt_apple.cpp`, `cgt.h`, `game.h`) are in the same directory.

```bash
make
```
The Makefile will automatically read the compilation instructions and generate an executable file `minesweeper.exe`.

> **Note:** 
- Do not use "Quick Edit Mode" in your terminal if possible, although the code attempts to disable it automatically to prevent mouse input conflicts.
- If you encounter garbled text, please switch the encoding format. Linux and macOS: UTF-8; Windows: GBK.

### 🚀 Future Plans

To further enhance the gaming experience and code quality, the following features are planned for future releases:

* [x] **First-Click Protection**: Ensure that the user's first click is guaranteed not to be a mine.
* [x] **First-Click Safe Zone**: Guarantee that the first click and its 8 surrounding neighbors are all mine-free, ensuring an open starting area.
* [x] **Custom Difficulty**: Allow users to customize the grid size (rows and columns) and the total number of mines according to their preference.
* [x] **Linux Support**: Port the game to Linux, ensuring cross-platform compatibility.
* [x] **Timer Function**: Real-time recording of game duration to add a competitive element. 

---

### 📜 Author & Credits

* **Author:** Main logic by *GZQ* from *Shanghai Jiao Tong University*.
* **CGT Library:** Originally by *gty*, macOS adapted by *JYQ* with Gemini, Linux adapted by *GZQ* with Gemini, learnt from *Shen Jian at Tongji University*.

