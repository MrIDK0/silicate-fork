#pragma once

#include <string>

#include <winuser.h>

constexpr int KEY_MW_UP = 0x97;
constexpr int KEY_MW_DOWN = 0x98;

inline bool isMouseButton(int key) {
    return key == VK_LBUTTON || key == VK_RBUTTON || key == VK_MBUTTON ||
           key == VK_XBUTTON1 || key == VK_XBUTTON2;
}

inline std::string vkToString(int key) {
    switch (key) {
        case VK_SPACE:
            return "Space";
        case VK_RETURN:
            return "Enter";
        case VK_ESCAPE:
            return "Escape";
        case VK_TAB:
            return "Tab";
        case VK_BACK:
            return "Backspace";
        case VK_DELETE:
            return "Delete";
        case VK_INSERT:
            return "Insert";
        case VK_HOME:
            return "Home";
        case VK_END:
            return "End";
        case VK_PRIOR:
            return "Page Up";
        case VK_NEXT:
            return "Page Down";
        case VK_LEFT:
            return "Left";
        case VK_RIGHT:
            return "Right";
        case VK_UP:
            return "Up";
        case VK_DOWN:
            return "Down";
        case VK_SHIFT:
            return "Shift";
        case VK_CONTROL:
            return "Ctrl";
        case VK_MENU:
            return "Alt";
        case KEY_MW_UP:
            return "Mouse Wheel Up";
        case KEY_MW_DOWN:
            return "Mouse Wheel Down";
        default:
            if (key >= 'A' && key <= 'Z') {
                return std::string(1, static_cast<char>(key));
            }
            if (key >= '0' && key <= '9') {
                return std::string(1, static_cast<char>(key));
            }
            if (key >= VK_F1 && key <= VK_F12) {
                return "F" + std::to_string(key - VK_F1 + 1);
            }
            if (key >= VK_NUMPAD0 && key <= VK_NUMPAD9) {
                return "Numpad " + std::to_string(key - VK_NUMPAD0);
            }
            return "Key " + std::to_string(key);
    }
}

inline std::string formatKeybind(int key, int modifiers) {
    std::string result;

    if (modifiers & 0b010) {
        result += "Ctrl";
    }
    if (modifiers & 0b001) {
        if (!result.empty()) result += " + ";
        result += "Shift";
    }
    if (modifiers & 0b100) {
        if (!result.empty()) result += " + ";
        result += "Alt";
    }
    if (!result.empty()) result += " + ";
    result += vkToString(key);

    return result;
}
