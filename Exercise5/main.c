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

/////////////////////////////////////////////////////
//                      MACROS                     //
/////////////////////////////////////////////////////
/*  STEP MOTOR  */
#define IN1 13
#define IN2 6
#define IN3 3
#define IN4 2
#define OPTOFORK 28
#define STEPS_PER_REVOLUTION 4096

/////////////////////////////////////////////////////
//             FUNCTION DECLARATIONS               //
/////////////////////////////////////////////////////
void stepperMotorInit();
void optoforkInit();
void optoFallingEdge();
void runMotor(const uint times);

/////////////////////////////////////////////////////
//                GLOBAL VARIABLES                 //
/////////////////////////////////////////////////////
static const uint turning_sequence[8][4] = {{1, 0, 0, 0},
                                            {1, 1, 0, 0},
                                            {0, 1, 0, 0},
                                            {0, 1, 1, 0},
                                            {0, 0, 1, 0},
                                            {0, 0, 1, 1},
                                            {0, 0, 0, 1},
                                            {1, 0, 0, 1}};
static volatile bool fallingEdge = false;
static volatile bool calibrated = false;
static volatile uint revolution_counter = 0;
static uint calibration_count = 0;
static volatile uint row = 0;

/////////////////////////////////////////////////////
//                     MAIN                        //
/////////////////////////////////////////////////////
int main() {

    stdio_init_all();

    stepperMotorInit();
    optoforkInit();

    gpio_set_irq_enabled_with_callback(OPTOFORK, GPIO_IRQ_EDGE_FALL, true, optoFallingEdge);

    char command[8];
    memset(command, 0, sizeof(command));
    char character;
    int index = 0;

    while (true) {

        character = getchar_timeout_us(0);
        while (character != 255) {
            command[index++] = character;

            if (index == 7 || character == '\n') {
                command[index - 1] = '\0';

                if (0 == strncmp("run", command, strlen("run"))) {
                    uint N_times = 8;
                    if (strlen(command) >= 5) {
                        sscanf(&command[strlen("run")], "%u", &N_times);
                    }
                    if (true == calibrated) {
                        runMotor(N_times * calibration_count / 8);
                    } else {
                        runMotor(N_times * STEPS_PER_REVOLUTION / 8);
                    }
                } else if (0 == strcmp("status", command)) {
                    if (true == calibrated) {
                        printf("Position: %u / %u \n", revolution_counter, calibration_count);
                    } else {
                        printf("Not available.\n");
                    }
                } else if (0 == strcmp("calib", command)) {
                    calibrated = false;
                    fallingEdge = false;
                    while (false == fallingEdge) {
                        runMotor(1);
                    }
                    fallingEdge = false;
                    while (false == fallingEdge) {
                        runMotor(1);
                    }
                    calibrated = true;
                    printf("Number of steps per revolution: %u\n", calibration_count);
                }

                memset(command, 0, sizeof(command));
                index = 0;
                break;
            }
            character = getchar_timeout_us(0);
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

void optoFallingEdge() {
    fallingEdge = true;
    if (false == calibrated) {
        calibration_count = revolution_counter;
    }
    revolution_counter = 0;
}

void runMotor(const uint times) {
    for(int i  = 0; i < times; i++) {
        gpio_put(IN1, turning_sequence[row][0]);
        gpio_put(IN2, turning_sequence[row][1]);
        gpio_put(IN3, turning_sequence[row][2]);
        gpio_put(IN4, turning_sequence[row++][3]);
        revolution_counter++;
        if (row >= 8) {
            row = 0;
        }
        sleep_ms(4);
    }
}
