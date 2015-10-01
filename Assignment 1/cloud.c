/*
 * SYSC 4001 Assignment 1
 *
 * File: cloud.c
 * Author: Brandon To
 * Student #: 100874049
 * Created: September 30, 2015
 *
 * Description:
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <sys/time.h>
#include <sys/msg.h>

int main(int argc, char* argv[])
{
    pid_t pid = getpid();

    char *name;

    int running = 1;
    struct timeval t1, t2;

    if (argc < 2)
    {
        fprintf(stderr, "Usage: sensor NAME [THRESHOLD] [MAX_READING]\n");
        exit(EXIT_FAILURE);
    }

    name = argv[1];

    printf("Cloud starting. PID=%d\n", pid);

    while (running);
}

