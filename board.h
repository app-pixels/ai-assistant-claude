/*
 * board.h — app-pixels board support.
 *
 * Single entry point for every hardware difference between the supported
 * boards. Apps include THIS and nothing else — no pin_config.h, no display or
 * touch driver header, no GPIO-expander header.
 *
 * This copy is flattened for the Waveshare ESP32-S3-Touch-AMOLED-1.8.
 *
 * Each board header defines:
 *   • pins + panel geometry (LCD_*, IIC_*, SDMMC_*, audio, …)
 *   • BOARD_ID / BOARD_NAME
 *   • capability flags — BOARD_HAS_EXPANDER, BOARD_HAS_AUDIO, …
 * and board.cpp builds the matching display + touch drivers.
 */
#pragma once

// ── Board selection ─────────────────────────────────────────────────────────
#define BOARD_AMOLED_18 1   // this repo targets the Waveshare ESP32-S3-Touch-AMOLED-1.8

#include "board_amoled_1_8.h"

// ── Common API — implemented per board in board.cpp ──────────────────────────
#include "Arduino_GFX_Library.h"     // Arduino_OLED / Arduino_SH8601 / Arduino_CO5300
#include "TouchDrvInterface.hpp"     // unified base for the FocalTech + CST816 drivers

/*
 * Build the display for this board. Call from setup() AFTER Wire.begin() —
 * the 1.8 needs I2C up to tell its two panel revisions apart.
 */
Arduino_OLED *board_make_display(Arduino_DataBus *bus);

/*
 * Build and begin() the touch controller for this board. Also call after
 * Wire.begin(). Returns nullptr on a board with no touch.
 */
TouchDrvInterface *board_make_touch();
