/*
 * board.cpp — per-board display + touch construction. See board.h.
 *
 * Everything here is behind the board-selection #if, so exactly one board's
 * code is compiled and the published per-app repos flatten to a single branch.
 */
#include "board.h"
#include <Wire.h>

// ─────────────────────────────────────────────────────────────────────────────
// Waveshare ESP32-S3-Touch-AMOLED-1.8 — V1 SH8601/FT3168, V2 CO5300/CST816
// Lifted verbatim from the per-app hw_panel.cpp so V1/V2 behaviour is unchanged.
// ─────────────────────────────────────────────────────────────────────────────
#include "TouchDrvFT6X36.hpp"        // V1 touch (FocalTech)
#include "touch/TouchDrvCST816.h"    // V2 touch

// Probe I2C 0x15 once: CST816 (V2) ACKs there; nothing on V1 does.
bool board_amoled18_is_v2() {
    static int cached = -1;            // -1 unknown, 0 = V1, 1 = V2
    if (cached < 0) {
        Wire.beginTransmission(0x15);  // CST816_SLAVE_ADDRESS
        cached = (Wire.endTransmission() == 0) ? 1 : 0;
    }
    return cached == 1;
}

Arduino_OLED *board_make_display(Arduino_DataBus *bus) {
    if (board_amoled18_is_v2())
        return new Arduino_CO5300(bus, GFX_NOT_DEFINED, 0, LCD_WIDTH, LCD_HEIGHT,
                                  PANEL_CO5300_XOFF, 0,
                                  480 - LCD_WIDTH - PANEL_CO5300_XOFF, 480 - LCD_HEIGHT);
    return new Arduino_SH8601(bus, GFX_NOT_DEFINED, 0, LCD_WIDTH, LCD_HEIGHT);
}

TouchDrvInterface *board_make_touch() {
    if (board_amoled18_is_v2()) {
        TouchDrvCST816 *t = new TouchDrvCST816();
        t->begin(Wire, CST816_SLAVE_ADDRESS, IIC_SDA, IIC_SCL);
        return t;
    }
    TouchDrvFT6X36 *t = new TouchDrvFT6X36();
    t->begin(Wire, FT6X36_SLAVE_ADDRESS, IIC_SDA, IIC_SCL);
    return t;
}

