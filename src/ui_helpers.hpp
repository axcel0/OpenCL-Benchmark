#pragma once
#include "utilities.hpp"

// ============================================================================
// UI Helper Functions - Clean, Robust, Best Practice
// ============================================================================

namespace ui {

// Box drawing characters using proper UTF-8 strings
inline const char* BOX_H = "─";  // Horizontal line
inline const char* BOX_V = "│";  // Vertical line
inline const char* BOX_TL = "┌"; // Top-left corner
inline const char* BOX_TR = "┐"; // Top-right corner
inline const char* BOX_BL = "└"; // Bottom-left corner
inline const char* BOX_BR = "┘"; // Bottom-right corner
inline const char* BOX_VR = "├"; // Vertical-right
inline const char* BOX_VL = "┤"; // Vertical-left
inline const char* BOX_HU = "┴"; // Horizontal-up
inline const char* BOX_HD = "┬"; // Horizontal-down
inline const char* BOX_X  = "┼"; // Cross

// Block characters for progress bars and charts
inline const char* BLOCK_FULL = "█";
inline const char* BLOCK_7_8 = "▉";
inline const char* BLOCK_3_4 = "▊";
inline const char* BLOCK_5_8 = "▋";
inline const char* BLOCK_1_2 = "▌";
inline const char* BLOCK_3_8 = "▍";
inline const char* BLOCK_1_4 = "▎";
inline const char* BLOCK_1_8 = "▏";
inline const char* BLOCK_LIGHT = "░";
inline const char* BLOCK_MEDIUM = "▒";
inline const char* BLOCK_DARK = "▓";

// Clear screen in a cross-platform, robust way
inline void clear_screen() noexcept {
#ifdef _WIN32
    [[maybe_unused]] int result = system("cls");
#else
    // Use ANSI escape codes for better portability
    print("\033[2J\033[H");
    // Fallback to system command if needed
    [[maybe_unused]] int result = system("clear");
#endif
}

// Draw a horizontal line with specified width
inline string draw_line(uint width, const char* style = BOX_H) noexcept {
    string line;
    line.reserve(width * 3); // UTF-8 can be 3 bytes per char
    for(uint i = 0; i < width; i++) {
        line += style;
    }
    return line;
}

// Draw a box header
inline void draw_box_header(const string& title, uint width = 78) noexcept {
    println(string(BOX_TL) + draw_line(width, BOX_H) + BOX_TR);
    const uint padding = (width - length(title)) / 2;
    println(string(BOX_V) + string(padding, ' ') + title + string(width - padding - length(title), ' ') + BOX_V);
    println(string(BOX_VR) + draw_line(width, BOX_H) + BOX_VL);
}

// Draw a box separator
inline void draw_box_separator(uint width = 78) noexcept {
    println(string(BOX_VR) + draw_line(width, BOX_H) + BOX_VL);
}

// Draw a box footer
inline void draw_box_footer(uint width = 78) noexcept {
    println(string(BOX_BL) + draw_line(width, BOX_H) + BOX_BR);
}

// Draw a simple box line with content
inline void draw_box_line(const string& content, uint width = 78) noexcept {
    const uint content_len = length(content);
    if(content_len < width) {
        println(string(BOX_V) + " " + content + string(width - content_len - 1, ' ') + BOX_V);
    } else {
        println(string(BOX_V) + " " + content.substr(0, width - 2) + BOX_V);
    }
}

// Draw a progress bar with percentage
inline string draw_progress_bar(float percent, uint width = 40) noexcept {
    percent = fmin(100.0f, fmax(0.0f, percent)); // Clamp to [0, 100]
    const uint filled = static_cast<uint>((percent / 100.0f) * width);
    const uint empty = width - filled;
    
    string bar = "[";
    bar += string(ANSI_BRIGHT_CYAN);
    for(uint i = 0; i < filled; i++) bar += BLOCK_FULL;
    bar += string(ANSI_RESET);
    for(uint i = 0; i < empty; i++) bar += BLOCK_LIGHT;
    bar += "]";
    
    return bar;
}

// Draw a horizontal bar chart
inline string draw_bar_chart(float value, float max_value, uint width = 30, 
                             const string& color = ANSI_BRIGHT_CYAN) noexcept {
    if(max_value <= 0.0f) max_value = 1.0f;
    const float ratio = fmin(1.0f, fmax(0.0f, value / max_value));
    const uint filled = static_cast<uint>(ratio * width);
    
    string bar = color;
    for(uint i = 0; i < filled; i++) bar += BLOCK_DARK;
    bar += ANSI_RESET;
    for(uint i = filled; i < width; i++) bar += BLOCK_LIGHT;
    
    return bar;
}

// Format time duration (seconds to human readable)
inline string format_duration(double seconds) noexcept {
    if(seconds < 60.0) {
        return to_string(static_cast<uint>(seconds)) + "s";
    } else if(seconds < 3600.0) {
        const uint mins = static_cast<uint>(seconds / 60.0);
        const uint secs = static_cast<uint>(seconds) % 60;
        return to_string(mins) + "m " + to_string(secs) + "s";
    } else {
        const uint hours = static_cast<uint>(seconds / 3600.0);
        const uint mins = (static_cast<uint>(seconds) % 3600) / 60;
        return to_string(hours) + "h " + to_string(mins) + "m";
    }
}

// Format large numbers with K/M/B suffixes
inline string format_number(double num) noexcept {
    if(num < 1000.0) {
        return to_string(num, 1u);
    } else if(num < 1000000.0) {
        return to_string(num / 1000.0, 2u) + "K";
    } else if(num < 1000000000.0) {
        return to_string(num / 1000000.0, 2u) + "M";
    } else {
        return to_string(num / 1000000000.0, 2u) + "B";
    }
}

// Get color based on performance level (0-100%)
inline string get_performance_color(float percent) noexcept {
    if(percent >= 80.0f) return ANSI_BRIGHT_GREEN;
    if(percent >= 60.0f) return ANSI_GREEN;
    if(percent >= 40.0f) return ANSI_YELLOW;
    if(percent >= 20.0f) return ANSI_BRIGHT_RED;
    return ANSI_RED;
}

// Draw a metric with bar chart
inline void draw_metric_bar(const string& label, float value, const string& unit,
                           float max_value, uint label_width = 25) noexcept {
    const string padded_label = alignr(label_width, label);
    const string value_str = alignr(8u, to_string(value, 2u));
    const string bar = draw_bar_chart(value, max_value, 30);
    const float percent = (max_value > 0) ? (value / max_value * 100.0f) : 0.0f;
    const string color = get_performance_color(percent);
    
    draw_box_line(padded_label + " " + color + value_str + " " + unit + ANSI_RESET + "  " + bar);
}

// Spinner animation for long operations
class Spinner {
private:
    const char* frames[4] = {"⠋", "⠙", "⠹", "⠸"};
    uint current_frame = 0;
    
public:
    void update() noexcept {
        print("\r" + string(frames[current_frame]) + " ");
        current_frame = (current_frame + 1) % 4;
    }
    
    void clear() noexcept {
        print("\r  \r");
    }
};

// Wait for user input with proper message
inline void wait_for_input(const string& message = "Press Enter to continue...") noexcept {
    println("\n" + string(ANSI_GREEN) + message + string(ANSI_RESET));
    wait();
}

} // namespace ui
