#pragma once
#include "ui_helpers.hpp"
#include "json_export.hpp"
#include <cmath>

// ============================================================================
// Performance Score System - 3DMark Style
// ============================================================================

namespace scoring {

// Base scores for normalization (RTX 4090 as reference at 100%)
struct ReferenceScores {
    static constexpr float FP32_REF = 82.5f;      // TFLOPs
    static constexpr float FP64_REF = 1.3f;       // TFLOPs
    static constexpr float FP16_REF = 165.0f;     // TFLOPs
    static constexpr float INT32_REF = 82.5f;     // TIOPs
    static constexpr float MEM_BW_REF = 1000.0f;  // GB/s
    static constexpr float PCIE_BW_REF = 24.0f;   // GB/s
};

// Calculate individual component scores (0-100+ scale)
struct ComponentScores {
    float compute_score = 0.0f;      // FP32/FP64/FP16/INT combined
    float memory_score = 0.0f;       // Memory bandwidth
    float transfer_score = 0.0f;     // PCIe bandwidth
    float overall_score = 0.0f;      // Weighted average
    
    // Sub-scores
    float fp32_score = 0.0f;
    float fp64_score = 0.0f;
    float fp16_score = 0.0f;
    float int32_score = 0.0f;
    float mem_coalesced_score = 0.0f;
    float mem_misaligned_score = 0.0f;
    float pcie_score = 0.0f;
};

// Calculate scores from benchmark results
[[nodiscard]] inline ComponentScores calculate_scores(const BenchmarkResults& results) noexcept {
    ComponentScores scores;
    
    // Compute scores (normalized to reference)
    scores.fp32_score = (results.fp32_tflops / ReferenceScores::FP32_REF) * 100.0f;
    
    if(results.is_fp64_capable) {
        scores.fp64_score = (results.fp64_tflops / ReferenceScores::FP64_REF) * 100.0f;
    }
    
    if(results.is_fp16_capable) {
        scores.fp16_score = (results.fp16_tflops / ReferenceScores::FP16_REF) * 100.0f;
    }
    
    scores.int32_score = (results.int32_tiops / ReferenceScores::INT32_REF) * 100.0f;
    
    // Weighted compute score (FP32 most important)
    float compute_weights_sum = 0.5f; // FP32 weight
    scores.compute_score = scores.fp32_score * 0.5f;
    
    if(results.is_fp64_capable) {
        scores.compute_score += scores.fp64_score * 0.2f;
        compute_weights_sum += 0.2f;
    }
    if(results.is_fp16_capable) {
        scores.compute_score += scores.fp16_score * 0.2f;
        compute_weights_sum += 0.2f;
    }
    scores.compute_score += scores.int32_score * 0.1f;
    compute_weights_sum += 0.1f;
    
    scores.compute_score /= compute_weights_sum;
    
    // Memory scores
    scores.mem_coalesced_score = (results.mem_bw_coalesced_read / ReferenceScores::MEM_BW_REF) * 100.0f;
    scores.mem_misaligned_score = (results.mem_bw_misaligned_read / ReferenceScores::MEM_BW_REF) * 100.0f;
    scores.memory_score = (scores.mem_coalesced_score * 0.7f + scores.mem_misaligned_score * 0.3f);
    
    // PCIe score (if applicable)
    if(!results.uses_ram) {
        const float avg_pcie = (results.pcie_bw_send + results.pcie_bw_receive + results.pcie_bw_bidirectional) / 3.0f;
        scores.pcie_score = (avg_pcie / ReferenceScores::PCIE_BW_REF) * 100.0f;
        scores.transfer_score = scores.pcie_score;
    } else {
        scores.transfer_score = 100.0f; // CPU uses RAM, perfect score
    }
    
    // Overall score (weighted average)
    // Compute: 50%, Memory: 35%, Transfer: 15%
    scores.overall_score = scores.compute_score * 0.50f 
                         + scores.memory_score * 0.35f 
                         + scores.transfer_score * 0.15f;
    
    return scores;
}

// Get performance grade (S, A, B, C, D, F)
[[nodiscard]] inline string get_grade(float score) noexcept {
    if(score >= 150.0f) return string(ANSI_BRIGHT_CYAN) + "S+" + string(ANSI_RESET);
    if(score >= 130.0f) return string(ANSI_BRIGHT_CYAN) + "S" + string(ANSI_RESET);
    if(score >= 110.0f) return string(ANSI_BRIGHT_GREEN) + "A+" + string(ANSI_RESET);
    if(score >= 90.0f)  return string(ANSI_BRIGHT_GREEN) + "A" + string(ANSI_RESET);
    if(score >= 75.0f)  return string(ANSI_GREEN) + "B+" + string(ANSI_RESET);
    if(score >= 60.0f)  return string(ANSI_GREEN) + "B" + string(ANSI_RESET);
    if(score >= 45.0f)  return string(ANSI_YELLOW) + "C+" + string(ANSI_RESET);
    if(score >= 30.0f)  return string(ANSI_YELLOW) + "C" + string(ANSI_RESET);
    if(score >= 15.0f)  return string(ANSI_RED) + "D" + string(ANSI_RESET);
    return string(ANSI_BRIGHT_RED) + "F" + string(ANSI_RESET);
}

// Get performance tier description
[[nodiscard]] inline string get_tier_description(float score) noexcept {
    if(score >= 150.0f) return "Extreme Performance";
    if(score >= 130.0f) return "Enthusiast Grade";
    if(score >= 110.0f) return "High Performance";
    if(score >= 90.0f)  return "Performance";
    if(score >= 75.0f)  return "Mainstream Plus";
    if(score >= 60.0f)  return "Mainstream";
    if(score >= 45.0f)  return "Entry Level Plus";
    if(score >= 30.0f)  return "Entry Level";
    if(score >= 15.0f)  return "Low Performance";
    return "Very Low Performance";
}

// Draw 3DMark-style results summary
inline void draw_results_summary(const BenchmarkResults& results, const ComponentScores& scores) noexcept {
    using namespace ui;
    
    clear_screen();
    
    // Header with overall score
    println("");
    println(string(ANSI_BRIGHT_CYAN) + string(40, '=') + string(ANSI_RESET));
    println(string(ANSI_BRIGHT_CYAN) + "  OpenCL Benchmark - Results Summary  " + string(ANSI_RESET));
    println(string(ANSI_BRIGHT_CYAN) + string(40, '=') + string(ANSI_RESET));
    println("");
    
    // Overall Score - Big and prominent
    const string score_str = to_string(static_cast<uint>(scores.overall_score));
    println(string(ANSI_BOLD) + "          OVERALL SCORE" + string(ANSI_RESET));
    println("");
    println("             " + string(ANSI_BRIGHT_GREEN) + string(ANSI_BOLD) + score_str + string(ANSI_RESET));
    println("");
    println("          Grade: " + get_grade(scores.overall_score) + "  (" + get_tier_description(scores.overall_score) + ")");
    println("");
    
    // Device info
    draw_box_separator(76);
    draw_box_line(string(ANSI_BOLD) + results.device_name + string(ANSI_RESET), 76);
    draw_box_line(results.device_vendor + " | " + results.driver_version, 76);
    draw_box_line(to_string(results.compute_units) + " CUs @ " + to_string(results.clock_frequency) + " MHz | " + 
                  to_string(results.memory_mb) + " MB VRAM", 76);
    draw_box_separator(76);
    println("");
    
    // Component scores with bars
    println(string(ANSI_BOLD) + "  Component Breakdown:" + string(ANSI_RESET));
    println("");
    
    const float max_score = fmax(150.0f, fmax(scores.compute_score, fmax(scores.memory_score, scores.transfer_score)));
    
    draw_metric_bar("Compute Performance", scores.compute_score, "", max_score, 30);
    draw_metric_bar("  ├─ FP32", scores.fp32_score, "", max_score, 30);
    if(results.is_fp64_capable) {
        draw_metric_bar("  ├─ FP64", scores.fp64_score, "", max_score, 30);
    }
    if(results.is_fp16_capable) {
        draw_metric_bar("  ├─ FP16", scores.fp16_score, "", max_score, 30);
    }
    draw_metric_bar("  └─ INT32", scores.int32_score, "", max_score, 30);
    
    println("");
    draw_metric_bar("Memory Performance", scores.memory_score, "", max_score, 30);
    draw_metric_bar("  ├─ Coalesced", scores.mem_coalesced_score, "", max_score, 30);
    draw_metric_bar("  └─ Misaligned", scores.mem_misaligned_score, "", max_score, 30);
    
    if(!results.uses_ram) {
        println("");
        draw_metric_bar("Transfer Performance", scores.transfer_score, "", max_score, 30);
    }
    
    println("");
    draw_box_separator(76);
    
    // Performance comparison
    println("");
    println(string(ANSI_BOLD) + "  Performance vs Reference (RTX 4090 = 100%):" + string(ANSI_RESET));
    println("");
    
    const float vs_ref_percent = (scores.overall_score / 100.0f - 1.0f) * 100.0f;
    if(vs_ref_percent > 0) {
        println("  Your device is " + string(ANSI_BRIGHT_GREEN) + "+" + to_string(vs_ref_percent, 1u) + "%" + 
                string(ANSI_RESET) + " faster than reference");
    } else {
        println("  Your device is " + string(ANSI_YELLOW) + to_string(-vs_ref_percent, 1u) + "%" + 
                string(ANSI_RESET) + " slower than reference");
    }
    println("");
    
    // Footer
    draw_box_separator(76);
    println("");
    println(string(ANSI_GREEN) + "  Results saved to: benchmark_results_" + 
            replace(results.device_name, " ", "_") + ".json" + string(ANSI_RESET));
    println("");
}

} // namespace scoring
