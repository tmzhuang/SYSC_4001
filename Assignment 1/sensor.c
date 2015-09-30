/*
 * SYSC 4001 Assignment 1
 * 
 * File: sensor.c
 * Author: Brandon To
 * Student #: 100874049
 * Created: September 29, 2015
 *
 * Description:
 * Performs a simple monitoring task periodically and sends the
 * results to the Controller via the message queue. If a threshold
 * crossing is observed, the Controller needs to take the appropriate
 * action. See description of the Controller in the controller.c file.
 * 
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "message_queue.h"

#define DEFAULT_MAX_READING 100
#define DEFAULT_THRESHOLD 90

int main(int argc, char* argv[])
{
    pid_t pid = getpid();
    char *name;
    int threshold = DEFAULT_THRESHOLD;
    int max_reading = DEFAULT_MAX_READING;
    int sensor_reading;
    int running = 1;
    struct timeval t1, t2;
    struct sensor_controller_struct tx_struct;
    struct controller_sensor_struct rx_struct;

    if (argc < 2)
    {
        fprintf(stderr, "Usage: sensor NAME [THRESHOLD] [MAX_READING]\n");
        exit(EXIT_FAILURE);
    }

    name = argv[1];

    if (argc > 2)
    {
        threshold = atoi(argv[2]);
    }

    if (argc > 3)
    {
        max_reading = atoi(argv[3]);
    }

    if (threshold > max_reading)
    {
        fprintf(stderr, "THRESHOLD(%d) must not exceed MAX_READING(%d)\n", threshold, max_reading);
        exit(EXIT_FAILURE);
    }

    // Initial struct
    tx_struct.type = TYPE_SENSOR_CONTROLLER;
    strncpy(tx_struct.name, name, sizeof(tx_struct.name));
    tx_struct.threshold = threshold;
    tx_struct.pid = pid;

    // Initilize random number generator
    srand(time(NULL));

    // Make note of current time
    gettimeofday(&t1, NULL);
    while (running)
    {
        // Get current time
        gettimeofday(&t2, NULL);

        // Compare current time to previously noted time and enter block
        // if the time difference is greater or equal than 2 seconds
        if ((t2.tv_sec - t1.tv_sec) >= 2)
        {
            // Generate a random number between 0 and MAX_READING
            sensor_reading = rand()%(max_reading+1);
            printf("%d\n", sensor_reading);

            if (sensor_reading >= threshold)
            {
                printf("Sensor reading exceeded THRESHOLD(%d)!\n", threshold);
                running = 0;
            }

            // Make note of current time
            gettimeofday(&t1, NULL);
        }
    }

    exit(EXIT_SUCCESS);
}
