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

#define D1_LED 22
#define D2_LED 21
#define D3_LED 20
#define SW0 9 // icreases brightness gradually if held; only in ON state
#define SW1 8 // ON - OFF
#define SW2 7 // decreases brightness gradually if held; only in ON state
#define BUTTON_FILTERING 5
#define RELEASED 1
#define PWM_FREQ 999
#define LEVEL 6
#define DIVIDER 125

void ledInitalizer();
void buttonInitializer();
void pwmInitializer();
void allLedOn();
void allLedOff();
bool repeatingTimerCallback(struct repeating_timer *t);

volatile bool buttonreleased = false;
volatile int brightness = 500;
volatile bool ledstate = false;

int main(void) {

    // Initialize input output
    stdio_init_all();

    // Initialize LED pin
    ledInitalizer();

    // Initialize LED pin
    buttonInitializer();

    // Initialize PWM
    pwmInitializer();

    struct repeating_timer timer;
    add_repeating_timer_ms(10, repeatingTimerCallback, NULL, &timer);

    // Loop forever
    while (true){

        if (buttonreleased) {
            buttonreleased = false;
            if(true == ledstate) {
                if (brightness != 0) {
                    ledstate = false;
                } else {
                    ledstate = true;
                    brightness = 500;
                }
            } else {
                ledstate = true;
            }
        }

        if (true == ledstate) {
            allLedOn();
        } else {
            allLedOff();
        }
    }
}

void ledInitalizer() {
    gpio_init(D3_LED);
    gpio_set_dir(D3_LED, GPIO_OUT);
    gpio_init(D2_LED);
    gpio_set_dir(D2_LED, GPIO_OUT);
    gpio_init(D1_LED);
    gpio_set_dir(D1_LED, GPIO_OUT);
}

void buttonInitializer() {
    gpio_init(SW0);
    gpio_set_dir(SW0, GPIO_IN);
    gpio_pull_up(SW0);
    gpio_init(SW1);
    gpio_set_dir(SW1, GPIO_IN);
    gpio_pull_up(SW1);
    gpio_init(SW2);
    gpio_set_dir(SW2, GPIO_IN);
    gpio_pull_up(SW2);
}

void pwmInitializer() {

    pwm_config config = pwm_get_default_config();

    // D1:             (2A)
    uint d1_slice = pwm_gpio_to_slice_num(D1_LED);
    uint d1_chanel = pwm_gpio_to_channel(D1_LED);
    pwm_set_enabled(d1_slice, false);
    pwm_config_set_clkdiv_int(&config, DIVIDER);
    pwm_config_set_wrap(&config, PWM_FREQ);
    pwm_init(d1_slice, &config, false);
    pwm_set_chan_level(d1_slice, d1_chanel, LEVEL);
    gpio_set_function(D1_LED, GPIO_FUNC_PWM);
    pwm_set_enabled(d1_slice, true);

    // D2:             (2B)
    uint d2_slice = pwm_gpio_to_slice_num(D2_LED);
    uint d2_chanel = pwm_gpio_to_channel(D2_LED);
    pwm_set_enabled(d2_slice, false);
    pwm_config_set_clkdiv_int(&config, DIVIDER);
    pwm_config_set_wrap(&config, PWM_FREQ);
    pwm_init(d2_slice, &config, false);
    pwm_set_chan_level(d2_slice, d2_chanel, LEVEL);
    gpio_set_function(D2_LED, GPIO_FUNC_PWM);
    pwm_set_enabled(d2_slice, true);

    //D3:              (3A)
    uint d3_slice = pwm_gpio_to_slice_num(D3_LED);
    uint d3_chanel = pwm_gpio_to_channel(D3_LED);
    pwm_set_enabled(d3_slice, false);
    pwm_config_set_clkdiv_int(&config, DIVIDER);
    pwm_config_set_wrap(&config, PWM_FREQ);
    pwm_init(d3_slice, &config, false);
    pwm_set_chan_level(d3_slice, d3_chanel, LEVEL);
    gpio_set_function(D3_LED, GPIO_FUNC_PWM);
    pwm_set_enabled(d3_slice, true);
}

void allLedOn() {
    pwm_set_gpio_level(D1_LED, brightness);
    pwm_set_gpio_level(D2_LED, brightness);
    pwm_set_gpio_level(D3_LED, brightness);
}

void allLedOff() {
    pwm_set_gpio_level(D1_LED, 0);
    pwm_set_gpio_level(D2_LED, 0);
    pwm_set_gpio_level(D3_LED, 0);
}

bool repeatingTimerCallback(struct repeating_timer *t) {

    // For SW1
    static uint button_state = 0, filter_counter = 0;
    uint new_state = 1;

    new_state = gpio_get(SW1);
    if (button_state != new_state) {
        if (++filter_counter >= BUTTON_FILTERING) {
            button_state = new_state;
            filter_counter = 0;
            if (new_state == RELEASED) {
                buttonreleased = true;
            }
        }
    } else {
        filter_counter = 0;
    }

    if (true == ledstate) {
        // For SW0
        if (!gpio_get(SW0)) { // increase
            if(brightness < 1000) {
                printf("Brightness: %d\n", brightness);
                brightness += 1;
            }
        }
        // For SW2
        if (!gpio_get(SW2)) { // decrease
            if(brightness > 0) {
                printf("Brightness to: %d\n", brightness);
                brightness -= 1;
            }
        }
    }

    return true;
}
