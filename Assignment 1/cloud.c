/*
 * SYSC 4001 Assignment 1
 *
 * File: cloud.c
 * Author: Brandon To
 * Student #: 100874049
 * Created: September 30, 2015
 *
 * Description:
 * Relays messages between the Controller and a mobile device. The
 * mobile device is combined with the Cloud for the purpose of this
 * assignment. The Cloud is the server side of a FIFO that connects to
 * clients (ie. the Controller). Echos any information received from
 * clients to stdout.
 *
 * A user can enter Get or Put commands. The Get command is used to
 * request data for a specific Sensor, whereas the Put command is used
 * to trigger an action for an Actuator. The command will be sent to
 * the parent part of the Controller process.
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/time.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "fifo.h"

int main(int argc, char* argv[])
{
    pid_t pid = getpid();

    char *name;

    int running = 1;
    int result;
    struct timeval t1, t2;

    int fifo_fd;

    if (argc < 2)
    {
        fprintf(stderr, "Usage: cloud NAME\n");
        exit(EXIT_FAILURE);
    }

    name = argv[1];

    printf("Cloud starting. PID=%d\n", pid);

    // Check for existance of fifo by attempting to access it
    if (access(FIFO_NAME, F_OK) == -1)
    {
        // Create the fifo is it does not exist
        result = mkfifo(FIFO_NAME, 0777);
        if (result != 0)
        {
            fprintf(stderr, "Could not create fifo %s\n", FIFO_NAME);
            exit(EXIT_FAILURE);
        }
    }

    // Opens the reading end of the fifo
    fifo_fd = open(FIFO_NAME, O_RDONLY);
    if (fifo_fd == -1)
    {
        fprintf(stderr, "open failed with error: %d\n", errno);
        exit(EXIT_FAILURE);
    }

    while (running)
    {

    }

    close(fifo_fd);
}

