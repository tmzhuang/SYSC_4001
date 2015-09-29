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

#define DEFAULT_MAX_READING 100
#define DEFAULT_THRESHOLD 90

int main(int argc, char* argv[])
{
    char *name;
    int threshold = DEFAULT_THRESHOLD;
    int max_reading = DEFAULT_MAX_READING;
    int sensor_reading;
    int running = 1;
    struct timeval t1, t2;

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

    // Initilize random number generator
    srand(time(NULL));

    gettimeofday(&t1, NULL);
    while (running)
    {
        gettimeofday(&t2, NULL);
        if ((t2.tv_sec - t1.tv_sec) >= 2)
        {
            sensor_reading = rand()%(max_reading+1);
            printf("%d\n", sensor_reading);
            if (sensor_reading >= threshold)
            {
                printf("Sensor reading exceeded THRESHOLD(%d)!\n", threshold);
                running = 0;
            }
            gettimeofday(&t1, NULL);
        }
    }

    exit(EXIT_SUCCESS);
}
