/*
Implement a program that switches LEDs on and off and remembers the state of the LEDs across reboot and/or power off.
The program should work as follows:
 • When the program starts it reads the state of the LEDs from EEPROM. If no valid state is found in the EEPROM
   the middle LED is switched on and the other two are switched off. The program must print number of seconds
   since power up and the state of the LEDs to stdout. Use time_us_64() to get a timestamp and convert that to seconds.

 • Each of the buttons SW0, SW1, and SW2 on the development board is associated with an LED. When user presses a button,
   the corresponding LED toggles. Pressing and holding the button may not make the LED to blink or to toggle multiple times.
   When state of the LEDs is changed the new state must be printed to stdout with a number of seconds since program was started.

 • When the state of an LEDs changes the program stores the state of all LEDs in the EEPROM and prints the state
   to LEDs to the debug UART. The program must employ a method to validate that settings read from the EEPROM are correct.

 • The program must use the highest possible EEPROM address to store the LED state.

A simple way to validate the LED state is store it to EEPROM twice: normally (un-inverted) and inverted.
When the state is read back both values are read, the inverted value is inverted after reading, and then the values are compared.
If the values match then LED state was stored correctly in the EEPROM. By storing an inverted value, we can avoid cases
where both bytes are identical, a typical case with erased/out of the box memory, to be accepted as a valid value.
*/

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/i2c.h"
#include "hardware/irq.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"

/*  LEDs  */
#define D1 22
#define D2 21
#define D3 20

/* BUTTONS */
#define SW_0 9
#define SW_1 8
#define SW_2 7
#define BUTTON_PERIOD 10
#define BUTTON_FILTER 5
#define SW0_RELEASED 1
#define SW1_RELEASED 1
#define SW2_RELEASED 1

/*   PWM   */
#define PWM_FREQ 1000
#define LEVEL 5
#define DIVIDER 125
#define BRIGHTNESS 200
#define MIN_BRIGHTNESS 0

/*   I2C   */
#define I2C0_SDA_PIN 16
#define I2C0_SCL_PIN 17
#define DEVADDR 0x50
#define BAUDRATE 100000
#define I2C_MEMORY_SIZE (2^15)

/* FUNCTIONS */
void ledsInit();
void buttonsInit();
void i2cInit();
void pwmInit();
void ledOn(uint led_pin);
void ledOff(uint led_pin);
void ledsInitState();
void printState();
bool repeatingTimerCallback(struct repeating_timer *t);
void i2c_write_byte(uint16_t address, uint8_t data);
uint8_t i2c_read_byte(uint16_t address);

/*  GLOBALS  */
volatile bool sw0_buttonEvent = false;
volatile bool sw1_buttonEvent = false;
volatile bool sw2_buttonEvent = false;

volatile bool d1State = false;
volatile bool d2State = false;
volatile bool d3State = false;

const uint8_t d1_address = I2C_MEMORY_SIZE - 1;
const uint8_t d2_address = I2C_MEMORY_SIZE - 2;
const uint8_t d3_address = I2C_MEMORY_SIZE - 3;

/*   MAIN   */
int main() {

    stdio_init_all();
    printf("\nBoot\n");

    ledsInit();
    pwmInit();
    buttonsInit();
    i2cInit();

    d1State = i2c_read_byte(d1_address);
    d2State = i2c_read_byte(d2_address);
    d3State = i2c_read_byte(d3_address);

    if ((d1State != 0 && d1State != 1) || (d2State != 0 && d2State!= 1) || (d3State != 0 && d3State != 1)) {
        ledsInitState();
    } else {
        if (true == d1State) {
            ledOn(D1);
        }
        if (true == d2State) {
            ledOn(D2);
        }
        if (true == d3State) {
            ledOn(D3);
        }
    }

    printState();

    struct repeating_timer timer;
    add_repeating_timer_ms(BUTTON_PERIOD, repeatingTimerCallback, NULL, &timer);

    while (true) {

        /* SW0 - D3 */
        if (sw0_buttonEvent) {
            sw0_buttonEvent = false;
            if(true == d3State) {
                d3State = false;
            } else {
                d3State = true;
            }
            i2c_write_byte(d3_address,d3State);
            printState();
        }

        if (true == d3State) {
            ledOn(D3);
        } else {
            ledOff(D3);
        }

        /* SW1 - D2 */
        if (sw1_buttonEvent) {
            sw1_buttonEvent = false;
            if(true == d2State) {
                d2State = false;
            } else {
                d2State = true;
            }
            i2c_write_byte(d2_address,d2State);
            printState();
        }

        if (true == d2State) {
            ledOn(D2);
        } else {
            ledOff(D2);
        }

        /* SW2 - D1 */
        if (sw2_buttonEvent) {
            sw2_buttonEvent = false;
            if(true == d1State) {
                d1State = false;
            } else {
                d1State = true;
            }
            i2c_write_byte(d1_address,d1State);
            printState();
        }

        if (true == d1State) {
            ledOn(D1);
        } else {
            ledOff(D1);
        }
    }

    return 0;
}

void ledsInit() {
    gpio_init(D3);
    gpio_set_dir(D3, GPIO_OUT);
    gpio_init(D2);
    gpio_set_dir(D2, GPIO_OUT);
    gpio_init(D1);
    gpio_set_dir(D1, GPIO_OUT);
}

void buttonsInit() {
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

    pwm_set_gpio_level(D1, MIN_BRIGHTNESS);
    pwm_set_gpio_level(D2, MIN_BRIGHTNESS);
    pwm_set_gpio_level(D3, MIN_BRIGHTNESS);
}

void i2cInit() {
    i2c_init(i2c0, BAUDRATE);
    gpio_set_function(I2C0_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C0_SCL_PIN, GPIO_FUNC_I2C);
}

void ledOn(uint led_pin) {
    pwm_set_gpio_level(led_pin, BRIGHTNESS);
}

void ledOff(uint led_pin) {
    pwm_set_gpio_level(led_pin, MIN_BRIGHTNESS);
}

void ledsInitState() {
    pwm_set_gpio_level(D1, MIN_BRIGHTNESS);
    i2c_write_byte(d1_address,d1State);

    pwm_set_gpio_level(D2, BRIGHTNESS);
    d2State = true;
    i2c_write_byte(d2_address,d2State);

    pwm_set_gpio_level(D3, MIN_BRIGHTNESS);
    i2c_write_byte(d3_address,d3State);
}

void printState() {
    printf("%llus since power up.\n", time_us_64() / 1000000);
    if (true == d1State) {
        printf("D1: on\n");
    } else {
        printf("D1: off\n");
    }
    if (true == d2State) {
        printf("D2: on\n");
    } else {
        printf("D2: off\n");
    }
    if (true == d3State) {
        printf("D3: on\n\n");
    } else {
        printf("D3: off\n\n");
    }
}

bool repeatingTimerCallback(struct repeating_timer *t) {
    /* SW0 */
    static uint sw0_button_state = 0, sw0_filter_counter = 0;
    uint sw0_new_state = 1;

    sw0_new_state = gpio_get(SW_0);
    if (sw0_button_state != sw0_new_state) {
        if (++sw0_filter_counter >= BUTTON_FILTER) {
            sw0_button_state = sw0_new_state;
            sw0_filter_counter = 0;
            if (sw0_new_state != SW0_RELEASED) {
                sw0_buttonEvent = true;
            }
        }
    } else {
        sw0_filter_counter = 0;
    }

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

    /* SW2 */
    static uint sw2_button_state = 0, sw2_filter_counter = 0;
    uint sw2_new_state = 1;

    sw2_new_state = gpio_get(SW_2);
    if (sw2_button_state != sw2_new_state) {
        if (++sw2_filter_counter >= BUTTON_FILTER) {
            sw2_button_state = sw2_new_state;
            sw2_filter_counter = 0;
            if (sw2_new_state != SW2_RELEASED) {
                sw2_buttonEvent = true;
            }
        }
    } else {
        sw2_filter_counter = 0;
    }

    return true;
}

void i2c_write_byte(uint16_t address, uint8_t data) {
    uint8_t buffer[3];
    buffer[0] = address >> 8; buffer[1] = address; buffer[2] = data;
    i2c_write_blocking(i2c0, DEVADDR, buffer, sizeof(buffer), false);
    sleep_ms(10);
}

uint8_t i2c_read_byte(uint16_t address) {
    uint8_t buffer[2];
    buffer[0] = address >> 8; buffer[1] = address;
    i2c_write_blocking(i2c0, DEVADDR, buffer, 2, true);
    i2c_read_blocking(i2c0, DEVADDR, buffer, 1, false);
    return buffer[0];
}
