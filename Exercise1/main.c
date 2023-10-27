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

void ledInitalizer(const uint led);
void buttonInitializer(const uint button);
bool buttonPressed(const uint gpio);

int main(void) {

    const uint led21 = 21;
    const uint sw0 = 9;
    const uint sw1 = 8;
    const uint sw2 = 7;
    uint blink_count = 0;
    uint state = 0; // for SW1 ON-OFF --> ON = 1; OFF = 0;

    // Initialize input output
    stdio_init_all();

    // Initialize LED pin
    ledInitalizer(led21);

    // Initialize LED pin
    buttonInitializer(sw0);
    buttonInitializer(sw1);
    buttonInitializer(sw2);

    // Loop forever
    while (true){

        if  (state == 1) {
            printf("Blinking! %u\r\n", ++blink_count);
            gpio_put(led21, true);
            sleep_ms(1000);
            gpio_put(led21, false);
            sleep_ms(1000);
        } else {
            gpio_put(led21, false);
            sleep_ms(1000);
        }

        if (!gpio_get(sw1)) {
            bool changed = buttonPressed(sw1);
            if (true == changed) {
                if (state == 0) {
                    state = 1;
                } else {
                    state = 0;
                }
            }
        }

    }
}

void ledInitalizer(const uint led) {
    gpio_init(led);
    gpio_set_dir(led, GPIO_OUT);
}

void buttonInitializer(const uint button) {
    gpio_init(button);
    gpio_set_dir(button, GPIO_IN);
    gpio_pull_up(button);
}

bool buttonPressed(const uint gpio) {
    int press = 0;
    int release = 0;

    while(press < 2 && release < 2) {
        if(!gpio_get(gpio)) {
            press++;
            release = 0;
        } else {
            release++;
            press = 0;
        }
        sleep_ms(10);
    }

    if(press > release) {
        return true;
    } else {
        return false;
    }
}
