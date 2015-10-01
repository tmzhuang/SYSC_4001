/*
 * SYSC 4001 Assignment 1
 *
 * File: actuator.c
 * Author: Brandon To
 * Student #: 100874049
 * Created: September 30, 2015
 *
 * Description:
 * Periodically checks the message queue to see if there is a command
 * specific for itself sent by the Controller. It reads any messages
 * and triggers a motion/action and sends a response back to the
 * Controller immediately after the operation.
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <sys/time.h>
#include <sys/msg.h>

#include "message_queue.h"

int main(int argc, char* argv[])
{
    pid_t pid = getpid();
    int msgid;

    char *name;

    int running = 1;
    struct timeval t1, t2;

    struct message_struct tx_data;
    struct message_struct rx_data;
    int tx_data_size = sizeof(struct message_struct) - sizeof(long);
    int rx_data_size = sizeof(struct message_struct) - sizeof(long);

    if (argc < 2)
    {
        fprintf(stderr, "Usage: actuator NAME\n");
        exit(EXIT_FAILURE);
    }

    name = argv[1];

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
    tx_data.device_type = DEVICE_TYPE_ACTUATOR;
    tx_data.pid = pid;

    // Send initial message to controller
    printf("Attempting to establish connection with controller...\n");
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
    printf("Connection establish.\n");

    // Check if the message received is an ack
    if (strncmp(rx_data.data, "ack", 3) != 0)
    {
        fprintf(stderr, "Expected ack message but received non-ack message\n");
        exit(EXIT_FAILURE);
    }

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

            if (msgrcv(msgid, (void *)&rx_data, rx_data_size,
                        pid, 0) == -1)
            {
                fprintf(stderr, "msgrcv failed with error: %d\n", errno);
                exit(EXIT_FAILURE);
            }

            // If a stop messge is received, stop the device
            if (strncmp(rx_data.data, "stop", 4) == 0)
            {
                printf("Received stop command from controller... stopping device.\n");
                break;
            }

            printf("Received '%s' from controller\n", rx_data.data);

            // TODO (Brandon): Send response back to the Controller

            // Make note of current time
            gettimeofday(&t1, NULL);
        }
    }

    exit(EXIT_SUCCESS);
}
