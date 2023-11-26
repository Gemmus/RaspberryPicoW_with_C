/*
In this exercise you need to calibrate the stepper motor position and count the number of steps per revolution by using
an optical sensor. The reducer gearing ratio is not exactly 1:64 so the number of steps per revolution is not exactly
4096 steps with half-stepping. By calibrating motor position and the number of steps per revolution we can get better accuracy.

Implement a program that reads commands from standard input. The commands to implement are:
    • status – prints the state of the system:
            o Is it calibrated?
            o Number of steps per revolution or “not available” if not calibrated
    • calib – perform calibration
            o Calibration should run the motor in one direction until a falling edge is seen in the opto fork input and
              then count number of steps to the next falling edge. To get more accurate results count the number of steps
              per revolution three times and take the average of the values.
    • run N – N is an integer that may be omitted. Runs the motor N times 1/8th of a revolution. If N is omitted run one
      full revolution. “Run 8” should also run one full revolution.
*/

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/irq.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"

/////////////////////////////////////////////////////
//                      MACROS                     //
/////////////////////////////////////////////////////

/*    LEDs    */
#define D1 22
#define D2 21
#define D3 20

/*   BUTTONS   */
#define SW_0 9
#define SW_1 8
#define SW_2 7
#define BUTTON_PERIOD 10
#define BUTTON_FILTER 5
#define SW0_RELEASED 1
#define SW1_RELEASED 1
#define SW2_RELEASED 1

/*     PWM      */
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
#define OPTOFORK 28
#define PIEZO 27
#define STEPS_PER_REVOLUTION 4096
#define CALIBRATION_RUNS 3

/*     LoRaWAN     */
#define UART_NR 1
#define UART_TX_PIN 4
#define UART_RX_PIN 5
#define BAUD_RATE 9600

/*   I2C   */
#define I2C0_SDA_PIN 16
#define I2C0_SCL_PIN 17
#define DEVADDR 0x50
#define BAUDRATE 100000
#define I2C_MEMORY_SIZE 32768
#define MAX_LOG_SIZE 64
#define MAX_LOG_ENTRY 32
#define DEBUG_LOG_SIZE 6

/////////////////////////////////////////////////////
//             FUNCTION DECLARATIONS               //
/////////////////////////////////////////////////////
void stepperMotorInit();
void optoforkInit();
void piezoInit();
void runMotor(const uint times);
void optoFallingEdge();

/////////////////////////////////////////////////////
//                GLOBAL VARIABLES                 //
/////////////////////////////////////////////////////
const uint turning_sequence[8][4] = {{1, 0, 0, 0},
                                     {1, 1, 0, 0},
                                     {0, 1, 0, 0},
                                     {0, 1, 1, 0},
                                     {0, 0, 1, 0},
                                     {0, 0, 1, 1},
                                     {0, 0, 0, 1},
                                     {1, 0, 0, 1}};
volatile bool fallingEdge = false;
volatile bool calibrated = false;
volatile uint revolution_counter = 0;

/////////////////////////////////////////////////////
//                     MAIN                        //
/////////////////////////////////////////////////////
int main() {

    stdio_init_all();

    stepperMotorInit();
    optoforkInit();
    piezoInit();

    gpio_set_irq_enabled_with_callback(OPTOFORK, GPIO_IRQ_EDGE_FALL, true, optoFallingEdge);

    while (true) {

        char command[10];
        int retval = scanf("%s", command);
        if (1 == retval) {
            if (strcmp(command, "status") == 0) {
                if(true == calibrated) {
                    printf("The number of steps per revolution: %d\n", revolution_counter);
                } else {
                    printf("Not available.\n");
                }
            } else if (strcmp(command, "calib") == 0){
                calibrated = false;
                while (false == calibrated) {
                        runMotor(1);
                        revolution_counter = 0;
                }
            } else if (strcmp(command, "run") == 0) {
                int N_times = STEPS_PER_REVOLUTION / 8;
                if (1 == scanf("%d", &N_times)) {
                    if (N_times > 0) {
                        printf("%d", N_times);
                        runMotor( N_times * STEPS_PER_REVOLUTION / 64 );
                    }
                }
                runMotor(N_times);
                fflush(stdin);
            }
        }
    }

    return 0;
}

/////////////////////////////////////////////////////
//                   FUNCTIONS                     //
/////////////////////////////////////////////////////
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

void optoforkInit() {
    gpio_init(OPTOFORK);
    gpio_set_dir(OPTOFORK, GPIO_IN);
    gpio_pull_up(OPTOFORK);
}

void piezoInit() {
    gpio_init(PIEZO);
    gpio_set_dir(PIEZO, GPIO_IN);
    gpio_pull_up(PIEZO);
}

void runMotor(const uint times) {
    for (int i = 0; i <= times; i++) {
        for (int j = 7; j >= 0; j--) {
            gpio_put(IN1, turning_sequence[j][0]);
            gpio_put(IN2, turning_sequence[j][1]);
            gpio_put(IN3, turning_sequence[j][2]);
            gpio_put(IN4, turning_sequence[j][3]);
            if (false == fallingEdge) {
                revolution_counter++;

            }
            printf("Rev number = %d\n", revolution_counter);
            sleep_ms(15);
        }
    }
}

void optoFallingEdge() {
    fallingEdge = false;
    calibrated = true;
}
