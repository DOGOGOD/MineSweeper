/*
    Console Graphic Tools on Windows
    
    by gty

    Learnt from Shen Jian at Tongji University
*/


#ifdef _WIN32

#include "./cgt.h"

#include <Windows.h>
#include <iostream>
#include <vector>


using namespace std;


static void* hInput = nullptr;
static void* hOutput = nullptr;

static unsigned long originalConsoleMode = 0;

static vector<INPUT_RECORD> inputBuffer;


// Handler function to catch Ctrl+C
static BOOL WINAPI __cgt_ctrl_handler(DWORD dwCtrlType) {
    switch (dwCtrlType) {
        case CTRL_C_EVENT:     // Handle Ctrl+C
        case CTRL_CLOSE_EVENT: // Handle console window close button
            cgt_close();
            return FALSE; // Pass to default handler to terminate process
        default:
            return FALSE; // Pass on other events
    }
}


void cgt_init() {
    if (hInput || hOutput) {
        return;
    }

    hInput = GetStdHandle(STD_INPUT_HANDLE);
    hOutput = GetStdHandle(STD_OUTPUT_HANDLE);

    // clear clipboard
    EmptyClipboard();

    GetConsoleMode(hInput, &originalConsoleMode);
    DWORD newConsoleMode = originalConsoleMode;
    newConsoleMode |= ENABLE_EXTENDED_FLAGS | ENABLE_MOUSE_INPUT;
    newConsoleMode &= ~ENABLE_QUICK_EDIT_MODE;

    SetConsoleMode(hInput, newConsoleMode);

    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hOutput, &cursorInfo);
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(hOutput, &cursorInfo);

    // Register our control handler
    SetConsoleCtrlHandler(__cgt_ctrl_handler, TRUE);
}


void cgt_close() {
    if (!hInput || !hOutput) {
        return;
    }
    
    // reset color
    cgt_reset_color();

    SetConsoleMode(hInput, originalConsoleMode);

    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hOutput, &cursorInfo);
    cursorInfo.bVisible = TRUE;
    SetConsoleCursorInfo(hOutput, &cursorInfo);

    // Unregister our control handler
    SetConsoleCtrlHandler(__cgt_ctrl_handler, FALSE);

    // Set handles to null to make this function idempotent
    // (safe to call more than once)
    hInput = nullptr;
    hOutput = nullptr;
}


void cgt_msleep(int milliseconds) {
    Sleep(milliseconds);
}


void cgt_clear_screen(int color) {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(hOutput, &csbi)) {
        // fallback
        cout << "\033[2J\033[H";
        return;
    }
    
    csbi.wAttributes = (csbi.wAttributes & 0x0F) | ((color & 0x0F) << 4);
    SetConsoleTextAttribute(hOutput, csbi.wAttributes);
    
    DWORD consoleSize = csbi.dwSize.X * csbi.dwSize.Y;
    DWORD charsWritten;
    COORD homeCoords = { 0, 0 };
    FillConsoleOutputCharacter(hOutput, ' ', consoleSize, homeCoords, &charsWritten);
    FillConsoleOutputAttribute(hOutput, csbi.wAttributes, consoleSize, homeCoords, &charsWritten);
    SetConsoleCursorPosition(hOutput, homeCoords);
    
    if (color != -1) {
        // reset color
        csbi.wAttributes = (csbi.wAttributes & 0xF0) | (csbi.wAttributes & 0x0F);
        SetConsoleTextAttribute(hOutput, csbi.wAttributes);
    }
}


static void __cgt_read_input_to_buffer() {
    if (!hInput) {
        return;
    }

    DWORD eventsAvailable = 0;
    GetNumberOfConsoleInputEvents(hInput, &eventsAvailable);
    if (eventsAvailable == 0) {
        return;
    }

    vector<INPUT_RECORD> records(eventsAvailable);
    DWORD eventsRead = 0;
    ReadConsoleInput(hInput, records.data(), eventsAvailable, &eventsRead);
    records.resize(eventsRead);
    for (const INPUT_RECORD& record : records) {
        if (record.EventType != KEY_EVENT && record.EventType != MOUSE_EVENT) {
            continue;
        }

        if (record.EventType == KEY_EVENT) {
            if (!record.Event.KeyEvent.bKeyDown) {
                continue;
            }
        }

        inputBuffer.push_back(record);
    }
}


bool cgt_has_mouse() {
    __cgt_read_input_to_buffer();
    for (const INPUT_RECORD& record : inputBuffer) {
        if (record.EventType == MOUSE_EVENT) {
            return true;
        }
    }

    return false;
}


void cgt_get_mouse(int& x, int& y, int& button, int& event) {
    for (int i = 0; i < (int) inputBuffer.size(); i++) {
        const INPUT_RECORD& record = inputBuffer[i];
        if (record.EventType != MOUSE_EVENT) {
            continue;
        }

        x = record.Event.MouseEvent.dwMousePosition.X;
        y = record.Event.MouseEvent.dwMousePosition.Y;

        if (record.Event.MouseEvent.dwEventFlags & MOUSE_MOVED) {
            button = 0;
            event = MOUSE_MOVE;
        }
        else {
            if (record.Event.MouseEvent.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED) {
                button = MOUSE_BUTTON_LEFT;
                event = MOUSE_CLICK;
            }
            else if (record.Event.MouseEvent.dwButtonState & RIGHTMOST_BUTTON_PRESSED) {
                button = MOUSE_BUTTON_RIGHT;
                event = MOUSE_CLICK;
            }
            else if (record.Event.MouseEvent.dwButtonState & FROM_LEFT_2ND_BUTTON_PRESSED) {
                button = MOUSE_BUTTON_MIDDLE;
                event = MOUSE_CLICK;
            }
            else {
                button = 0;
                event = MOUSE_RELEASE;
            }

            if (record.Event.MouseEvent.dwEventFlags & DOUBLE_CLICK) {
                event = MOUSE_DOUBLECLICK;
            }
        }

        inputBuffer.erase(inputBuffer.begin() + i);
        return;
    }
}


bool cgt_has_key() {
    __cgt_read_input_to_buffer();
    for (const INPUT_RECORD& record : inputBuffer) {
        if (record.EventType == KEY_EVENT) {
            return true;
        }
    }
    return false;
}


void cgt_get_key(char& ch) {
    for (int i = 0; i < (int) inputBuffer.size(); i++) {
        const INPUT_RECORD& record = inputBuffer[i];
        if (record.EventType != KEY_EVENT) {
            continue;
        }

        if (record.Event.KeyEvent.bKeyDown) {
            ch = record.Event.KeyEvent.uChar.AsciiChar;

            if (ch == '\r') {
                ch = '\n';
            }

            inputBuffer.erase(inputBuffer.begin() + i);
            return;
        }
    }
}


void cgt_set_color(int foreground, int background) {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(hOutput, &csbi)) {
        return;
    }

    csbi.wAttributes = (csbi.wAttributes & 0xF0) | (foreground & 0x0F);
    csbi.wAttributes = (csbi.wAttributes & 0x0F) | ((background & 0x0F) << 4);
    
    SetConsoleTextAttribute(hOutput, csbi.wAttributes);
}


void cgt_reset_color() {
    cgt_set_color();
}


void cgt_getxy(int &x, int &y) {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(hOutput, &csbi)) {
        cout << "CGT Internal Error!";
        exit(-1);
    }

    x = csbi.dwCursorPosition.X;
    y = csbi.dwCursorPosition.Y;
}


void cgt_gotoxy(int x, int y) {
    COORD coord;
    
    int currentX, currentY;
    cgt_getxy(currentX, currentY);
    
    coord.X = (short) (x == -1 ? currentX : x);
    coord.Y = (short) (y == -1 ? currentY : y);

    SetConsoleCursorPosition(hOutput, coord);
}


void cgt_print_str(const char* str, int x, int y, int foreground, int background) {
    cgt_set_color(foreground, background);
    cgt_gotoxy(x, y);

    cout << str;
}


void cgt_print_char(char ch, int x, int y, int foreground, int background) {
    cgt_set_color(foreground, background);
    cgt_gotoxy(x, y);

    cout << ch;
}


void cgt_print_int(int num, int x, int y, int foreground, int background) {
    cgt_set_color(foreground, background);
    cgt_gotoxy(x, y);

    cout << num;
}


void cgt_print_double(double num, int x, int y, int foreground, int background) {
    cgt_set_color(foreground, background);
    cgt_gotoxy(x, y);

    cout << num;
}


#endif  // _WIN32
