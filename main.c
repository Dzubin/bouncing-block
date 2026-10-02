//
//  PicoCalc - one-paddle PONG with a seven-segment "lives" counter
//
//  A 9x9 "ball" is served from the right wall and bounces inside a 320x256
//  playfield anchored to the top-left of the 320x320 LCD. A 5x40 white "paddle"
//  on the left edge is moved with the UP / DOWN arrow keys; the ball bounces off
//  it (with a little "spin"). Each paddle hit speeds the ball up by 10%. Short
//  0.2 s tones mark a side bounce (800 Hz), a paddle hit (600 Hz) and a miss
//  (400 Hz). Below the field, a two-digit seven-segment counter starts at 9 and
//  drops by one on every miss, and an MM:SS readout beside it counts elapsed
//  time (both blank their leading zero). When the counter hits zero the game
//  freezes until any key is pressed, then starts over.
//
//  Controls (PicoCalc keyboard):
//    UP / DOWN - move the paddle
//    ESC       - reboot into BOOTSEL (USB mass storage) mode
//
//  Source layout:
//    constants.h - every tunable value and colour
//    game.c / .h - all helper functions (drawing, sound, serve)
//    main.c      - just main() and the game loop
//
//  Only the LCD, keyboard and audio drivers from the picocalc-text-starter are
//  used. The controller's own frame memory is drawn into directly, so there is
//  no framebuffer in RAM.
//

#include "pico/stdlib.h"
#include "pico/bootrom.h"

#include "drivers/lcd.h"
#include "drivers/keyboard.h"
#include "drivers/audio.h"

#include "constants.h"
#include "game.h"

// Referenced by the keyboard driver (set when the Brk key is pressed). We do
// not use it here, but the driver needs the symbol to link.
volatile bool user_interrupt = false;

int main(void)
{
    lcd_init();                 // also clears the screen and turns the display on
    lcd_enable_cursor(false);   // we are not drawing text; hide the blinking cursor

    keyboard_init();
    keyboard_set_background_poll(true);   // poll the keyboard on a timer
    audio_init();

    // Playfield border, drawn once just inside the play area.
    lcd_solid_rectangle(COLOR_BORDER, FIELD_LEFT,         FIELD_TOP,           FIELD_W, WALL);   // top
    lcd_solid_rectangle(COLOR_BORDER, FIELD_LEFT,         FIELD_BOTTOM - WALL, FIELD_W, WALL);   // bottom
    lcd_solid_rectangle(COLOR_BORDER, FIELD_LEFT,         FIELD_TOP,           WALL, FIELD_H);   // left
    lcd_solid_rectangle(COLOR_BORDER, FIELD_RIGHT - WALL, FIELD_TOP,           WALL, FIELD_H);   // right

    int lives = START_LIVES;
    draw_counter(lives);

    // Elapsed-time readout (MM:SS), incremented once per second. Reset on restart.
    uint32_t start_ms = to_ms_since_boot(get_absolute_time());
    uint32_t shown_seconds = 0;
    draw_timer(shown_seconds);

    // Ball state (sub-pixel position + velocity + current horizontal speed).
    float bx, by, vx, vy, hspeed;
    serve(&bx, &by, &vx, &vy, &hspeed);

    // Paddle state.
    float paddle_y  = FIELD_TOP + (FIELD_H - PADDLE_H) / 2.0f;
    float paddle_vy = 0.0f;

    // Last integer positions actually drawn, so we can erase exactly those.
    int ball_px = (int)(bx + 0.5f);
    int ball_py = (int)(by + 0.5f);
    int paddle_drawn_y = (int)(paddle_y + 0.5f);

    lcd_solid_rectangle(COLOR_PADDLE, PADDLE_X, paddle_drawn_y, PADDLE_W, PADDLE_H);
    lcd_solid_rectangle(COLOR_BALL,   ball_px,  ball_py,        BALL_SIZE, BALL_SIZE);

    while (true)
    {
        // ---- input ----
        while (keyboard_key_available())
        {
            switch ((unsigned char)keyboard_get_key())
            {
                case KEY_ESC:  audio_stop(); rom_reset_usb_boot(0, 0); break;
                case KEY_UP:   paddle_vy -= PADDLE_ACCEL; break;
                case KEY_DOWN: paddle_vy += PADDLE_ACCEL; break;
                default: break;
            }
        }

        // ---- paddle physics ----
        paddle_vy = clampf(paddle_vy, -PADDLE_VMAX, PADDLE_VMAX);
        paddle_y += paddle_vy;
        paddle_vy *= PADDLE_FRICTION;
        paddle_y = clampf(paddle_y, FIELD_TOP + WALL, FIELD_BOTTOM - WALL - PADDLE_H);

        // ---- ball physics ----
        bx += vx;
        by += vy;

        // top / bottom walls
        if (by <= FIELD_TOP + WALL)
        {
            by = FIELD_TOP + WALL;
            vy = -vy;
            beep(TONE_WALL);
        }
        else if (by >= FIELD_BOTTOM - WALL - BALL_SIZE)
        {
            by = FIELD_BOTTOM - WALL - BALL_SIZE;
            vy = -vy;
            beep(TONE_WALL);
        }

        // right wall
        if (bx >= FIELD_RIGHT - WALL - BALL_SIZE)
        {
            bx = FIELD_RIGHT - WALL - BALL_SIZE;
            vx = -vx;
            beep(TONE_WALL);
        }

        // paddle: bounce if the ball is moving left, has reached the paddle
        // column, and overlaps it vertically.
        if (vx < 0.0f && bx <= PADDLE_X + PADDLE_W &&
            by + BALL_SIZE >= paddle_y && by <= paddle_y + PADDLE_H)
        {
            // Successful hit: speed the ball up by 10% (capped) and send it back.
            hspeed = clampf(hspeed * SPEED_GAIN, 0.0f, HSPEED_MAX);
            bx = PADDLE_X + PADDLE_W;
            vx = hspeed;
            beep(TONE_HIT);

            // "spin": where the ball strikes the paddle steers it.
            //   -1 = top of paddle, +1 = bottom of paddle
            float rel = ((by + BALL_SIZE / 2.0f) - (paddle_y + PADDLE_H / 2.0f))
                        / (PADDLE_H / 2.0f);
            vy = clampf(vy + rel * 2.5f, -VY_MAX, VY_MAX);
        }

        // missed the paddle
        if (bx <= FIELD_LEFT + WALL)
        {
            beep(TONE_MISS);

            if (lives > 0)
            {
                lives--;
            }
            draw_counter(lives);

            if (lives == 0)
            {
                // Game over: freeze until a fresh keypress, then start over.
                while (keyboard_key_available())
                {
                    keyboard_get_key();          // drop anything already buffered
                }
                while (!keyboard_key_available())
                {
                    beep_update();               // let the miss tone finish
                    tight_loop_contents();
                }
                keyboard_get_key();              // consume the restart key
                audio_stop();

                lives = START_LIVES;
                draw_counter(lives);
                start_ms = to_ms_since_boot(get_absolute_time());
                shown_seconds = 0;
                draw_timer(shown_seconds);
                paddle_y = FIELD_TOP + (FIELD_H - PADDLE_H) / 2.0f;
                paddle_vy = 0.0f;
            }

            serve(&bx, &by, &vx, &vy, &hspeed);
        }

        // ---- render ----
        int ball_nx = (int)(bx + 0.5f);
        int ball_ny = (int)(by + 0.5f);
        int paddle_ny = (int)(paddle_y + 0.5f);

        // Erase the ball, then (if it moved) the strip the paddle vacated.
        lcd_solid_rectangle(COLOR_BG, ball_px, ball_py, BALL_SIZE, BALL_SIZE);
        if (paddle_ny != paddle_drawn_y)
        {
            lcd_solid_rectangle(COLOR_BG, PADDLE_X, paddle_drawn_y, PADDLE_W, PADDLE_H);
        }

        // Redraw paddle every frame (cheap overwrite), then the ball on top.
        lcd_solid_rectangle(COLOR_PADDLE, PADDLE_X, paddle_ny, PADDLE_W, PADDLE_H);
        lcd_solid_rectangle(COLOR_BALL,   ball_nx,  ball_ny,   BALL_SIZE, BALL_SIZE);

        ball_px = ball_nx;
        ball_py = ball_ny;
        paddle_drawn_y = paddle_ny;

        // Tick the MM:SS readout when a new second has elapsed.
        uint32_t elapsed = (to_ms_since_boot(get_absolute_time()) - start_ms) / 1000;
        if (elapsed != shown_seconds)
        {
            shown_seconds = elapsed;
            draw_timer(shown_seconds);
        }

        beep_update();          // silence any sound effect whose 0.2 s is up
        sleep_ms(FRAME_MS);
    }
}
