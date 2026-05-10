namespace PrismaEngine;

/// <summary>
/// 按键码（与 C++ IInputDriver KeyCode 一致 = SDL3 扫描码）。
/// </summary>
public enum KeyCode
{
    // 字母键 (SDL3 scancode)
    A = 4, B = 5, C = 6, D = 7, E = 8, F = 9, G = 10,
    H = 11, I = 12, J = 13, K = 14, L = 15, M = 16,
    N = 17, O = 18, P = 19, Q = 20, R = 21,
    S = 22, T = 23, U = 24, V = 25, W = 26, X = 27, Y = 28, Z = 29,

    // 数字键
    Num0 = 39, Num1 = 30, Num2 = 31, Num3 = 32, Num4 = 33,
    Num5 = 34, Num6 = 35, Num7 = 36, Num8 = 37, Num9 = 38,

    // 方向键
    Up    = 82, Down = 81, Left = 80, Right = 79,

    // 控制键
    Escape = 41, Enter = 40, Tab = 43,
    Space = 44, Backspace = 42, Delete = 76,

    // 修饰键
    LShift = 225, RShift = 229,
    LCtrl  = 224, RCtrl  = 228,
    LAlt   = 226, RAlt   = 230,

    // 功能键
    F1 = 58, F2 = 59, F3 = 60, F4 = 61, F5 = 62,
    F6 = 63, F7 = 64, F8 = 65, F9 = 66, F10 = 67,
    F11 = 68, F12 = 69,
}
