/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2015 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
    included in this distribution in the file called "COPYING". If not,
    see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@eurecom.fr

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

*******************************************************************************/
/*! \file enb_agent_task_manager.h
 * \brief Implementation of scheduled tasks manager for the enb agent
 * \author Xenofon Foukas
 * \date January 2016
 * \version 0.1
 * \email: x.foukas@sms.ed.ac.uk
 * @ingroup _mac
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "enb_agent_task_manager.h"


/* Util macros */
#define LEFT(x) (2 * (x) + 1)
#define RIGHT(x) (2 * (x) + 2)
#define PARENT(x) ((x - 1) / 2)

enb_agent_task_t *enb_agent_task_create(Protocol__ProgranMessage *msg,
				      uint16_t frame_num, uint8_t subframe_num) {
  enb_agent_task_t *task = NULL;
  task = malloc(sizeof(enb_agent_task_t));

  if (task == NULL)
    goto error;

  task->frame_num = frame_num;
  task->subframe_num = subframe_num;
  task->task = msg;
  
  return task;
  
 error:
  return NULL;
}

void enb_agent_task_destroy(enb_agent_task_t *task) {
  if (task == NULL)
    return;

  /* TODO: must free the task properly */
  free(task->task);
  free(task);
}

enb_agent_task_queue_t *enb_agent_task_queue_init(size_t capacity,
						int (*cmp)(const enb_agent_task_t *t1, const enb_agent_task_t *t2)) {
  enb_agent_task_queue_t *queue = NULL;

  queue = malloc(sizeof(enb_agent_task_queue_t));
  if (queue == NULL)
    goto error;

  /* If no comparator was given, use the default one */
  if (cmp == NULL)
    queue->cmp = _enb_agent_task_queue_cmp;
  else
    queue->cmp = cmp;

  queue->first_frame = 0;
  queue->first_subframe = 0;
  
  queue->task = malloc(capacity * sizeof(enb_agent_task_t *));
  if (queue->task == NULL)
    goto error;

  queue->count = 0;
  queue->capacity = capacity;
  
  queue->mutex = malloc(sizeof(pthread_mutex_t));
  if (queue->mutex == NULL)
    goto error;
  if (pthread_mutex_init(queue->mutex, NULL))
    goto error;

  return queue;

 error:
  if (queue != NULL) {
    free(queue->mutex);
    free(queue->task);
    free(queue);
  }
  return NULL;
}

enb_agent_task_queue_t *enb_agent_task_queue_default_init() {
  return enb_agent_task_queue_init(DEFAULT_CAPACITY, NULL);
}

void enb_agent_task_queue_destroy(enb_agent_task_queue_t *queue) {
  int i;
  
  if (queue == NULL)
    return;

  for (i = 0; i < queue->count; i++) {
    enb_agent_task_destroy(queue->task[i]);
  }
  free(queue->task);
  free(queue->mutex);
  free(queue);
}

int enb_agent_task_queue_put(enb_agent_task_queue_t *queue, enb_agent_task_t *task) {
  size_t i;
  enb_agent_task_t *tmp = NULL;
  int realloc_status, err_code;

  if (pthread_mutex_lock(queue->mutex)) {
    /*TODO*/
    err_code = -1;
    goto error;
  }

  if (queue->count >= queue->capacity) {
    /*TODO: need to call realloc heap*/
    realloc_status = _enb_agent_task_queue_realloc_heap(queue);
    if (realloc_status != HEAP_OK) {
      err_code = realloc_status;
      goto error;
    }
  }

  queue->task[queue->count] = task;
  i = queue->count;
  queue->count++;
  /*Swap elements to maintain heap properties*/
  while(i > 0 && queue->cmp(queue->task[i], queue->task[PARENT(i)]) > 0) {
    tmp = queue->task[i];
    queue->task[i] = queue->task[PARENT(i)];
    queue->task[PARENT(i)] = tmp;
    i = PARENT(i);
  }  
  
  if (pthread_mutex_unlock(queue->mutex)) {
    // LOG_E(MAC, "%s:%d:%s: fatal error\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }
  return 0;

 error:
  /*TODO*/
  return err_code;
}


int enb_agent_task_queue_get_current_task(enb_agent_task_queue_t *queue, enb_agent_task_t **task) {
  int err_code;

  /*TODO*/
  /* Find current frame and subframe number */
  uint16_t curr_frame=1;
  uint8_t curr_subframe=1;
  
  if (pthread_mutex_lock(queue->mutex)) {
    /*TODO*/
    err_code = -1;
    goto error;
  }
  
  if (queue->count < 1) {         
    /* Priority Queue is empty */         
    err_code = HEAP_EMPTY;
    goto error;
  }
  /* If no task is scheduled for the current subframe, return without any task */
  if(queue->task[0]->frame_num != curr_frame || queue->task[0]->subframe_num != curr_subframe) {
    *task = NULL;
    return 0;
  }
  /* Otherwise, the first task should be returned */
  *task = queue->task[0];
  queue->task[0] = queue->task[queue->count-1];
  queue->count--;
  /* Restore heap property */
  _enb_agent_task_queue_heapify(queue, 0);

  if (pthread_mutex_unlock(queue->mutex)) {
    // LOG_E(MAC, "%s:%d:%s: fatal error\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }
  return HEAP_OK;

 error:
  /*TODO*/
  return err_code;
}

/*Warning: Internal function. Should not be called as API function. Not thread safe*/
void _enb_agent_task_queue_heapify(enb_agent_task_queue_t *queue, size_t idx) {
  /* left index, right index, largest */
  enb_agent_task_t *tmp = NULL;
  size_t l_idx, r_idx, lrg_idx;

  l_idx = LEFT(idx);
  r_idx = RIGHT(idx);

  /* Left child exists, compare left child with its parent */
  if (l_idx < queue->count && queue->cmp(queue->task[l_idx], queue->task[idx]) > 0) {
    lrg_idx = l_idx;
  } else {
    lrg_idx = idx;
  }

  /* Right child exists, compare right child with the largest element */
  if (r_idx < queue->count && queue->cmp(queue->task[r_idx], queue->task[lrg_idx]) > 0) {
    lrg_idx = r_idx;
  }

  /* At this point largest element was determined */
  if (lrg_idx != idx) {
    /* Swap between the index at the largest element */
    tmp = queue->task[lrg_idx];
    queue->task[lrg_idx] = queue->task[idx];
    queue->task[idx] = tmp;
    /* Heapify again */
    _enb_agent_task_queue_heapify(queue, lrg_idx);
  }
}

/*Warning: Internal function. Should not be called as API function. Not thread safe*/
int _enb_agent_task_queue_realloc_heap(enb_agent_task_queue_t *queue) {
  enb_agent_task_t **resized_task_heap;
  if (queue->count >= queue->capacity) {
    size_t task_size = sizeof(enb_agent_task_t);

    resized_task_heap = realloc(queue->task, (2*queue->capacity) * task_size);
    if (resized_task_heap != NULL) {
      queue->capacity *= 2;
      queue->task = (enb_agent_task_t **) resized_task_heap;
      return HEAP_OK;
    } else return HEAP_REALLOCERROR;
  }
  return HEAP_NOREALLOC;
}

int _enb_agent_task_queue_cmp(const enb_agent_task_t *t1, const enb_agent_task_t *t2) {
  if ((t1->frame_num == t2->frame_num) && (t1->subframe_num == t2->subframe_num))
    return 0;
  /* TODO: need to get current frame and subframe number */
  uint16_t curr_frame = 0;
  uint8_t curr_subframe = 7;

  int f_offset, sf_offset, tmp1, tmp2;

  /*Check if the offsets have the same sign and compare the tasks position frame-wise*/
  tmp1 = t1->frame_num - curr_frame;
  tmp2 = t2->frame_num - curr_frame;
  if ((tmp1 >= 0) ^ (tmp2 < 0)) {
    f_offset = tmp2 - tmp1;
  }
  else {
    f_offset = tmp1 - tmp2;
  }
  /*Do the same for the subframe*/
  tmp1 = t1->subframe_num - curr_subframe;
  tmp2 = t2->subframe_num - curr_subframe;
  if ((tmp1 >= 0) ^ (tmp2 < 0))
    sf_offset = tmp2 - tmp1;
  else
    sf_offset = tmp1 - tmp2;

  /*Subframe position matters only if f_offset is 0. Multiply f_offset by 100
    to be the only comparisson parameter in all other cases */
  return f_offset*100 + sf_offset;
}


int main(int argc, char **argv) {
  return 0;
}
