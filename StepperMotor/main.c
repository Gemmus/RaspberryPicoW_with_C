#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/irq.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"

/*  LEDs  */
#define D2 21

/* BUTTONS */
#define SW_1 8
#define BUTTON_PERIOD 10
#define BUTTON_FILTER 5
#define SW1_RELEASED 1

/*   PWM   */
#define PWM_FREQ 1000
#define LEVEL 5
#define DIVIDER 125
#define BRIGHTNESS 200
#define MIN_BRIGHTNESS 0

/*  STEP MOTOR  */
#define IN1 13
#define IN2 6
#define IN3 3
#define IN4 2

/* FUNCTIONS */
void ledInit();
void buttonInit();
void pwmInit();
void ledOn(uint led_pin);
void ledOff(uint led_pin);
void stepperMotorInit();
bool repeatingTimerCallback(struct repeating_timer *t);

/*  GLOBALS  */
volatile bool sw1_buttonEvent = false;
volatile bool d2State = false;

/*   MAIN   */
int main() {

    stdio_init_all();

    ledInit();
    pwmInit();
    buttonInit();
    stepperMotorInit();

    struct repeating_timer timer;
    add_repeating_timer_ms(BUTTON_PERIOD, repeatingTimerCallback, NULL, &timer);

    const uint turning_sequence[8][4] = {{1, 0, 0, 0},
                                         {1, 1, 0, 0},
                                         {0, 1, 0, 0},
                                         {0, 1, 1, 0},
                                         {0, 0, 1, 0},
                                         {0, 0, 1, 1},
                                         {0, 0, 0, 1},
                                         {1, 0, 0, 1}};

    while (true) {

        /* SW1 - D2 */
        if (sw1_buttonEvent) {
            sw1_buttonEvent = false;
            if(true == d2State) {
                d2State = false;
            } else {
                d2State = true;
            }
        }

        if (true == d2State) {
            ledOn(D2);
            for (int i = 7; i >= 0; i--) {
                gpio_put(IN1, turning_sequence[i][0]);
                gpio_put(IN2, turning_sequence[i][1]);
                gpio_put(IN3, turning_sequence[i][2]);
                gpio_put(IN4, turning_sequence[i][3]);
                sleep_ms(10);
            }
        } else {
            ledOff(D2);
        }

    }

    return 0;
}

void ledInit() {
    gpio_init(D2);
    gpio_set_dir(D2, GPIO_OUT);
}

void buttonInit() {
    gpio_init(SW_1);
    gpio_set_dir(SW_1, GPIO_IN);
    gpio_pull_up(SW_1);
}

void pwmInit() {
    pwm_config config = pwm_get_default_config();

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

    pwm_set_gpio_level(D2, MIN_BRIGHTNESS);
}

void ledOn(uint led_pin) {
    pwm_set_gpio_level(led_pin, BRIGHTNESS);
}

void ledOff(uint led_pin) {
    pwm_set_gpio_level(led_pin, MIN_BRIGHTNESS);
}

void stepperMotorInit() {
    gpio_init(IN1);
    gpio_set_dir(IN1, GPIO_OUT);
    gpio_init(IN2);
    gpio_set_dir(IN2, GPIO_OUT);
    gpio_init(IN3);
    gpio_set_dir(IN3, GPIO_OUT);
    gpio_init(IN4);
    gpio_set_dir(IN4, GPIO_OUT);
}

bool repeatingTimerCallback(struct repeating_timer *t) {

    /* SW1 */
    static uint sw1_button_state = 0, sw1_filter_counter = 0;
    uint sw1_new_state = 1;

    sw1_new_state = gpio_get(SW_1);
    if (sw1_button_state != sw1_new_state) {
        if (++sw1_filter_counter >= BUTTON_FILTER) {
            sw1_button_state = sw1_new_state;
            sw1_filter_counter = 0;
            if (sw1_new_state != SW1_RELEASED) {
                sw1_buttonEvent = true;
            }
        }
    } else {
        sw1_filter_counter = 0;
    }

    return true;
}
