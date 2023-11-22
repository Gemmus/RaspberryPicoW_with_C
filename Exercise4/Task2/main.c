/*
Improve Exercise 1 by adding a persistent log that stores messages to EEPROM. When the program starts it writes “Boot” 
to the log and every time when state or LEDs is changed the state change message, as described in Exercise 1, is also written to the log.
 
The log must have the following properties:
    • Log starts from address 0 in the EEPROM.
    • First two kilobytes (2048 bytes) of EEPROM are used for the log.
    • Each log entry is reserved 64 bytes.
            o First entry is at address 0, second at address 64, third at address 128, etc.
            o Log can contain up to 32 entries.
    • A log entry consists of a string that contains maximum 61 characters, a terminating null character (zero) 
      and two-byte CRC that is used to validate the integrity of the data. A maximum length log entry uses all 64 bytes.
      A shorter entry will not use all reserved bytes. The string must contain at least one character.
    • When a log entry is written to the log, the string is written to the log including the terminating zero.
      Immediately after the terminating zero follows a 16-bit CRC, MSB first followed by LSB.
            o Entry is written to the first unused (invalid) location that is available.
            o If the log is full then the log is erased first and then entry is written to address 0.
    • User can read the content of the log by typing read and pressing enter.
            o Program starts reading and validating log entries starting from address zero. If a valid string
              is found it is printed and program reads string from the next location.
            o A string is valid if the first character is not zero, there is a zero in the string before index 62,
              and the string passes the CRC validation.
            o Printing stops when an invalid string is encountered or the end log are is reached.
    • User can erase the log by typing erase and pressing enter.
            o Erasing is done by writing a zero at the first byte of every log entry.
*/

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/i2c.h"
#include "hardware/irq.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"

                                        /////////////////////////////////////////////////////
                                        //                      MACROS                     //
                                        /////////////////////////////////////////////////////

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
#define MAX_LOG_SIZE 64
#define MAX_LOG_ENTRY 4 //should be 32
#define DEBUG_LOG_SIZE 6

                                        /////////////////////////////////////////////////////
                                        //             FUNCTION DECLARATIONS               //
                                        /////////////////////////////////////////////////////

void ledsInit();
void buttonsInit();
void i2cInit();
void pwmInit();
void ledOn(uint led_pin);
void ledOff(uint led_pin);
void ledsInitState();
void printState();
bool repeatingTimerCallback(struct repeating_timer *t);
void i2cWriteByte(uint16_t address, uint8_t data);
uint8_t i2cReadByte(uint16_t address);
uint16_t crc16(const uint8_t *data_p, size_t length);
void writeLogEntry(const char *message);
void printLog();
void eraseLog();

                                        /////////////////////////////////////////////////////
                                        //                GLOBAL VARIABLES                 //
                                        /////////////////////////////////////////////////////

volatile bool sw0_buttonEvent = false;
volatile bool sw1_buttonEvent = false;
volatile bool sw2_buttonEvent = false;

volatile bool d1State = false;
volatile bool d2State = false;
volatile bool d3State = false;

const uint8_t d1_address = I2C_MEMORY_SIZE - 1;
const uint8_t d2_address = I2C_MEMORY_SIZE - 2;
const uint8_t d3_address = I2C_MEMORY_SIZE - 3;

volatile uint log_counter = 0;

                                        /////////////////////////////////////////////////////
                                        //                     MAIN                        //
                                        /////////////////////////////////////////////////////

int main() {

    stdio_init_all();

    ledsInit();
    pwmInit();
    buttonsInit();
    i2cInit();

    printf("\nBoot\n");
    writeLogEntry("\nBoot\n");

    d1State = i2cReadByte(d1_address);
    d2State = i2cReadByte(d2_address);
    d3State = i2cReadByte(d3_address);

    /* Printing in the beginning: */
    for (int i = 0; i < MAX_LOG_ENTRY; i++) {
        for (int j = 0; j < DEBUG_LOG_SIZE; j++) {
            uint8_t printed = i2cReadByte(i * MAX_LOG_SIZE + j);
            printf("%c", printed);
        }
    }
    printf("\n");

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
            i2cWriteByte(d3_address,d3State);
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
            i2cWriteByte(d2_address,d2State);
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
            i2cWriteByte(d1_address,d1State);
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

                                        /////////////////////////////////////////////////////
                                        //                   FUNCTIONS                     //
                                        /////////////////////////////////////////////////////

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
    i2cWriteByte(d1_address,d1State);

    pwm_set_gpio_level(D2, BRIGHTNESS);
    d2State = true;
    i2cWriteByte(d2_address,d2State);

    pwm_set_gpio_level(D3, MIN_BRIGHTNESS);
    i2cWriteByte(d3_address,d3State);
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
    /* stdin inputs */

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

void i2cWriteByte(uint16_t address, uint8_t data) {
    uint8_t buffer[3];
    buffer[0] = address >> 8; buffer[1] = address; buffer[2] = data;
    i2c_write_blocking(i2c0, DEVADDR, buffer, sizeof(buffer), false);
    sleep_ms(10);
}

uint8_t i2cReadByte(uint16_t address) {
    uint8_t buffer[2];
    buffer[0] = address >> 8; buffer[1] = address;
    i2c_write_blocking(i2c0, DEVADDR, buffer, 2, true);
    i2c_read_blocking(i2c0, DEVADDR, buffer, 1, false);
    return buffer[0];
}

uint16_t crc16(const uint8_t *data_p, size_t length) {
    uint8_t x;
    uint16_t crc = 0xFFFF;

    while (length--) {
        x = crc >> 8 ^ *data_p++;
        x ^= x >> 4;
        crc = (crc << 8) ^((uint16_t) (x << 12)) ^ ((uint16_t) (x << 5)) ^ ((uint16_t) (x));
    }
    return crc;
}

void writeLogEntry(const char *message) {
    if (log_counter >= MAX_LOG_ENTRY) {
        eraseLog();
        log_counter = 0;
    }

    size_t message_length = strlen(message);
    if (message_length >= 1) {
        if (message_length > 61) {
            message_length = 61;
        }

        uint8_t buffer[message_length + 3];
        memcpy(buffer, message, message_length);
        buffer[message_length] = '\0';

        uint16_t crc = crc16(buffer, message_length);
        buffer[message_length + 1] = (uint8_t) (crc >> 8);
        buffer[message_length + 2] = (uint8_t) crc;

        uint16_t log_address = MAX_LOG_SIZE * log_counter++;
        for(int i = 0; i < (message_length + 3); i++) {
            i2cWriteByte(log_address++, buffer[i]);
        }
    } else {
        printf("Invalid input. Log message must contain at least one character.\n");
    }
}

void printLog() {
    if (0 != log_counter) {
        uint16_t log_address = 0;
        printf("Printing log messages from memory:\n");
        for(int i = 0; i <= log_counter; i++) {
            // we retrieve the message

            // we validate the message here
            // if valid print else print log[i + 1] invalid
        }
    } else {
        printf("No log message in memory.");
    }
}

void eraseLog() {
    printf("Erasing log messages from memory... ");
    uint16_t log_address = 0;
    for(int i = 0; i < MAX_LOG_ENTRY; i++) {
        i2cWriteByte(log_address, 0);
        log_address += MAX_LOG_SIZE;
    }
    printf(" done.\n");
}
