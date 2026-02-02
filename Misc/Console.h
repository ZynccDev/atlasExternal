#pragma once
#include <string>
#include <iostream>
#include <thread>
#include <sstream>
#include <iomanip>
#include "Colours.h"
#include "../Utilities/KeyAuth/skStr.h"

void print_line(const std::string& line, int time) // this prints a line character by character so it looks good
{
    for (char c : line) {
        std::cout << c << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(time));
    }
    std::cout << std::endl;
}

void log(const std::string& line, int type)
{
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= 0x0004; // ENABLE_VIRTUAL_TERMINAL_PROCESSING
    SetConsoleMode(hOut, dwMode);

    std::ostringstream msg;

    switch (type)
    {
    case 0: // info
        msg << WHITE << "[" << INFO_BLUE << skCrypt("*").decrypt() << WHITE << "] " << INFO_BLUE << line;
        break;
    case 1: // success
        msg << WHITE << "[" << SUCCESS_GREEN << skCrypt("+").decrypt() << WHITE << "] " << SUCCESS_GREEN << line;
        break;
    case 2: // error
        msg << WHITE << "[" << ERROR_RED << skCrypt("-").decrypt() << WHITE << "] " << ERROR_RED << line;
        break;
    case 3: // warning/injection
        msg << WHITE << "[" << WARNING_YELLOW << skCrypt(">").decrypt() << WHITE << "] " << WARNING_YELLOW << line;
        break;
    default:
        msg << line;
        break;
    }

    print_line(msg.str(), 0);
}

void setup_console_window()
{
    HWND console = GetConsoleWindow();
    if (console != NULL)
    {
        SetWindowPos(console, 0, 100, 100, 900, 600, SWP_SHOWWINDOW);
    }
    
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD bufferSize = {120, 40};
    SetConsoleScreenBufferSize(hOut, bufferSize);
    
    SetConsoleTitleA("Atlas");
}

template<typename T>
std::string toHexString(T value, bool prefix = false, bool uppercase = false) // this is because we can't do std::hex in custom
{                                                                             // print let me know if u have a better way
    std::stringstream stream;
    if (uppercase)
        stream << std::uppercase;

    if (prefix)
        stream << "0x";

    stream << std::hex << value;
    return stream.str();
}
