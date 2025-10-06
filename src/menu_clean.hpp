#pragma once
#include "ui_helpers.hpp"
#include <vector>

#ifdef _WIN32
    #include <conio.h>
#else
    #include <termios.h>
    #include <unistd.h>
#endif

// ============================================================================
// Interactive Menu System - Clean, Robust, Best Practice
// ============================================================================

namespace ui {

// Cross-platform getch() implementation
class KeyboardInput {
private:
#ifndef _WIN32
    struct termios old_tio{}, new_tio{};
    bool term_initialized = false;
    
    void init_terminal() noexcept {
        if(!term_initialized) {
            tcgetattr(STDIN_FILENO, &old_tio);
            new_tio = old_tio;
            new_tio.c_lflag &= ~(ICANON | ECHO);
            tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);
            term_initialized = true;
        }
    }
    
    void restore_terminal() noexcept {
        if(term_initialized) {
            tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);
            term_initialized = false;
        }
    }
#endif

public:
    KeyboardInput() noexcept {
#ifndef _WIN32
        init_terminal();
#endif
    }
    
    ~KeyboardInput() noexcept {
#ifndef _WIN32
        restore_terminal();
#endif
    }
    
    // Get a single character without echo
    [[nodiscard]] int getch() noexcept {
#ifdef _WIN32
        return _getch();
#else
        return getchar();
#endif
    }
};

// Interactive menu with keyboard navigation
class InteractiveMenu {
private:
    string title;
    vector<string> options;
    uint selected = 0;
    KeyboardInput keyboard;
    
    void draw() noexcept {
        clear_screen();
        
        // Draw header
        draw_box_header(title);
        draw_box_line("");
        
        // Draw menu options
        for(uint i = 0; i < static_cast<uint>(options.size()); i++) {
            const bool is_selected = (i == selected);
            string line;
            
            if(is_selected) {
                line = string(ANSI_BRIGHT_CYAN) + "  ▶ " + options[i] + string(ANSI_RESET);
            } else {
                line = "    " + options[i];
            }
            
            draw_box_line(line);
        }
        
        // Draw footer
        draw_box_line("");
        draw_box_line(string(ANSI_YELLOW) + "Navigation: ↑/↓ or w/s/k/j | Select: Enter/Space | Quit: q/ESC" + string(ANSI_RESET));
        draw_box_footer();
    }
    
public:
    InteractiveMenu(const string& menu_title, const vector<string>& menu_options) noexcept
        : title(menu_title), options(menu_options) {}
    
    // Show menu and return selected index (-1 for quit)
    [[nodiscard]] int show() noexcept {
        if(options.empty()) return -1;
        
        while(true) {
            draw();
            
            const int key = keyboard.getch();
            
            // Handle arrow keys (escape sequences)
            if(key == 27) { // ESC
                const int next = keyboard.getch();
                if(next == '[') {
                    const int arrow = keyboard.getch();
                    if(arrow == 'A') { // Up arrow
                        if(selected > 0) selected--;
                    } else if(arrow == 'B') { // Down arrow
                        if(selected < static_cast<uint>(options.size()) - 1) selected++;
                    }
                } else {
                    return -1; // ESC pressed alone
                }
            }
            // Handle letter keys
            else if(key == 'w' || key == 'k' || key == 'W' || key == 'K') { // Up
                if(selected > 0) selected--;
            }
            else if(key == 's' || key == 'j' || key == 'S' || key == 'J') { // Down
                if(selected < static_cast<uint>(options.size()) - 1) selected++;
            }
            // Handle number keys (1-9)
            else if(key >= '1' && key <= '9') {
                const uint num = static_cast<uint>(key - '1');
                if(num < static_cast<uint>(options.size())) {
                    return static_cast<int>(num);
                }
            }
            // Handle zero key
            else if(key == '0') {
                const uint num = 9; // 0 represents the 10th option
                if(num < static_cast<uint>(options.size())) {
                    return static_cast<int>(num);
                }
            }
            // Handle Enter, Space
            else if(key == '\n' || key == '\r' || key == ' ') {
                return static_cast<int>(selected);
            }
            // Handle quit
            else if(key == 'q' || key == 'Q') {
                return -1;
            }
        }
    }
    
    // Simple confirmation dialog
    [[nodiscard]] static bool confirm(const string& message) noexcept {
        KeyboardInput kb;
        clear_screen();
        draw_box_header("Confirmation");
        draw_box_line("");
        draw_box_line(message);
        draw_box_line("");
        draw_box_line(string(ANSI_YELLOW) + "[Y] Yes    [N] No" + string(ANSI_RESET));
        draw_box_footer();
        
        while(true) {
            const int key = kb.getch();
            if(key == 'y' || key == 'Y') return true;
            if(key == 'n' || key == 'N' || key == 27) return false;
        }
    }
};

// Simple function to show menu and get selection
[[nodiscard]] inline int show_menu(const string& title, const vector<string>& options) noexcept {
    InteractiveMenu menu(title, options);
    return menu.show();
}

} // namespace ui
