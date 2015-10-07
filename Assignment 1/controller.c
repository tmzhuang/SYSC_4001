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
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

#include <sys/msg.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "message_queue.h"
#include "fifo.h"
#include "queue.c"

#define MAX_DEVICES 256

struct device_info
{
    pid_t pid;
    char name[MAX_NAME_LENGTH];
    char device_type;
    int threshold;
    int actuator_index;
};

void child_handler(void);
int get_device_index(pid_t pid, struct device_info *devices, int size);
int get_actuator_index(pid_t pid, struct device_info *devices, int size);

void parent_handler(void);
void get_message(int signal_number);

sig_atomic_t get_message_flag = 0;

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

    printf("Controller starting.\n");

    // Creates a message queue
    int msgid = msgget((key_t)MESSAGE_QUEUE_ID, 0666 | IPC_CREAT);
    if (msgid == -1)
    {
        fprintf(stderr, "msgget failed with error: %d\n", errno);
        exit(EXIT_FAILURE);
    }

    printf("[CONTROLLER] Flushing message queue\n");

    // Flush message queue
    if (msgctl(msgid, IPC_RMID, 0) == -1)
    {
        fprintf(stderr, "msgctl failed with error: %d\n", errno);
        exit(EXIT_FAILURE);
    }

    // Fork the process into child and parent process
    pid = fork();

    // Switch according to the pid returned from fork()
    switch (pid)
    {
    case -1:
        fprintf(stderr, "fork failed\n");
        exit(EXIT_FAILURE);
    case 0:
        // Child process
        child_handler();
        break;
    default:
        // Parent process
        parent_handler();
        break;
    }

    exit(EXIT_SUCCESS);
}

void child_handler(void)
{
    pid_t pid = getpid();
    pid_t ppid = getppid();
    int msgid;
    int sequence_number = 1;

    struct device_info devices[MAX_DEVICES];
    struct queue *unmapped_sensor_index_queue = queue_create();
    struct queue *unmapped_actuator_index_queue = queue_create();;
    int current_devices_index = 0;

    int result;

    struct message_struct rx_data;
    struct message_struct tx_data;
    int tx_data_size = sizeof(struct message_struct) - sizeof(long);
    int rx_data_size = sizeof(struct message_struct) - sizeof(long);

    memset(devices, 0, sizeof(devices));

    printf("[CHILD] Started with PID=%d\n", pid);

    // Creates a message queue
    msgid = msgget((key_t)MESSAGE_QUEUE_ID, 0666 | IPC_CREAT);
    if (msgid == -1)
    {
        fprintf(stderr, "[CHILD] msgget failed with error: %d\n", errno);
        exit(EXIT_FAILURE);
    }

    printf("[CHILD] Ready to receive messages\n");

    // No actuators mapped to any sensors yet
    for (int i=0; i<MAX_DEVICES; i++)
    {
        devices[i].actuator_index = -1;
    }

    while (1)
    {
        // Block until a message is received
        if (msgrcv(msgid, (void *)&rx_data, rx_data_size,
                    TO_CONTROLLER, 0) == -1)
        {
            fprintf(stderr, "PID=%d [CHILD] msgrcv failed with error: %d\n", pid, errno);
            exit(EXIT_FAILURE);
        }

        // Register device if it hasn't been registered yet
        int received_device_index = get_device_index(rx_data.fields.pid, devices, MAX_DEVICES);
        //printf("received_device_index = %d\n", received_device_index);
        if (received_device_index == -1)
        {
            devices[current_devices_index].pid = rx_data.fields.pid;
            strncpy(devices[current_devices_index].name, rx_data.fields.name, sizeof(devices[current_devices_index].name));
            devices[current_devices_index].device_type = rx_data.fields.device_type;
            devices[current_devices_index].threshold = rx_data.fields.threshold;
            devices[current_devices_index].actuator_index = -1;

            // Map Actuator to available Sensor
            if (rx_data.fields.device_type == DEVICE_TYPE_ACTUATOR)
            {
                printf("[CHILD] Actuator with PID=%d is now registered!\n", rx_data.fields.pid);
                int unmapped_sensor_index;
                int result = queue_remove(unmapped_sensor_index_queue, &unmapped_sensor_index);
                if (result == -1)
                {
                    printf("[CHILD] There are no available Sensors at the moment. Queuing up Actuator to be mapped to next available Sensor.\n");
                    queue_add(unmapped_actuator_index_queue, current_devices_index);
                }
                else
                {
                    printf("[CHILD] Actuator successfully mapped to available Sensor.\n");
                    devices[unmapped_sensor_index].actuator_index = current_devices_index;
                }
            }
            // Map Sensor to available Actuator
            else if (rx_data.fields.device_type == DEVICE_TYPE_SENSOR)
            {
                printf("[CHILD] Sensor with PID=%d is now registered!\n", rx_data.fields.pid);
                int unmapped_actuator_index;
                int result = queue_remove(unmapped_actuator_index_queue, &unmapped_actuator_index);
                if (result == -1)
                {
                    printf("[CHILD] There are no available Actuators at the moment. Queuing up Sensor to be mapped to next available Actuator.\n");
                    queue_add(unmapped_sensor_index_queue, current_devices_index);
                }
                else
                {
                    printf("[CHILD] Actuator successfully mapped to available Sensor.\n");
                    devices[current_devices_index].actuator_index = unmapped_actuator_index;
                }
            }

            current_devices_index++;

            // Constructs and sends an acknowledgement message to device
            memset((void *)&tx_data, 0, sizeof(tx_data));
            tx_data.type = rx_data.fields.pid;
            strncpy(tx_data.fields.data, "ack", sizeof(tx_data.fields.data));

            printf("[CHILD] Sending ack to Device with PID=%d\n", rx_data.fields.pid);
            if (msgsnd(msgid, (void *)&tx_data, tx_data_size, 0) == -1)
            {
                fprintf(stderr, "[CHILD] msgsnd failed\n");
                exit(EXIT_FAILURE);
            }

            continue;
        }

        // Threshold field is being multiplexed as sequence number
        if (rx_data.fields.device_type == DEVICE_TYPE_ACTUATOR)
        {
            printf("[CHILD] Received ack from Actuator with PID=%d and Sequence#=%d\n",
                    (int)rx_data.fields.pid, rx_data.fields.threshold);
            continue;
        }

        printf("[CHILD] Received reading of %d from PID=%d\n", rx_data.fields.sensor_reading, rx_data.fields.pid);

        if (rx_data.fields.sensor_reading >= devices[received_device_index].threshold)
        {
            // Find index of mapped Actuator
            int actuator_index = get_actuator_index(rx_data.fields.pid, devices, MAX_DEVICES);

            if (actuator_index == -1)
            {
                printf("[CHILD] This Sensor is not currently mapped to any Actuators.\n");
                continue;
            }

            // Constructs and sends a command message to an actuator
            memset((void *)&tx_data, 0, sizeof(tx_data));
            tx_data.type = devices[actuator_index].pid;
            tx_data.fields.threshold = sequence_number; // Threshold field multiplex as sequence number
            strncpy(tx_data.fields.data, "turn off", sizeof(tx_data.fields.data));

            printf("[CHILD] Sending command to Actuator with PID=%d and Sequence#=%d\n",
                    (int)tx_data.type, sequence_number++);
            if (msgsnd(msgid, (void *)&tx_data, tx_data_size, 0) == -1)
            {
                fprintf(stderr, "[CHILD] msgsnd failed\n");
                exit(EXIT_FAILURE);
            }

            // Constructs and sends an update message to the parent
            memset((void *)&tx_data, 0, sizeof(tx_data));
            tx_data.type = ppid;
            strncpy(tx_data.fields.name, devices[received_device_index].name, sizeof(tx_data.fields.name));
            tx_data.fields.threshold = devices[received_device_index].threshold;
            tx_data.fields.sensor_reading = rx_data.fields.sensor_reading;
            tx_data.fields.pid = rx_data.fields.pid;
            strncpy(tx_data.fields.data, "turn off", sizeof(tx_data.fields.data));

            printf("[CHILD] Sending update to parent\n");
            if (msgsnd(msgid, (void *)&tx_data, tx_data_size, 0) == -1)
            {
                fprintf(stderr, "[CHILD] msgsnd failed\n");
                exit(EXIT_FAILURE);
            }

            // Raise signal for parent process
            kill(ppid, SIGUSR1);
        }
    }
}

int get_device_index(pid_t pid, struct device_info *devices, int size)
{
    int index = -1;

    for (int i=0; i<size; i++)
    {
        if (devices[i].pid == pid)
        {
            index = i;
            break;
        }
    }

    return index;
}

int get_actuator_index(pid_t pid, struct device_info *devices, int size)
{
    int index = -1;

    for (int i=0; i<size; i++)
    {
        if (devices[i].pid == pid)
        {
            index = devices[i].actuator_index;
            break;
        }
    }

    return index;
}

void parent_handler(void)
{
    pid_t pid = getpid();
    int msgid;
    int fifo_fd;
    int result;

    struct message_struct rx_data;
    struct message_struct tx_data;
    int rx_data_size = sizeof(struct message_struct) - sizeof(long);
    int tx_data_size = sizeof(struct message_struct) - sizeof(long);

    struct sigaction sa;
    memset((void *)&sa, 0, sizeof(sa));
    sa.sa_handler = &get_message;
    sigaction(SIGUSR1, &sa, 0);

    printf("[Parent] Started with PID=%d\n", pid);

    // Creates a message queue
    msgid = msgget((key_t)MESSAGE_QUEUE_ID, 0666 | IPC_CREAT);
    if (msgid == -1)
    {
        fprintf(stderr, "[PARENT] msgget failed with error: %d\n", errno);
        exit(EXIT_FAILURE);
    }

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

    // Opens the writing end of the fifo
    fifo_fd = open(FIFO_NAME, O_WRONLY);
    if (fifo_fd == -1)
    {
        fprintf(stderr, "open failed with error: %d\n", errno);
        fprintf(stderr, "%s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    printf("[PARENT] Connected to Cloud via FIFO\n");

    while (1)
    {
        if (get_message_flag)
        {
            // Receive update message from child
            if (msgrcv(msgid, (void *)&rx_data, rx_data_size,
                        pid, IPC_NOWAIT) == -1)
            {
                fprintf(stderr, "[PARENT] msgrcv failed with error: %d\n", errno);
                exit(EXIT_FAILURE);
            }

            printf("[PARENT] Received update from child. Sensor: pid=%d, threshold=%d, reading=%d, command='%s'\n",
                    rx_data.fields.pid, rx_data.fields.threshold, rx_data.fields.sensor_reading, rx_data.fields.data);

            // Constructs and sends update to Cloud process
            strncpy(tx_data.fields.name, rx_data.fields.name, sizeof(tx_data.fields.name));
            tx_data.fields.threshold = rx_data.fields.threshold;
            tx_data.fields.sensor_reading = rx_data.fields.sensor_reading;
            tx_data.fields.pid = rx_data.fields.pid;

            if (write(fifo_fd, (void *)&tx_data, tx_data_size) == -1)
            {
                fprintf(stderr, "[PARENT] write failed with error: %d\n", errno);
                exit(EXIT_FAILURE);
            }

            get_message_flag = 0;
        }
    }

    close(fifo_fd);
}

// Signal handler for SIGUSR1
void get_message(int signal_number)
{
    get_message_flag = 1;
}
