//
//  constants.h - compile-time configuration for the PicoCalc PONG game.
//
//  Every tunable value and colour lives here. main.c and game.c both include it.
//

#pragma once

#include "drivers/lcd.h"        // for RGB() and the LCD geometry

// --- Playfield: 320 wide x 256 tall, top-left corner of the screen ---------
#define FIELD_LEFT      0
#define FIELD_TOP       0
#define FIELD_W         320
#define FIELD_H         256
#define FIELD_RIGHT     (FIELD_LEFT + FIELD_W)           // 320
#define FIELD_BOTTOM    (FIELD_TOP  + FIELD_H)           // 256
#define WALL            2       // border thickness (drawn inside) and play inset

// --- Ball -----------------------------------------------------------------
#define BALL_SIZE       9
#define SERVE_SPEED     0.75f   // horizontal speed at serve (75% slower than 3.0)
#define SPEED_GAIN      1.10f   // x1.10 horizontal speed on every paddle hit
#define HSPEED_MAX      8.0f    // horizontal speed cap (keeps collisions reliable)
#define VY_MAX          6.0f    // vertical speed cap (from paddle "spin")

// --- Paddle -------------------------------------------------------------
#define PADDLE_W        5
#define PADDLE_H        40
#define PADDLE_X        (FIELD_LEFT + WALL)
#define PADDLE_ACCEL    6.0f    // velocity added per UP/DOWN key event
#define PADDLE_FRICTION 0.80f   // per-frame coast-down
#define PADDLE_VMAX     14.0f   // paddle speed cap, px/frame

// --- Sound effects (non-blocking, 0.2 s each) ------------------------------
#define BEEP_MS         200
#define TONE_MISS       400     // ball missed by the paddle
#define TONE_HIT        600     // ball hit by the paddle
#define TONE_WALL       800     // ball bounced off a playfield side

// --- Seven-segment readouts below the playfield ---------------------------
//     [ score ]      [ MM : SS elapsed time ]
#define SEG_W           28      // digit cell width
#define SEG_H           48      // digit cell height (> 30, "LED display" look)
#define SEG_T           6       // segment thickness
#define DIGIT_GAP       14      // space between adjacent digits
#define COLON_W         12      // width of the colon column in MM:SS
#define SEG_BLANK       (-1)    // pass to draw_digit() for an all-unlit cell

#define BOTTOM_Y        (FIELD_BOTTOM + 10)     // 266
#define SCORE_X         10                      // score's left edge
#define TIMER_X         132                     // MM:SS field's left edge

#define START_LIVES     9
#define FRAME_MS        16      // ~60 frames per second

// --- Leaving for the PicoCalc UF2 Loader (pelrun/uf2loader) ------------
// The loader has no call for an app to use, but its own menu hands commands to
// its start-up code through the chip's watchdog scratch registers, which
// survive a watchdog reboot: scratch 0 holds a magic number, 1 the boot mode,
// 2 an argument. Asking for boot mode "SD" and then rebooting makes the loader
// show its menu again. With no loader installed the game just restarts.
#define LOADER_COMMAND_MAGIC     0xE98CC638u  // PICOCALC_BL_MAGIC in the loader's proginfo.h
#define LOADER_BOOT_MODE_SD      1            // BOOT_SD: load the menu from the SD card
#define LOADER_SCRATCH_MAGIC     0
#define LOADER_SCRATCH_MODE      1
#define LOADER_SCRATCH_ARGUMENT  2
#define LOADER_REBOOT_DELAY_MS   10           // the reboot happens this long after it is asked for

// --- Colours (RGB565) --------------------------------------------------
#define COLOR_BG        RGB(0, 0, 0)         // background
#define COLOR_BALL      RGB(0, 255, 128)     // ball
#define COLOR_PADDLE    RGB(255, 255, 255)   // paddle (white)
#define COLOR_BORDER    RGB(60, 60, 90)      // playfield outline
#define SEG_ON          RGB(255, 40, 40)     // lit segment  (red LED)
#define SEG_OFF         RGB(28, 8, 8)        // unlit segment (faint)
