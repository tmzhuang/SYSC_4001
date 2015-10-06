/*
 * SYSC 4001 Assignment 1
 * 
 * File: queue.c
 * Author: Brandon To
 * Student #: 100874049
 * Created: October 6, 2015
 *
 * Description:
 * Implementation of an integer queue.
 *
 */
#include "queue.h"

#include <stdio.h>
#include <stdlib.h>

struct queue *queue_create()
{
    struct queue *q = (struct queue*)malloc(sizeof(struct queue));
    q->size = 0;
    q->head = NULL;
    q->tail = NULL;

    return q;
}

void queue_add(struct queue *q, int i)
{
    struct node *temp = (struct node*)malloc(sizeof(struct node));
    temp->i = i;
    temp->next = NULL;

    if (q->size == 0)
    {
        q->head = temp;
        q->tail = temp;
    }
    else
    {
        q->tail->next = temp;
        q->tail = temp;
    }
    q->size++;
}

int queue_remove(struct queue *q, int *i)
{
    if (q->size == 0)
    {
        return -1;
    }

    *i = q->head->i;

    struct node *remove = q->head;
    q->head = q->head->next;
    free(remove);
    q->size--;

    return 0;
}

void queue_destroy(struct queue *q)
{
    if (q->size > 0)
    {
        struct node *remove = q->head;
        struct node *temp;

        while (remove->next != NULL)
        {
            temp = remove;
            remove = remove->next;
            free(temp);
        }
        free(remove);
    }
}

void queue_print(struct queue *q)
{
    if (q->size > 0)
    {
        struct node *temp = q->head;
        while (temp->next != NULL)
        {
            printf("%d ", temp->i);
            temp = temp->next;
        }
        printf("%d ", temp->i);
        printf("\n");
    }
}

