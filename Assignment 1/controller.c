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

#define MAX_DEVICES 256

struct device_info
{
    pid_t pid;
    char name[MAX_NAME_LENGTH];
    char device_type;
    int threshold;
};

void child_handler(void);
int get_device_index(pid_t pid, struct device_info *devices, int size);
int get_actuator_index(struct device_info *devices, int size);

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

    struct device_info devices[MAX_DEVICES];
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
        int received_device_index = get_device_index(rx_data.pid, devices, MAX_DEVICES);
        //printf("received_device_index = %d\n", received_device_index);
        if (received_device_index == -1)
        {
            devices[current_devices_index].pid = rx_data.pid;
            strncpy(devices[current_devices_index].name, rx_data.name, sizeof(devices[current_devices_index].name));
            devices[current_devices_index].device_type = rx_data.device_type;
            devices[current_devices_index].threshold = rx_data.threshold;
            printf("[CHILD] Device with PID=%d is now registered!\n", rx_data.pid);
            current_devices_index++;

            // Constructs and sends an acknowledgement message to device
            memset((void *)&tx_data, 0, sizeof(tx_data));
            tx_data.type = rx_data.pid;
            strncpy(tx_data.data, "ack", sizeof(tx_data.data));

            printf("[CHILD] Sending ack to device with PID=%d\n", rx_data.pid);
            if (msgsnd(msgid, (void *)&tx_data, tx_data_size, 0) == -1)
            {
                fprintf(stderr, "[CHILD] msgsnd failed\n");
                exit(EXIT_FAILURE);
            }

            continue;
        }

        if (rx_data.device_type == DEVICE_TYPE_ACTUATOR)
        {
            printf("[CHILD] Received ack from Actuator with PID=%d\n", (int)rx_data.pid);
            continue;
        }

        printf("[CHILD] Received reading of %d from PID=%d\n", rx_data.sensor_reading, rx_data.pid);

        if (rx_data.sensor_reading >= devices[received_device_index].threshold)
        {
            // Find an actuator
            int actuator_index = get_actuator_index(devices, MAX_DEVICES);

            if (actuator_index == -1)
            {
                printf("[CHILD] There are no actuators available at this time.\n");
                continue;
            }

            // Constructs and sends a command message to an actuator
            memset((void *)&tx_data, 0, sizeof(tx_data));
            tx_data.type = devices[actuator_index].pid;
            strncpy(tx_data.data, "turn off", sizeof(tx_data.data));

            printf("[CHILD] Sending command to Actuator with PID=%d\n", (int)tx_data.type);
            if (msgsnd(msgid, (void *)&tx_data, tx_data_size, 0) == -1)
            {
                fprintf(stderr, "[CHILD] msgsnd failed\n");
                exit(EXIT_FAILURE);
            }

            // Constructs and sends an update message to the parent
            memset((void *)&tx_data, 0, sizeof(tx_data));
            tx_data.type = ppid;
            strncpy(tx_data.name, devices[received_device_index].name, sizeof(tx_data.name));
            tx_data.threshold = devices[received_device_index].threshold;
            tx_data.sensor_reading = rx_data.sensor_reading;
            tx_data.pid = rx_data.pid;
            strncpy(tx_data.data, "turn off", sizeof(tx_data.data));

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

int get_actuator_index(struct device_info *devices, int size)
{
    int index = -1;

    for (int i=0; i<size; i++)
    {
        if (devices[i].device_type == DEVICE_TYPE_ACTUATOR)
        {
            index = i;
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
                    rx_data.pid, rx_data.threshold, rx_data.sensor_reading, rx_data.data);

            // Constructs and sends update to Cloud process
            strncpy(tx_data.name, rx_data.name, sizeof(tx_data.name));
            tx_data.threshold = rx_data.threshold;
            tx_data.sensor_reading = rx_data.sensor_reading;
            tx_data.pid = rx_data.pid;

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
