// color_helpers.h - Color utilities for JD9165 (CORRECTED)
#pragma once

#include <stdint.h>
#include <Arduino.h>

// ============================================
// BGR COLOR DEFINITIONS (for JD9165 with Arduino_GFX settings)
// ============================================

// BGR565 color definitions
#define BGR_WHITE     0xFFFF
#define BGR_BLACK     0x0000
#define BGR_RED       0x001F    // In BGR mode, 0x001F = RED
#define BGR_GREEN     0x07E0    // GREEN is same in RGB/BGR
#define BGR_BLUE      0xF800    // In BGR mode, 0xF800 = BLUE
#define BGR_YELLOW    0x07FF    // GREEN + BLUE in BGR
#define BGR_MAGENTA   0xF81F    // BLUE + RED in BGR
#define BGR_CYAN      0xFFE0    // Actually YELLOW in BGR
#define BGR_GRAY      0x8410
#define BGR_DARKGRAY  0x2104
#define BGR_LIGHTGRAY 0xCE59

// RGB565 color definitions (for reference)
#define RGB_WHITE     0xFFFF
#define RGB_BLACK     0x0000
#define RGB_RED       0xF800    // Standard RED in RGB
#define RGB_GREEN     0x07E0    // GREEN
#define RGB_BLUE      0x001F    // Standard BLUE in RGB
#define RGB_YELLOW    0xFFE0    // RED + GREEN
#define RGB_MAGENTA   0xF81F    // RED + BLUE
#define RGB_CYAN      0x07FF    // GREEN + BLUE
#define RGB_GRAY      0x8410
#define RGB_DARKGRAY  0x2104
#define RGB_LIGHTGRAY 0xCE59

// Aliases for JD9165 (we use BGR mode)
#define COLOR_WHITE     BGR_WHITE
#define COLOR_BLACK     BGR_BLACK
#define COLOR_RED       BGR_RED
#define COLOR_GREEN     BGR_GREEN
#define COLOR_BLUE      BGR_BLUE
#define COLOR_YELLOW    BGR_YELLOW
#define COLOR_MAGENTA   BGR_MAGENTA
#define COLOR_CYAN      BGR_CYAN
#define COLOR_GRAY      BGR_GRAY
#define COLOR_DARKGRAY  BGR_DARKGRAY
#define COLOR_LIGHTGRAY BGR_LIGHTGRAY

// ============================================
// CONVERSION FUNCTIONS - USE DIFFERENT NAMES
// ============================================

// Convert RGB888 to BGR565 (for JD9165)
inline uint16_t rgb_to_bgr565(uint8_t r, uint8_t g, uint8_t b) {
    // RGB565 to BGR565 conversion
    return ((b & 0xF8) << 8) | ((g & 0xFC) << 3) | (r >> 3);
}

// Convert BGR565 to RGB888
inline void bgr565_to_rgb888(uint16_t bgr_color, uint8_t &r, uint8_t &g, uint8_t &b) {
    r = (bgr_color & 0x1F) << 3;
    g = (bgr_color & 0x7E0) >> 3;
    b = (bgr_color & 0xF800) >> 8;
}

// Convert RGB565 to RGB888
inline void rgb565_to_rgb888(uint16_t rgb_color, uint8_t &r, uint8_t &g, uint8_t &b) {
    r = (rgb_color & 0xF800) >> 8;
    g = (rgb_color & 0x07E0) >> 3;
    b = (rgb_color & 0x001F) << 3;
}

// Convert between RGB565 and BGR565
inline uint16_t rgb565_to_bgr565(uint16_t rgb_color) {
    uint8_t r = (rgb_color & 0xF800) >> 11;
    uint8_t g = (rgb_color & 0x07E0) >> 5;
    uint8_t b = (rgb_color & 0x001F);
    return (b << 11) | (g << 5) | r;
}

inline uint16_t bgr565_to_rgb565(uint16_t bgr_color) {
    return rgb565_to_bgr565(bgr_color); // Symmetrical conversion
}

// Helper function to create colors - USE createColor() INSTEAD OF color()
inline uint16_t createColor(uint8_t r, uint8_t g, uint8_t b) {
    return rgb_to_bgr565(r, g, b);
}

// Color blending
inline uint16_t blendColors(uint16_t color1, uint16_t color2, uint8_t alpha) {
    uint8_t r1, g1, b1, r2, g2, b2;
    bgr565_to_rgb888(color1, r1, g1, b1);
    bgr565_to_rgb888(color2, r2, g2, b2);
    
    uint16_t r = (r1 * alpha + r2 * (255 - alpha)) / 255;
    uint16_t g = (g1 * alpha + g2 * (255 - alpha)) / 255;
    uint16_t b = (b1 * alpha + b2 * (255 - alpha)) / 255;
    
    return createColor(r, g, b);
}

// Brightness adjustment
inline uint16_t adjustBrightness(uint16_t color, float factor) {
    uint8_t r, g, b;
    bgr565_to_rgb888(color, r, g, b);
    
    r = constrain(r * factor, 0, 255);
    g = constrain(g * factor, 0, 255);
    b = constrain(b * factor, 0, 255);
    
    return createColor(r, g, b);  // FIXED: gebruik createColor() ipv color()
}

// Print color info to Serial
inline void printColorInfo(const char* name, uint16_t color) {
    uint8_t r, g, b;
    bgr565_to_rgb888(color, r, g, b);
    Serial.printf("%s: 0x%04X -> RGB(%d,%d,%d)\n", name, color, r, g, b);
}

// Simple color creation with different name to avoid conflict
inline uint16_t makeColor(uint8_t r, uint8_t g, uint8_t b) {
    return createColor(r, g, b);
}