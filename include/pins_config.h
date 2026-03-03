#pragma once

//TFT
#define LCD_H_RES 1024
#define LCD_V_RES 600
//lcd ctrl
#define LCD_RST 27
#define LCD_LED 23

//i2c (touch + rtc + codec + conector)
#define TP_I2C_SDA 7
#define TP_I2C_SCL 8
#define TP_RST 22
#define TP_INT 21

// ---- AUDIO CODEC ES8311 ----
#define CODEC_I2C_ADDR 0x18
#define AUDIO_I2S_USE_MSB 0  // 0=PHILIPS, 1=MSB
#define AUDIO_I2S_WS_INV  1   // 0=normal, 1=invertir WS
#define AUDIO_I2S_BCLK_INV 0  // 0=normal, 1=invertir BCLK

// I2S0 pins (ESP32-P4)
#define CODEC_I2S_MCLK 13
#define CODEC_I2S_BCLK 12
#define CODEC_I2S_LRCK 10
#define CODEC_I2S_DOUT 9
// (futuro) mic input
#define CODEC_I2S_DIN  11

// Amplifier enable (NS4150)
#define PA_CTRL 20

// Extra hardware
#define WS2812_PIN 26
