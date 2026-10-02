//
//  game.h - helper routines for the PicoCalc PONG game.
//
//  Everything except main() lives in game.c; main.c drives these.
//

#pragma once

#include <stdint.h>

// Clamp v to the inclusive range [lo, hi].
float clampf(float v, float lo, float hi);

// Non-blocking sound effect: start a tone that auto-stops after BEEP_MS.
void beep(uint32_t freq);
// Call once per frame; silences the tone when its 0.2 s is up.
void beep_update(void);

// Draw one seven-segment digit with its top-left corner at (dx, dy). A value of
// 0-9 lights the matching segments; any other value (e.g. SEG_BLANK) draws every
// segment in the "off" colour - used to blank a leading zero.
void draw_digit(int dx, int dy, int value, uint16_t on, uint16_t off);
// Draw the ":" of MM:SS in the colon column whose top-left is (cx, cy).
void draw_colon(int cx, int cy, uint16_t on);

// Redraw the two-digit score (leading zero blanked).
void draw_counter(int value);
// Redraw the MM:SS elapsed-time readout (leading zero of MM blanked).
void draw_timer(uint32_t total_seconds);

// Serve the ball from the right wall toward the paddle at the serve speed.
void serve(float *x, float *y, float *vx, float *vy, float *hspeed);
