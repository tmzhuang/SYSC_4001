/*
 * SYSC 4001 Assignment 1
 * 
 * File: controller.c
 * Author: Brandon To
 * Student #: 100874049
 * Created: September 29, 2015
 *
 * Description:
 * The controller is used to control and coordinate other processes.
 * It consists of a parent and child process.
 *
 * The child process communicates with various Device processes
 * (Sensors and Actuator processes) via a message queue. The child
 * process receives messages periodically from Device processes. If
 * the message received is the first message from a particular
 * Device process, it registers the device and all the necessary
 * information about it. Each subsequent message from a Device
 * process contains the sender information. The child process will
 * echo any information it receives from any process. If the value
 * from a Sensor proess exeeds the pre-configured threshold, the
 * child process will trigger a action by sending a command to an
 * Actuator process. The Actuator echos the message it receives. The
 * child also raises a signal and sends it to the parent process,
 * indicating the parent to read a specific mmessage from the
 * message queue. The message contains information for the sensor,
 * sensing data, and the action.
 *
 * The parent process communicates witht he Cloud process using a
 * FIFO. The Clound is the server and the parent is a clent. The
 * parent should relay any information received from the client to
 * the Cloud process.
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include <sys/msg.h>

#include "message_queue.h"

void child_handler(void);
void parent_handler(void);

int main(int argc, char* argv[])
{
    pid_t pid;

    char *name;

    if (argc < 2)
    {
        fprintf(stderr, "Usage: controller NAME\n");
        exit(EXIT_FAILURE);
    }

    name = argv[1];

    // Fork the process into child and parent process
    pid = fork();

    switch (pid)
    {
    case -1:
        fprintf(stderr, "fork failed\n");
    case 0:
        // Child process
        child_handler();
        printf("We're in the child process!\n");
        break;
    default:
        // Parent process
        printf("We're in the parent process!\n");
        parent_handler();
        break;
    }

    exit(EXIT_SUCCESS);
}

void child_handler(void)
{
    /*struct sensor_controller_struct rx_data;
    struct controller_sensor_struct tx_data;
    int tx_data_size = sizeof(struct sensor_controller_struct) - sizeof(long);
    int rx_data_size = sizeof(struct controller_sensor_struct) - sizeof(long);


    // Creates a message queue
    msgid = msgget((key_t)MESSAGE_QUEUE_ID, 0666| IPC_CREAT);
    if (msgid == -1)
    {
        fprintf(stderr, "msgget failed with error: %d\n", errno);
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        if (msgrcv)
        {

        }

    }*/
}

void parent_handler(void)
{

}
