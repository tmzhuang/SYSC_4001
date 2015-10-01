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
#include <errno.h>
#include <unistd.h>

#include <sys/time.h>
#include <sys/msg.h>

#include "message_queue.h"

#define DEFAULT_MAX_READING 100
#define DEFAULT_THRESHOLD 90

int main(int argc, char* argv[])
{
    pid_t pid = getpid();
    int msgid;

    char *name;
    int threshold = DEFAULT_THRESHOLD;
    int max_reading = DEFAULT_MAX_READING;
    int sensor_reading;

    int running = 1;
    struct timeval t1, t2;

    struct message_struct tx_data;
    struct message_struct rx_data;
    int tx_data_size = sizeof(struct message_struct) - sizeof(long);
    int rx_data_size = sizeof(struct message_struct) - sizeof(long);

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

    printf("Device starting. PID=%d\n", pid);

    // Creates a message queue
    msgid = msgget((key_t)MESSAGE_QUEUE_ID, 0666 | IPC_CREAT);
    if (msgid == -1)
    {
        fprintf(stderr, "msgget failed with error: %d\n", errno);
        exit(EXIT_FAILURE);
    }

    // Initial message to send
    tx_data.type = TO_CONTROLLER;
    strncpy(tx_data.name, name, sizeof(tx_data.name));
    tx_data.device_type = DEVICE_TYPE_SENSOR;
    tx_data.threshold = threshold;
    tx_data.pid = pid;

    // Send initial message to controller
    printf("Attempting to establish connection with Controller...\n");
    if (msgsnd(msgid, (void *)&tx_data, tx_data_size, 0) == -1)
    {
        fprintf(stderr, "msgsnd failed\n");
        exit(EXIT_FAILURE);
    }

    // Receive acknowledgement message from controller
    if (msgrcv(msgid, (void *)&rx_data, rx_data_size,
                pid, 0) == -1)
    {
        fprintf(stderr, "msgrcv failed with error: %d\n", errno);
        exit(EXIT_FAILURE);
    }

    // Check if the message received is an ack
    if (strncmp(rx_data.data, "ack", 3) != 0)
    {
        fprintf(stderr, "Expected ack message but received non-ack message\n");
        exit(EXIT_FAILURE);
    }
    printf("Received ack message from Controller. Connection establish.\n");

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
            printf("Sensor reading = %d\n", sensor_reading);

            if (sensor_reading >= threshold)
            {
                printf("Sensor reading exceeded THRESHOLD(%d)!\n", threshold);
                running = 0;
            }

            // Constructs and sends update message to controller
            memset((void *)&tx_data, 0, sizeof(tx_data));
            tx_data.type = TO_CONTROLLER;
            tx_data.sensor_reading = sensor_reading;
            tx_data.pid = pid;

            if (msgsnd(msgid, (void *)&tx_data, tx_data_size, 0) == -1)
            {
                fprintf(stderr, "msgsnd failed\n");
                exit(EXIT_FAILURE);
            }

            // Poll for stop message
            int result = msgrcv(msgid, (void *)&rx_data, rx_data_size,
                        pid, IPC_NOWAIT);
            if (result == -1)
            {
                // Error code ENOMSG(42) corresponds to no message received
                if (errno != ENOMSG)
                {
                    fprintf(stderr, "msgrcv failed with error: %d\n", errno);
                    exit(EXIT_FAILURE);
                }
            }
            else if (result > 0)
            {
                // If a stop messge is received, stop the device
                if (strncmp(rx_data.data, "stop", 3) == 0)
                {
                    printf("Received stop command from Controller. Stopping device.\n");
                    break;
                }
            }

            // Make note of current time
            gettimeofday(&t1, NULL);
        }
    }

    exit(EXIT_SUCCESS);
}
