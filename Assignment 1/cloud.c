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
#include <signal.h>

#include <sys/time.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "fifo.h"
#include "message_queue.h"

void child_handler(void);
int process_user_input(struct message_struct *message, char *user_input);

void parent_handler(pid_t child_pid);

void program_done(int signal_number);

int g_running_flag = 1;

int main(int argc, char* argv[])
{
    pid_t pid;

    char *name;

    // Capture SIGINT to close cleanly
    struct sigaction sa;
    memset((void *)&sa, 0, sizeof(sa));
    sa.sa_handler = &program_done;
    sigaction(SIGINT, &sa, 0);

    if (argc < 2)
    {
        fprintf(stderr, "Usage: cloud NAME\n");
        exit(EXIT_FAILURE);
    }

    name = argv[1];

    printf("Cloud starting.\n");

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
        parent_handler(pid);
        break;
    }

    exit(EXIT_SUCCESS);
}

void child_handler(void)
{
    pid_t pid = getpid();
    int fifo_fd;

    int result;
    char user_input[MAX_DATA_LENGTH];

    struct message_struct tx_data;

    printf("[CHILD] Started with PID=%d\n", pid);

    // Check for existance of fifo by attempting to access it
    if (access(FIFO_2_NAME, F_OK) == -1)
    {
        // Create the fifo if it does not exist
        result = mkfifo(FIFO_2_NAME, 0777);
        if (result != 0)
        {
            fprintf(stderr, "[CHILD] Could not create fifo %s\n", FIFO_2_NAME);
            exit(EXIT_FAILURE);
        }
    }

    // Opens the writing end of the fifo
    fifo_fd = open(FIFO_2_NAME, O_WRONLY);
    if (fifo_fd == -1)
    {
        fprintf(stderr, "[CHILD] open failed with error: %d\n", errno);
        exit(EXIT_FAILURE);
    }

    printf("[CHILD] Connected to Controller via FIFO\n");

    while (g_running_flag)
    {
        char *result = fgets(user_input, MAX_DATA_LENGTH, stdin);
        int len = strlen(user_input);
        if (result != NULL && user_input[len-1] == '\n')
        {
            user_input[len-1] = '\0';
        }

        // Break out (end program) if SIGINT is raised
        if (!g_running_flag)
        {
            break;
        }

        memset((void *)&tx_data, 0, sizeof(tx_data));

        // Process query
        if (process_user_input(&tx_data, user_input) == -1)
        {
            printf("[CHILD] Malformed query. Please try again...\n");
            continue;
        }

        // Send query to controller
        printf("[CHILD] Sending query to Controller\n");
        if (write(fifo_fd, (void *)&tx_data, sizeof(tx_data)) == -1)
        {
            fprintf(stderr, "[CHILD] write failed with error: %d\n", errno);
            exit(EXIT_FAILURE);
        }
    }

    close(fifo_fd);
}

int process_user_input(struct message_struct *message, char *user_input)
{
    int command_required = 0;
    char delim_space[2] = " ";
    char delim_quote[2] = "\"";
    char* token = strtok(user_input, delim_space);

    if (token != NULL)
    {
        if (strncmp(token, "Get", 3) == 0)
        {
            message->fields.device_type = DEVICE_TYPE_SENSOR;
        }
        else if (strncmp(token, "Put", 3) == 0)
        {
            message->fields.device_type = DEVICE_TYPE_ACTUATOR;
            command_required = 1;
        }
        else
        {
            return -1;
        }
    }
    else
    {
        return -1;
    }

    token = strtok(NULL, delim_space);
    if (token != NULL)
    {
        message->fields.pid = atoi(token);
    }
    else
    {
        return -1;
    }

    if (command_required)
    {
        token = strtok(NULL, delim_quote);
        if (token != NULL)
        {
            strncpy(message->fields.data, token, sizeof(message->fields.data));
        }
        else
        {
            return -1;
        }
    }

    return 0;
}

void parent_handler(pid_t child_pid)
{
    pid_t pid = getpid();
    int result;
    struct timeval t1, t2;

    int fifo_fd;
    int bytes_read = 0;

    struct message_struct rx_data;

    printf("[PARENT] Started with PID=%d\n", pid);

    // Check for existance of fifo by attempting to access it
    if (access(FIFO_1_NAME, F_OK) == -1)
    {
        // Create the fifo if it does not exist
        result = mkfifo(FIFO_1_NAME, 0777);
        if (result != 0)
        {
            fprintf(stderr, "[PARENT] Could not create fifo %s\n", FIFO_1_NAME);
            exit(EXIT_FAILURE);
        }
    }

    // Opens the reading end of the fifo
    fifo_fd = open(FIFO_1_NAME, O_RDONLY);
    if (fifo_fd == -1)
    {
        fprintf(stderr, "[PARENT] open failed with error: %d\n", errno);
        exit(EXIT_FAILURE);
    }

    printf("[PARENT] Connected to Controller via FIFO\n");

    while (g_running_flag)
    {
        // Receive update from Controller
        memset((void *)&rx_data, 0, sizeof(rx_data));
        if((bytes_read = read(fifo_fd, (void *)&rx_data,
                        sizeof(rx_data))) == -1)
        {
            if (errno != 11)
            {
                fprintf(stderr, "[PARENT] read failed with error: %d\n", errno);
                exit(EXIT_FAILURE);
            }
            continue;
        }

        // Check for "stop" command and quit if received
        if (strncmp(rx_data.fields.data, "stop", 4) == 0)
        {
            printf("[PARENT] Received stop command from Controller. Stopping device.\n");
            g_running_flag = 0;
            break;
        }

        printf("[PARENT] Received update from Controller. Sensor: pid=%d, name='%s', threshold=%d, reading=%d\n",
                rx_data.fields.pid, rx_data.fields.name, rx_data.fields.threshold, rx_data.fields.sensor_reading);
    }

    kill(child_pid, SIGINT);
    close(fifo_fd);
}

// Signal handler for SIGINT
void program_done(int signal_number)
{
    g_running_flag = 0;
}
