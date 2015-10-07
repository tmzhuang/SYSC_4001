/*
 * SYSC 4001 Assignment 1
 * 
 * File: queue.c
 * Author: Brandon To
 * Student #: 100874049
 * Created: October 6, 2015
 *
 * Description:
 * An integer queue.
 *
 */
#ifndef QUEUE_H_
#define QUEUE_H_

struct node
{
    int i;
    struct node *next;
};

struct queue
{
    unsigned int size;
    struct node *head;
    struct node *tail;
};

struct queue *queue_create();
void queue_add(struct queue *q, int i);
int queue_remove(struct queue *q, int *i);
void queue_destroy(struct queue *q);
void queue_print(struct queue *q);

#endif
