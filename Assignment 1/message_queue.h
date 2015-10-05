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

#define TO_CONTROLLER 1

#define MAX_NAME_LENGTH 64
#define MAX_DATA_LENGTH 512

#define DEVICE_TYPE_SENSOR 1
#define DEVICE_TYPE_ACTUATOR 2

struct message_struct
{
    long type;
    char name[MAX_NAME_LENGTH];
    char device_type;
    int threshold;
    int sensor_reading;
    pid_t pid;
    char data[MAX_DATA_LENGTH];
};

