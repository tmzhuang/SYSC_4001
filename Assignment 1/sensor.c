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

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "Usage: sensor [NAME] [THRESHOLD]\n");
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}
