/*
 * SYSC 4001 Assignment 1
 * 
 * File: message_queue.h
 * Author: Brandon To
 * Student #: 100874049
 * Created: September 29, 2015
 *
 * Description:
 * Data related to message queue that is common to multiple processes.
 * 
 */

#define MESSAGE_QUEUE_ID 1234

#define TYPE_SENSOR_CONTROLLER 1
#define TYPE_CONTROLLER_SENSOR 2

struct sensor_controller_struct
{
    long type;
    char name[BUFSIZ];
    int threshold;
    int sensor_reading;
    pid_t pid;
};

struct controller_sensor_struct
{
    long type;
    char data[BUFSIZ];
};
