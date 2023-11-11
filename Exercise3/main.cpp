/*
Implement a program that verifies that UART connection to LoRa module works and then reads and processes EUID from the module.
The program should work as follows:
1. Program waits for user to press SW_0. When user presses the button then program starts communication with the LoRa module.
2. Program sends command “AT” to module and waits for response for 500 ms. If no response is received or the response
   is not correct the program tries again up to five times. If no response is received after five attempts program prints
   “module not responding” and goes back to step 1. If response is received program prints “Connected to LoRa module”.
3. Program reads firmware version of the module and prints the result. If no response is received in 500 ms program prints
   “Module stopped responding” and goes back to step 1.
4. Program reads DevEui from the device. If no response is received in 500 ms program prints “Module stopped responding”
   and goes back to step 1. DevEui contains 8 bytes that the module outputs in hexadecimal separated by colons.
   The program must remove the colons between the bytes and convert the hexadecimal digits to lower case.
5. Go to step 1.

See page 10 in LoRa-E5 AT Command Specification_V1.0.pdf in the Oma workspace/Documents/Project for information about
the structure of the commands. See page 24 for details of the commands needed in this assignment.

Processing of DevEui is needed for registering the module to Metropolia LoRaWAN network.
The server needs the DevEui in the specified above format to register the device to the network.
In the project you need you register your device LoRaWAN to send data.

For example:
• Module sends DevEui:
          +ID: DevEui, 2C:F7:F1:20:32:30:A5:70
• Program prints:
          2cf7f1203230a570

Hints:
First implement a function that reads a string from UART with a timeout. The function should wait until
linefeed is received or timeout occurs. See implementation of uart_is_readable_within_us in
PicoSDK for the principle of implementing a timeout.

Step described above can also be considered states of a state machine. For example, the state machine
could be implemented as shown below:

switch(state) {
    case 1:
        // do what needs to be done in state1
        // to change state assign a new value to 'state'
        break;
    case 2:
        break;
    case 3:
        break;
    case 4:
        break;
}
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/irq.h"
#include "hardware/gpio.h"
#include "uart.h"
#include "hardware/pwm.h"

#define SW_0 9
#define BUTTON_PERIOD 10
#define BUTTON_FILTER 5
#define RELEASED 1

#define D1 22
#define D2 21
#define D3 20
#define BRIGHTNESS 20
#define MIN_BRIGHTNESS 0
#define PWM_FREQ 1000
#define LEVEL 5
#define DIVIDER 125

#if 0
#define UART_NR 0
#define UART_TX_PIN 0
#define UART_RX_PIN 1
#else
#define UART_NR 1
#define UART_TX_PIN 4
#define UART_RX_PIN 5
#endif

#define BAUD_RATE 9600
#define WAITING_TIME 500
#define MAX_COUNT 5

#define STRLEN 80

void buttonInit();
bool repeatingTimerCallback(struct repeating_timer *t);
void ledsInit();
void pwmInit();
void allLedsOn();
void allLedsOff();
void removeColonsAndLowercase(const char *input, char *output);

volatile bool buttonEvent = false;

int main(void) {

    stdio_init_all();

    buttonInit();
    ledsInit();
    pwmInit();

    uart_setup(UART_NR, UART_TX_PIN, UART_RX_PIN, BAUD_RATE);

    bool lo_ra_comm = false;
    bool firmware_version_read = false;
    bool DevEui_read = false;
    const char AT_command[] = "AT\r\n";
    const char AT_VER_command[] = "AT+VER\r\n";
    const char DevEui_command[] = "AT+ID=DevEui\r\n";
    char str[STRLEN];
    int pos = 0;

    struct repeating_timer timer;
    add_repeating_timer_ms(BUTTON_PERIOD, repeatingTimerCallback, NULL, &timer);

    while (true) {

        if (buttonEvent) {
            buttonEvent = false;
            if (true == lo_ra_comm) {
                lo_ra_comm = false;
                allLedsOff();
            } else {
                lo_ra_comm = true;
                allLedsOn();
            }
        }

        if (true == lo_ra_comm) {

            uint count = 0;

            // Using uart_is_readable_within_us:

            while (MAX_COUNT > count++) {
                const int uart_nr = UART_NR;
                const int us_waiting_time = WAITING_TIME * 1000;
                uart_send(uart_nr, AT_command);
                if(uart_is_readable_within_us((uart_inst_t *) &uart_nr, (uint32_t) us_waiting_time)) {
                    printf("Connected to LoRa module.\n");
                    firmware_version_read = true;
                    break;
                }
            }

            // Using sleep_ms:
/*
            while (MAX_COUNT > count++) {
                uart_send(UART_NR, AT_command);
                sleep_ms(WAITING_TIME);
                pos = uart_read(UART_NR, (uint8_t *) str, STRLEN - 1);
                if (pos > 0) {
                    printf("Connected to LoRa module.\n");
                    firmware_version_read = true;
                    pos = 0;
                    break;
                }
            }
*/

            if (MAX_COUNT == count) {
                printf("Module not responding.\n");
                lo_ra_comm = false;
                allLedsOff();
            }

            if (true == firmware_version_read) {
                uart_send(UART_NR, AT_VER_command);
                sleep_ms(WAITING_TIME);
                pos = uart_read(UART_NR, (uint8_t *) str, STRLEN - 1);
                if (pos > 0) {
                    str[pos] = '\0';
                    printf("%d, received: %s", time_us_32() / 1000, str);
                    pos = 0;
                    DevEui_read = true;
                    firmware_version_read = false;
                } else {
                    printf("Module stopped responding.\n");
                    lo_ra_comm = false;
                    allLedsOff();
                }
            }

            if (true == DevEui_read) {
                uart_send(UART_NR, DevEui_command);
                sleep_ms(WAITING_TIME);
                pos = uart_read(UART_NR, (uint8_t *) str, STRLEN - 1);
                if (pos > 0) {
                    str[pos] = '\0';
                    char modified_str[STRLEN];
                    char *devEui = strstr(str, "DevEui,");
                    devEui += strlen("DevEui,");
                    removeColonsAndLowercase(devEui, modified_str);
                    printf("%s\n", modified_str);
                    pos = 0;
                    DevEui_read = false;
                } else {
                    printf("Module stopped responding.\n");
                }
                lo_ra_comm = false;
                allLedsOff();
            }
        }
    }
}

void buttonInit() {
    gpio_init(SW_0);
    gpio_set_dir(SW_0, GPIO_IN);
    gpio_pull_up(SW_0);
}

void ledsInit() {
    gpio_init(D3);
    gpio_set_dir(D3, GPIO_OUT);
    gpio_init(D2);
    gpio_set_dir(D2, GPIO_OUT);
    gpio_init(D1);
    gpio_set_dir(D1, GPIO_OUT);
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

    allLedsOff();
}

void allLedsOn() {
    pwm_set_gpio_level(D1, BRIGHTNESS);
    pwm_set_gpio_level(D2, BRIGHTNESS);
    pwm_set_gpio_level(D3, BRIGHTNESS);
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

    new_state = gpio_get(SW_0);
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

void removeColonsAndLowercase(const char *input, char *output) {
    int inputLength = strlen(input);
    int outputIndex = 0;

    for (int i = 0; i < inputLength; i++) {
        if (isxdigit((unsigned char) input[i])) {
            output[outputIndex] = tolower(input[i]);
            outputIndex++;
        }
    }

    output[outputIndex] = '\0';
}
