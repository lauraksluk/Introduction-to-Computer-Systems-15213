/* 
 * Code for basic C skills diagnostic.
 * Developed for courses 15-213/18-213/15-513 by R. E. Bryant, 2017
 * Modified to store strings, 2018
 */

/*
 * This program implements a queue supporting both FIFO and LIFO
 * operations.
 *
 * It uses a singly-linked list to represent the set of queue elements
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "harness.h"
#include "queue.h"

// Create an empty queue.
// Returns NULL if not enough space could be allocated.
queue_t *q_new()
{
    queue_t *q =  malloc(sizeof(queue_t));
    if (q == NULL) return NULL;
    q->head = NULL;
    q->tail = NULL;
    q->size = 0;
    return q;
}

// Frees all storage used by queue.
void q_free(queue_t *q)
{
    if (q != NULL) {
      list_ele_t *head = q->head;
      list_ele_t *tail = q->tail;
      if (head != NULL) {

        // Loop through all elements in queue and free them.
        while (head != tail) {
          list_ele_t *temp = head;
          head = head->next;
          free(temp->value);
          free(temp);
        }
        free(head->value);
        free(head);
      }
      // Free queue struct.
      free(q);
    }
}

// Inserts string s at head of queue. Returns true if successful.
// Returns false if q is NULL or not enough space could be allocated.
bool q_insert_head(queue_t *q, char *s)
{
    if (q == NULL) return false;

    // Allocate space for new element.
    list_ele_t *newh = malloc(sizeof(list_ele_t));
    if (newh == NULL) return false;

    // Allocate space for new string.
    size_t str = strlen(s) + 1;
    char *val = malloc(sizeof(char) * str);
    if (val == NULL) {
      free(newh);
      return false;
    }

    // Copy over characters from input string.
    for (size_t i = 0; i < (str - 1); i++) {
      val[i] = s[i];
    }
    val[str - 1] = '\0';

    // Update field values.
    newh->value = val;
    newh->next = q->head;
    q->head = newh;
    q->size += 1;

    // Update tail field if queue was originally empty.
    if (q->tail == NULL) {
      q->tail = newh;
    }

    return true;
}

// Inserts string s at tail of queue. Returns true if successful.
// Returns false if q is NULL or not enough space could be allocated.
bool q_insert_tail(queue_t *q, char *s)
{
    if (q == NULL) return false;
    
    // Allocate space for new element.
    list_ele_t *ntail = malloc(sizeof(list_ele_t));
    if (ntail == NULL) {
      return false;
    }

    // Allocate space for new string.
    size_t str = strlen(s) + 1;
    char *val = malloc(sizeof(char) * str);
    if (val == NULL) {
      free(ntail);
      return false;
    }

    // Copy over characters from input string.
    for (size_t i = 0; i < (str - 1); i++) {
      val[i] = s[i];
    }
    val[str - 1] = '\0';

    // Update field values.
    ntail->value = val;
    ntail->next = NULL;
    q->size += 1;

    // Case on if queue was originally empty or not.
    if (q->size == 1) {
      q->tail = ntail;
      q->head = ntail;
    }
    else {
      q->tail->next = ntail;
      q->tail = ntail;
    }

    return true;
}

// Removes element from head of queue. Returns true if successful.
// Returns false if q is NULL or empty.
// Copies removed string into sp up to a maximum of (bufsize - 1) characters.
bool q_remove_head(queue_t *q, char *sp, size_t bufsize)
{
    if (q == NULL || q->head == NULL) return false;

    // Removes head element and updates field values.
    list_ele_t *head = q->head;
    q->head = q->head->next;
    q->size -= 1;

    // Copy removed string into sp.
    if (sp != NULL && head != NULL) {
      size_t index = 0;
      char *str = head->value;
      
      // Only copies up to a maximum of (bufsize - 1) characters.
      while (index < (bufsize - 1) && str[index] != '\0') {
        sp[index] = str[index];
        index++;
      }
      sp[index] = '\0';
    }

    // Frees removed list element.
    free(head->value);
    free(head);

    return true;
}

// Returns number of elements in queue. Returns 0 if q is NULL.
int q_size(queue_t *q)
{
    if (q == NULL) return 0;
    return (int)q->size;
}

// Reverses elements in queue.
void q_reverse(queue_t *q)
{
    // No effect if q is NULL, empty, or only has 1 element.
    if (q == NULL || q->head == NULL || q->size == 1) return;

    // Initialize variables.
    list_ele_t *prev;
    list_ele_t *head = q->head;
    list_ele_t *temp = head;
    list_ele_t *end = q->tail;

    // Place first element at the end.
    end->next = temp;
    head = temp->next;
    temp->next = NULL;
    q->tail = temp;
    temp = head;

    // Repeatedly place first element behind old tail.
    while (temp != end) {
      prev = end->next;
      end->next = temp;
      head = temp->next;
      temp->next = prev;
      temp = head;
    }

    // Update queue head field.
    q->head = head;
}

