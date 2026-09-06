/*
 * board_amoled_1_8.h — Waveshare ESP32-S3-Touch-AMOLED-1.8
 *
 * 368×448 AMOLED, ESP32-S3R8, 8 MB PSRAM / 16 MB flash.
 *
 * This board shipped in TWO revisions with the same panel size:
 *   V1  SH8601 display + FT3168 touch (FocalTech @0x38)
 *   V2  CO5300 display + CST816  touch (@0x15)   — from ~2026-05-30
 * They are told apart at boot by board_amoled18_is_v2(); V1 is the fallback,
 * so a V1 device always takes the original, proven path.
 *
 * Pin values are a verbatim lift of the old libraries/Mylibrary/pin_config.h.
 * Do not "tidy" them — they are the shipped configuration for 24 live apps.
 */
#pragma once

#define BOARD_ID              "amoled-1.8"
#define BOARD_NAME            "Waveshare ESP32-S3-Touch-AMOLED-1.8"

// ── Capabilities ────────────────────────────────────────────────────────────
#define BOARD_HAS_EXPANDER    1      // XCA9554 @0x20 — display/touch reset, SD CS
#define BOARD_HAS_AUDIO       1      // ES8311 codec
#define BOARD_HAS_TOUCH       1
#define BOARD_HAS_SD          1
#define BOARD_HAS_IMU         1      // QMI8658
#define BOARD_HAS_RTC         1      // PCF85063
#define BOARD_HAS_PSRAM       1

#define XPOWERS_CHIP_AXP2101         // must precede XPowersLib.h

// ── Display — QSPI ──────────────────────────────────────────────────────────
#define LCD_SDIO0             4
#define LCD_SDIO1             5
#define LCD_SDIO2             6
#define LCD_SDIO3             7
#define LCD_SCLK              11
#define LCD_CS                12
#define LCD_WIDTH             368
#define LCD_HEIGHT            448

// Display reset lives on the GPIO expander (EXIO1/2), not a real GPIO.
#define LCD_RESET             GFX_NOT_DEFINED

// CO5300 (V2 only) GRAM column where the 368px panel starts.
#define PANEL_CO5300_XOFF     16

// ── Physical UI geometry (see apps/UI_GUIDE-amoled-1.8.md) ──────────────────
#define CORNER_R              50   // AMOLED corner rounding, px
#define BOOT_BTN_Y_P          90   // BOOT button centre, portrait y
#define PWR_BTN_Y_P          355   // PWR  button centre, portrait y
#define BOOT_BTN_X_L          95   // BOOT mapped to landscape x (rotation=1)
#define PWR_BTN_X_L          345   // PWR  mapped to landscape x (rotation=1)

// ── IMU orientation ─────────────────────────────────────────────────────────
// Sign applied to each QMI8658 accelerometer axis so "tilt right" means the
// same thing on every board. This board is the reference: all +1.
#define BOARD_IMU_AX_SIGN      1
#define BOARD_IMU_AY_SIGN      1
#define BOARD_IMU_AZ_SIGN      1

// ── Touch — I2C ─────────────────────────────────────────────────────────────
#define IIC_SDA               15
#define IIC_SCL               14
#define TP_INT                21

// ── Audio — ES8311 over I2S ─────────────────────────────────────────────────
#define I2S_MCK_IO            16
#define I2S_BCK_IO            9
#define I2S_DI_IO             10
#define I2S_WS_IO             45
#define I2S_DO_IO             8

#define MCLKPIN               16
#define BCLKPIN               9
#define WSPIN                 45
#define DOPIN                 10
#define DIPIN                 8
#define PA                    46

// ── SD (SDMMC; chip-select is EXIO7 on the expander) ────────────────────────
const int SDMMC_CLK  = 2;
const int SDMMC_CMD  = 1;
const int SDMMC_DATA = 3;

/*
 * True if this is a V2 board (CO5300 + CST816). Probes I2C 0x15 once and
 * caches the result — only V2's CST816 answers there. Must be called after
 * Wire.begin().
 *
 * Only meaningful on this board; it is deliberately not part of the common
 * board API, so code that calls it cannot compile for another board by
 * accident.
 */
bool board_amoled18_is_v2();
