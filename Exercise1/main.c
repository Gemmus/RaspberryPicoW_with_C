/*
 * Follow instructions in clion_setup.pdf to create CMake project.
 * Implement a program for switching LEDs on/off and dimming them. The program should work as follows:
 * • SW1, the middle button is the on/off button. When button is pressed the state of LEDs is toggled.
 * Program must require the button to be released before the LEDs toggle again. Holding the button
 * may not cause LEDs to toggle multiple times.
 * • SW0 and SW2 are used to control dimming when LEDs are in ON state. SW0 increases brightness
 *   and SW2 decreases brightness. Holding a button makes the brightness to increase/decrease
 *   smoothly. If LEDs are in OFF state the buttons have no effect.
 * • When LED state is toggled to ON the program must use same brightness of the LEDs they were at
 *   when they were switched off. If LEDs were dimmed to 0% then toggling them on will set 50% brightness.
 * • PWM frequency divider must be configured to output 1 MHz frequency and PWM frequency must be 1 kHz.
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

#define SW_0 9 // icreases brightness gradually if held; only in ON state
#define SW_1 8 // ON - OFF
#define SW_2 7 // decreases brightness gradually if held; only in ON state
#define BUTTON_PERIOD 10 // Button sampling timer period in ms
#define BUTTON_FILTER 5
#define RELEASED 1

#define PWM_FREQ 1000
#define LEVEL 5
#define DIVIDER 125
#define MIN_BRIGHTNESS 0
#define MAX_BRIGHTNESS 1000
#define BRIGHTNESS_STEP 2

void ledInitalizer();
void buttonInitializer();
void pwmInitializer();
void allLedOn();
void allLedOff();
bool repeatingTimerCallback(struct repeating_timer *t);

volatile bool buttonEvent = false;
volatile int brightness = MAX_BRIGHTNESS / 2;
volatile bool ledState = true;

int main(void) {

    stdio_init_all();

    ledInitalizer();

    buttonInitializer();

    pwmInitializer();

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
            allLedOn();
        } else {
            allLedOff();
        }
    }
}

void ledInitalizer() {
    gpio_init(D3);
    gpio_set_dir(D3, GPIO_OUT);
    gpio_init(D2);
    gpio_set_dir(D2, GPIO_OUT);
    gpio_init(D1);
    gpio_set_dir(D1, GPIO_OUT);
}

void buttonInitializer() {
    gpio_init(SW_0);
    gpio_set_dir(SW_0, GPIO_IN);
    gpio_pull_up(SW_0);
    gpio_init(SW_1);
    gpio_set_dir(SW_1, GPIO_IN);
    gpio_pull_up(SW_1);
    gpio_init(SW_2);
    gpio_set_dir(SW_2, GPIO_IN);
    gpio_pull_up(SW_2);
}

void pwmInitializer() {

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

void allLedOn() {
    pwm_set_gpio_level(D1, brightness);
    pwm_set_gpio_level(D2, brightness);
    pwm_set_gpio_level(D3, brightness);
}

void allLedOff() {
    pwm_set_gpio_level(D1, MIN_BRIGHTNESS);
    pwm_set_gpio_level(D2, MIN_BRIGHTNESS);
    pwm_set_gpio_level(D3, MIN_BRIGHTNESS);
}

bool repeatingTimerCallback(struct repeating_timer *t) {

    // For SW1
    static uint button_state = 0, filter_counter = 0;
    uint new_state = 1;

    new_state = gpio_get(SW_1);
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

    if (true == ledState) {
        // For SW0
        if (!gpio_get(SW_0)) { // increase
            if (MIN_BRIGHTNESS <= brightness && brightness <= MAX_BRIGHTNESS) {
                brightness += BRIGHTNESS_STEP;
                if (brightness > MAX_BRIGHTNESS) {
                    brightness = MAX_BRIGHTNESS;
                }
            }
        }
        // For SW2
        if (!gpio_get(SW_2)) { // decrease
            if (MIN_BRIGHTNESS <= brightness && brightness <= MAX_BRIGHTNESS) {
                brightness -= BRIGHTNESS_STEP;
                if (brightness < MIN_BRIGHTNESS) {
                    brightness = MIN_BRIGHTNESS;
                }
            }
        }
    }

    return true;
}
