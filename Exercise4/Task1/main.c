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
#include "hardware/i2c.h"

#define I2C0_SDA_PIN 16
#define I2C0_SCL_PIN 17

#define DEVADDR 0x20

int main() {

    const uint led_pin = 22;

    gpio_init(led_pin);
    gpio_set_dir(led_pin, GPIO_OUT);

    stdio_init_all();
    printf("\nBoot\n");

    i2c_init(i2c0, 100000);
    gpio_set_function(I2C0_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C0_SCL_PIN, GPIO_FUNC_I2C);

    uint8_t buf[8] = {0x00, 0x20};
    i2c_write_blocking(i2c0, DEVADDR, buf, 2, false);

    buf[0] = 0x12;
    buf[1] = 0x01;
    i2c_write_blocking(i2c0, DEVADDR, buf, 2, false);

    buf[0] = 0x0C;
    buf[1] = 0x20;
    i2c_write_blocking(i2c0, DEVADDR, buf, 2, false);

    while (true) {
        uint8_t value;
        buf[0] = 0x12;
        i2c_write_blocking(i2c0, DEVADDR, buf, 1, true);
        i2c_read_blocking(i2c0, DEVADDR, &value, 1, false);

        // Blink LED
        printf("value:  %02X\r\n", value & 0x20);
        gpio_put(led_pin, true);
        buf[0] = 0x12;
        buf[1] = 0x81;
        i2c_write_blocking(i2c0, DEVADDR, buf, 2, false);
        //printf("value:  %02X\r\n", value & 0x20);
        sleep_ms(500);
        gpio_put(led_pin, false);
        buf[0] = 0x12;
        buf[1] = 0x80;
        i2c_write_blocking(i2c0, DEVADDR, buf, 2, false);
        sleep_ms(500);

    }

    return 0;
}
