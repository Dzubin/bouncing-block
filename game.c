//
//  game.c - helper routines for the PicoCalc PONG game (everything except main()).
//

#include "pico/stdlib.h"

#include "drivers/lcd.h"
#include "drivers/audio.h"

#include "constants.h"
#include "game.h"

float clampf(float v, float lo, float hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

// --- Sound -----------------------------------------------------------------
// Start a tone that auto-stops after BEEP_MS, without blocking the game loop.
static absolute_time_t beep_off_time;
static bool            beep_active = false;

void beep(uint32_t freq)
{
    audio_play_sound(freq, freq);            // both channels, non-blocking
    beep_off_time = make_timeout_time_ms(BEEP_MS);
    beep_active = true;
}

void beep_update(void)
{
    if (beep_active && time_reached(beep_off_time))
    {
        audio_stop();
        beep_active = false;
    }
}

// --- Seven-segment drawing ----------------------------------------------
// Segment bitmasks, bit0=a (top), b, c, d, e, f, bit6=g (middle).
static const uint8_t seg_font[10] = {
    0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F
};

void draw_digit(int dx, int dy, int value, uint16_t on, uint16_t off)
{
    const uint8_t m    = (value >= 0 && value <= 9) ? seg_font[value] : 0x00;
    const int     hlen = SEG_W - 2 * SEG_T;
    const int     vlen = (SEG_H - 3 * SEG_T) / 2;
    const int     midY = dy + (SEG_H - SEG_T) / 2;

    lcd_solid_rectangle((m & 0x01) ? on : off, dx + SEG_T,          dy,                  hlen,  SEG_T);  // a
    lcd_solid_rectangle((m & 0x02) ? on : off, dx + SEG_W - SEG_T,  dy + SEG_T,          SEG_T, vlen);   // b
    lcd_solid_rectangle((m & 0x04) ? on : off, dx + SEG_W - SEG_T,  midY + SEG_T,        SEG_T, vlen);   // c
    lcd_solid_rectangle((m & 0x08) ? on : off, dx + SEG_T,          dy + SEG_H - SEG_T,  hlen,  SEG_T);  // d
    lcd_solid_rectangle((m & 0x10) ? on : off, dx,                  midY + SEG_T,        SEG_T, vlen);   // e
    lcd_solid_rectangle((m & 0x20) ? on : off, dx,                  dy + SEG_T,          SEG_T, vlen);   // f
    lcd_solid_rectangle((m & 0x40) ? on : off, dx + SEG_T,          midY,                hlen,  SEG_T);  // g
}

// Two small squares, vertically centred, forming the ":" of MM:SS (always lit).
void draw_colon(int cx, int cy, uint16_t on)
{
    const int x = cx + (COLON_W - SEG_T) / 2;
    lcd_solid_rectangle(on, x, cy + SEG_H / 3 - SEG_T / 2,     SEG_T, SEG_T);
    lcd_solid_rectangle(on, x, cy + 2 * SEG_H / 3 - SEG_T / 2, SEG_T, SEG_T);
}

// Score: two digits at the left of the bottom strip; a leading zero is blanked.
void draw_counter(int value)
{
    if (value < 0)
    {
        value = 0;
    }

    int tens = (value >= 10) ? (value / 10) % 10 : SEG_BLANK;

    draw_digit(SCORE_X,                     BOTTOM_Y, tens,       SEG_ON, SEG_OFF);
    draw_digit(SCORE_X + SEG_W + DIGIT_GAP, BOTTOM_Y, value % 10, SEG_ON, SEG_OFF);
}

// Elapsed time as MM:SS to the right of the score. Minutes saturate at 99, and
// the leading zero of MM is blanked.
void draw_timer(uint32_t total_seconds)
{
    uint32_t mm = total_seconds / 60;
    uint32_t ss = total_seconds % 60;
    if (mm > 99)
    {
        mm = 99;
        ss = 59;
    }

    int mm_tens = (mm >= 10) ? (int)(mm / 10) : SEG_BLANK;

    const int x = TIMER_X;
    draw_digit(x,                                       BOTTOM_Y, mm_tens,               SEG_ON, SEG_OFF);
    draw_digit(x + SEG_W + DIGIT_GAP,                   BOTTOM_Y, (int)(mm % 10),        SEG_ON, SEG_OFF);
    draw_colon(x + 2 * SEG_W + DIGIT_GAP,               BOTTOM_Y, SEG_ON);
    draw_digit(x + 2 * SEG_W + DIGIT_GAP + COLON_W,     BOTTOM_Y, (int)((ss / 10) % 10), SEG_ON, SEG_OFF);
    draw_digit(x + 3 * SEG_W + 2 * DIGIT_GAP + COLON_W, BOTTOM_Y, (int)(ss % 10),        SEG_ON, SEG_OFF);
}

// --- Ball serve -------------------------------------------------------
// Vertical direction alternates each serve so it is not perfectly repetitive.
static int serve_dir = 1;

void serve(float *x, float *y, float *vx, float *vy, float *hspeed)
{
    *hspeed = SERVE_SPEED;
    *x  = FIELD_RIGHT - WALL - BALL_SIZE;                 // start at the right wall
    *y  = FIELD_TOP  + (FIELD_H - BALL_SIZE) / 2.0f;
    *vx = -*hspeed;
    *vy = serve_dir * (*hspeed * 0.6f);
    serve_dir = -serve_dir;
}
