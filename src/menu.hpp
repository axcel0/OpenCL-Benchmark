#pragma once

#include "utilities.hpp"
#include <cstdio>
#include <termios.h>
#include <unistd.h>

// Simple cross-platform menu system
class InteractiveMenu {
private:
    vector<string> options;
    string title;
    int selected_index;
    
    // Get single character input without Enter (Linux/macOS)
    #ifndef _WIN32
    char getch() {
        char buf = 0;
        struct termios old = {0};
        if (tcgetattr(0, &old) < 0) perror("tcsetattr()");
        old.c_lflag &= ~ICANON;
        old.c_lflag &= ~ECHO;
        old.c_cc[VMIN] = 1;
        old.c_cc[VTIME] = 0;
        if (tcsetattr(0, TCSANOW, &old) < 0) perror("tcsetattr ICANON");
        if (read(0, &buf, 1) < 0) perror("read()");
        old.c_lflag |= ICANON;
        old.c_lflag |= ECHO;
        if (tcsetattr(0, TCSADRAIN, &old) < 0) perror("tcsetattr ~ICANON");
        return buf;
    }
    #else
    #include <conio.h>
    char getch() {
        return _getch();
    }
    #endif
    
public:
    InteractiveMenu(const string& title, const vector<string>& options) 
        : title(title), options(options), selected_index(0) {}
    
    void draw() {
        // Clear screen
        #ifdef _WIN32
        system("cls");
        #else
        system("clear");
        #endif
        
        // Draw border and title
        println(string(ANSI_BRIGHT_CYAN));
        println("┌" + string(78, '─') + "┐");
        println("│" + alignr(79u, title) + "│");
        println("├" + string(78, '─') + "┤");
        println(string(ANSI_RESET));
        
        // Draw options
        for(uint i = 0; i < (uint)options.size(); i++) {
            if(i == (uint)selected_index) {
                println("│ " + string(ANSI_BRIGHT_GREEN) + "► " + options[i] + string(ANSI_RESET) + string(75u - length(options[i]), ' ') + "│");
            } else {
                println("│   " + options[i] + string(75u - length(options[i]), ' ') + "│");
            }
        }
        
        println(string(ANSI_BRIGHT_CYAN));
        println("├" + string(78, '─') + "┤");
        println("│ " + string(ANSI_YELLOW) + "↑/↓ Navigate  │  Enter Select  │  q Quit" + string(ANSI_RESET) + string(37u, ' ') + "│");
        println("└" + string(78, '─') + "┘");
        println(string(ANSI_RESET));
    }
    
    int run() {
        while(true) {
            draw();
            
            char c = getch();
            
            // Arrow keys on Linux send 3 characters: ESC [ A/B/C/D
            #ifndef _WIN32
            if(c == 27) { // ESC
                getch(); // '['
                c = getch(); // Direction
                if(c == 'A' || c == 'D') c = 'w'; // Up
                if(c == 'B' || c == 'C') c = 's'; // Down
            }
            #endif
            
            switch(c) {
                case 'w':
                case 'W':
                case 'k': // vim keys
                case 'K':
                #ifdef _WIN32
                case 72: // Up arrow on Windows
                #endif
                    selected_index--;
                    if(selected_index < 0) selected_index = (int)options.size() - 1;
                    break;
                    
                case 's':
                case 'S':
                case 'j': // vim keys
                case 'J':
                #ifdef _WIN32
                case 80: // Down arrow on Windows
                #endif
                    selected_index++;
                    if(selected_index >= (int)options.size()) selected_index = 0;
                    break;
                    
                case '\n':
                case '\r':
                #ifdef _WIN32
                case 13: // Enter on Windows
                #endif
                    return selected_index;
                    
                case 'q':
                case 'Q':
                case 27: // ESC
                    return -1; // Exit
                    
                default:
                    // Number key selection
                    if(c >= '1' && c <= '9') {
                        int idx = c - '1';
                        if(idx < (int)options.size()) {
                            return idx;
                        }
                    }
                    break;
            }
        }
    }
};

// Convenience function
inline int show_menu(const string& title, const vector<string>& options) {
    InteractiveMenu menu(title, options);
    return menu.run();
}

// Yes/No confirmation dialog
inline bool confirm(const string& message) {
    println("\n" + string(ANSI_YELLOW) + message + " (y/n): " + string(ANSI_RESET));
    
    #ifndef _WIN32
    char buf = 0;
    struct termios old = {0};
    if (tcgetattr(0, &old) < 0) perror("tcsetattr()");
    old.c_lflag &= ~ICANON;
    old.c_lflag &= ~ECHO;
    old.c_cc[VMIN] = 1;
    old.c_cc[VTIME] = 0;
    if (tcsetattr(0, TCSANOW, &old) < 0) perror("tcsetattr ICANON");
    if (read(0, &buf, 1) < 0) perror("read()");
    old.c_lflag |= ICANON;
    old.c_lflag |= ECHO;
    if (tcsetattr(0, TCSADRAIN, &old) < 0) perror("tcsetattr ~ICANON");
    char c = buf;
    #else
    char c = _getch();
    #endif
    
    println(string(1, c));
    return (c == 'y' || c == 'Y');
}
