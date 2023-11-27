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
uint calibration_count = 0;

/////////////////////////////////////////////////////
//                     MAIN                        //
/////////////////////////////////////////////////////
int main() {

    stdio_init_all();

    stepperMotorInit();
    optoforkInit();

    gpio_set_irq_enabled_with_callback(OPTOFORK, GPIO_IRQ_EDGE_FALL, true, optoFallingEdge);

    while (true) {

        char command[6];
        int retval = scanf("%s", command);
        if (1 == retval) {
            if (strcmp(command, "status") == 0) {
                if (true == calibrated) {
                    printf("The number of steps per revolution: %d\n", calibration_count);
                } else {
                    printf("Not available.\n");
                }
            } else if (strcmp(command, "calib") == 0) {
                calibrated = false;
                fallingEdge = false;
                while (false == fallingEdge) {
                    runMotor(1);
                }
                revolution_counter = 0;
                fallingEdge = false;
                while (false == fallingEdge) {
                    runMotor(1);
                }
                calibrated = true;
                calibration_count = revolution_counter;
            } else if (strcmp(command, "run") == 0) {
                int N_times = STEPS_PER_REVOLUTION / 8;
                uint num = STEPS_PER_REVOLUTION / 8;
                num = getchar_timeout_us(10);
                if (num >= 0) {
                    printf("Not timed out %d", num);
                }
                else {
                    printf("Timed out %d", num);
                }
#if 0

                if (1 == scanf("%d", &N_times)) {
                    if (N_times > 0) {
                        printf("%d", N_times);
                        runMotor( N_times * STEPS_PER_REVOLUTION / 64 );
                    }
                }
#endif
                //runMotor(N_times);
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

void optoFallingEdge() {
    fallingEdge = true;
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
            sleep_ms(5);
        }
    }
}
