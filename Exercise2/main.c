/* Implement a program for switching LEDs on/off and dimming them. The program should work as follows:
 * • Rot_Sw, the push button on the rotary encoder shaft is the on/off button. When button is pressed
 *   the state of LEDs is toggled. Program must require the button to be released before the LEDs toggle
 *   again. Holding the button may not cause LEDs to toggle multiple times.
 * • Rotary encoder is used to control brightness of the LEDs. Turning the knob clockwise increases
 *   brightness and turning counter-clockwise reduces brightness. If LEDs are in OFF state turning the
 *   knob has no effect.
 * • When LED state is toggled to ON the program must use same brightness of the LEDs they were at
 *   when they were switched off. If LEDs were dimmed to 0% then toggling them on will set 50%
 *   brightness.
 * • PWM frequency divider must be configured to output 1 MHz frequency and PWM frequency must
 *   be 1 kHz.
 * You must use GPIO interrupts for detecting the encoder turns.
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/irq.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"

#define D1 22
#define D2 21
#define D3 20

#define ROT_SW 12
#define ROT_A 10
#define ROT_B 11
#define BUTTON_PERIOD 10 // Button sampling timer period in ms
#define BUTTON_FILTER 5
#define RELEASED 1

#define PWM_FREQ 1000
#define LEVEL 5
#define DIVIDER 125
#define MIN_BRIGHTNESS 0
#define MAX_BRIGHTNESS 1000
#define BRIGHTNESS_STEP 20

void ledsInit();
void pwmInit();
void allLedsOn();
void allLedsOff();
void rotInit();
bool repeatingTimerCallback(struct repeating_timer *t);
void encoderAInterruptHandler(uint gpio, uint32_t events);

volatile bool buttonEvent = false;
volatile int brightness = MAX_BRIGHTNESS / 2;
volatile bool ledState = true;

int main(void) {

    stdio_init_all();

    rotInit();
    gpio_set_irq_enabled_with_callback(ROT_A, GPIO_IRQ_EDGE_RISE, true, encoderAInterruptHandler);

    ledsInit();

    pwmInit();

    struct repeating_timer timer;
    add_repeating_timer_ms(BUTTON_PERIOD, repeatingTimerCallback, NULL, &timer);

    while (true){

        if (buttonEvent) {
            buttonEvent = false;
            if(true == ledState) {
                if (brightness != MIN_BRIGHTNESS) {
                    ledState = false;
                } else {
                    ledState = true;
                    brightness = MAX_BRIGHTNESS / 2;
                }
            } else {
                ledState = true;
            }
        }

        if (true == ledState) {
            allLedsOn();
        } else {
            allLedsOff();
        }
    }
}

void ledsInit() {
    gpio_init(D3);
    gpio_set_dir(D3, GPIO_OUT);
    gpio_init(D2);
    gpio_set_dir(D2, GPIO_OUT);
    gpio_init(D1);
    gpio_set_dir(D1, GPIO_OUT);
}

void rotInit() {
    gpio_init(ROT_A);
    gpio_set_dir(ROT_A, GPIO_IN);

    gpio_init(ROT_SW);
    gpio_set_dir(ROT_SW, GPIO_IN);
    gpio_pull_up(ROT_SW);

    gpio_init(ROT_B);
    gpio_set_dir(ROT_B, GPIO_IN);

}

void pwmInit() {

    pwm_config config = pwm_get_default_config();

    // D1:             (2A)
    uint d1_slice = pwm_gpio_to_slice_num(D1);
    uint d1_chanel = pwm_gpio_to_channel(D1);
    pwm_set_enabled(d1_slice, false);
    pwm_config_set_clkdiv_int(&config, DIVIDER);
    pwm_config_set_wrap(&config, PWM_FREQ - 1);
    pwm_init(d1_slice, &config, false);
    pwm_set_chan_level(d1_slice, d1_chanel, LEVEL + 1);
    gpio_set_function(D1, GPIO_FUNC_PWM);
    pwm_set_enabled(d1_slice, true);

    // D2:             (2B)
    uint d2_slice = pwm_gpio_to_slice_num(D2);
    uint d2_chanel = pwm_gpio_to_channel(D2);
    pwm_set_enabled(d2_slice, false);
    pwm_config_set_clkdiv_int(&config, DIVIDER);
    pwm_config_set_wrap(&config, PWM_FREQ - 1);
    pwm_init(d2_slice, &config, false);
    pwm_set_chan_level(d2_slice, d2_chanel, LEVEL + 1);
    gpio_set_function(D2, GPIO_FUNC_PWM);
    pwm_set_enabled(d2_slice, true);

    //D3:              (3A)
    uint d3_slice = pwm_gpio_to_slice_num(D3);
    uint d3_chanel = pwm_gpio_to_channel(D3);
    pwm_set_enabled(d3_slice, false);
    pwm_config_set_clkdiv_int(&config, DIVIDER);
    pwm_config_set_wrap(&config, PWM_FREQ - 1);
    pwm_init(d3_slice, &config, false);
    pwm_set_chan_level(d3_slice, d3_chanel, LEVEL + 1);
    gpio_set_function(D3, GPIO_FUNC_PWM);
    pwm_set_enabled(d3_slice, true);
}

void allLedsOn() {
    pwm_set_gpio_level(D1, brightness);
    pwm_set_gpio_level(D2, brightness);
    pwm_set_gpio_level(D3, brightness);
}

void allLedsOff() {
    pwm_set_gpio_level(D1, MIN_BRIGHTNESS);
    pwm_set_gpio_level(D2, MIN_BRIGHTNESS);
    pwm_set_gpio_level(D3, MIN_BRIGHTNESS);
}

bool repeatingTimerCallback(struct repeating_timer *t) {

    // For SW_1: ON-OFF
    static uint button_state = 0, filter_counter = 0;
    uint new_state = 1;

    new_state = gpio_get(ROT_SW);
    if (button_state != new_state) {
        if (++filter_counter >= BUTTON_FILTER) {
            button_state = new_state;
            filter_counter = 0;
            if (new_state != RELEASED) {
                buttonEvent = true;
            }
        }
    } else {
        filter_counter = 0;
    }

    return true;
}

void encoderAInterruptHandler(uint gpio, uint32_t events) {

    int rotB_state = gpio_get(ROT_B);

    if (true == ledState) {
        if (rotB_state == 0) {
            if (MIN_BRIGHTNESS <= brightness && brightness <= MAX_BRIGHTNESS) {
                brightness += BRIGHTNESS_STEP;
                if (brightness > MAX_BRIGHTNESS) {
                    brightness = MAX_BRIGHTNESS;
                }
            }
        } else {
            if (MIN_BRIGHTNESS <= brightness && brightness <= MAX_BRIGHTNESS) {
                brightness -= BRIGHTNESS_STEP;
                if (brightness < MIN_BRIGHTNESS) {
                    brightness = MIN_BRIGHTNESS;
                }
            }
       }
    }
}
