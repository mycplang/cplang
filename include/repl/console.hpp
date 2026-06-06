// CP Language — 跨平台控制台 I/O 抽象层
// 在 Windows 上使用 conio + Console API，在 Linux/macOS 上使用 termios + ANSI
#pragma once

#ifdef _WIN32
  #ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
  #endif
  #include <windows.h>
  #include <conio.h>
#else
  #include <termios.h>
  #include <unistd.h>
  #include <fcntl.h>
  #include <sys/select.h>
#endif

namespace cplang {

// ═══════════════════════════════════════════════════════════════════
//  跨平台单字符输入（无回显、无缓冲）
// ═══════════════════════════════════════════════════════════════════

inline int console_getch() {
#ifdef _WIN32
    return _getch();
#else
    struct termios oldt, newt;
    int ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
#endif
}

// ═══════════════════════════════════════════════════════════════════
//  跨平台键盘缓冲检查（有按键可用时返回 true）
// ═══════════════════════════════════════════════════════════════════

inline bool console_kbhit() {
#ifdef _WIN32
    return _kbhit() != 0;
#else
    struct timeval tv = {0, 0};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0;
#endif
}

// ═══════════════════════════════════════════════════════════════════
//  跨平台控制台初始化 / 恢复
// ═══════════════════════════════════════════════════════════════════

struct ConsoleState {
#ifdef _WIN32
    void* hConsoleIn_ = nullptr;
    void* hConsoleOut_ = nullptr;
    int   oldConsoleMode_ = -1;
#endif
};

inline void console_init(ConsoleState& state) {
#ifdef _WIN32
    state.hConsoleIn_ = reinterpret_cast<void*>(GetStdHandle(STD_INPUT_HANDLE));
    state.hConsoleOut_ = reinterpret_cast<void*>(GetStdHandle(STD_OUTPUT_HANDLE));

    // 启用 ANSI 转义序列 (Windows 10 14393+)
    HANDLE hOut = reinterpret_cast<HANDLE>(state.hConsoleOut_);
    if (hOut != INVALID_HANDLE_VALUE && hOut != nullptr) {
        DWORD mode = 0;
        if (GetConsoleMode(hOut, &mode)) {
            state.oldConsoleMode_ = static_cast<int>(mode);
        }
    }

    // 启用 ANSI 转义序列处理
    HANDLE hIn = reinterpret_cast<HANDLE>(state.hConsoleIn_);
    if (hIn != INVALID_HANDLE_VALUE && hIn != nullptr) {
        DWORD mode = 0;
        GetConsoleMode(hIn, &mode);
        mode &= ~ENABLE_LINE_INPUT;
        mode &= ~ENABLE_ECHO_INPUT;
        mode |= ENABLE_VIRTUAL_TERMINAL_INPUT;
        SetConsoleMode(hIn, mode);
    }

    // 设置输出为 UTF-8
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#else
    // Linux/macOS: 终端默认支持 ANSI + UTF-8，无需额外初始化
    (void)state;
#endif
}

inline void console_restore(ConsoleState& state) {
#ifdef _WIN32
    HANDLE hIn = reinterpret_cast<HANDLE>(state.hConsoleIn_);
    if (hIn != INVALID_HANDLE_VALUE && hIn != nullptr && state.oldConsoleMode_ >= 0) {
        SetConsoleMode(hIn, static_cast<DWORD>(state.oldConsoleMode_));
    }
#else
    (void)state;
#endif
}

// ═══════════════════════════════════════════════════════════════════
//  跨平台控制台清屏
// ═══════════════════════════════════════════════════════════════════

inline void console_clear() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

} // namespace cplang
