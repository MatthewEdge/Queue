/**
  * Copyright (C) 2011 by Tobias Thiel
  * Permission is hereby granted, free of charge, to any person obtaining a copy
  * of this software and associated documentation files (the "Software"), to deal
  * in the Software without restriction, including without limitation the rights
  * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  * copies of the Software, and to permit persons to whom the Software is
  * furnished to do so, subject to the following conditions:
  * 
  * The above copyright notice and this permission notice shall be included in
  * all copies or substantial portions of the Software.
  * 
  * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  * THE SOFTWARE.
  */

#include "queue.h"
#include "queue_internal.h"

queue_t *queue_create(void) {
	queue_t *q = malloc(sizeof(queue_t));
	if(q != NULL) {
		q->mutex = malloc(sizeof(pthread_mutex_t));
		if(q->mutex == NULL) {
			free(q);
			return NULL;
		}
		pthread_mutex_init(q->mutex, NULL);
		
		q->cond_get = malloc(sizeof(pthread_cond_t));
		if(q->cond_get == NULL) {
			pthread_mutex_destroy(q->mutex);
			free(q->mutex);
			free(q);
			return NULL;
		}
		pthread_cond_init(q->cond_get, NULL);

		q->cond_put = malloc(sizeof(pthread_cond_t));
		if(q->cond_put == NULL) {
			pthread_cond_destroy(q->cond_get);
			free(q->cond_get);
			pthread_mutex_destroy(q->mutex);
			free(q->mutex);
			free(q);
			return NULL;
		}
		pthread_cond_init(q->cond_put, NULL);
		
		q->first_el = NULL;
		q->last_el = NULL;
		q->num_els = 0;
		q->max_els = 0;
		q->new_data = 1;
		q->sort = 0;
		q->asc_order = 1;
		q->cmp_el = NULL;
	}
	
	return q;
}

queue_t *queue_create_limited(uintX_t max_elements) {
	queue_t *q = queue_create();
	if(q != NULL)
		q->max_els = max_elements;
	
	return q;
}

queue_t *queue_create_sorted(int8_t asc, int (*cmp)(void *, void *)) {
	if(cmp == NULL)
		return NULL;
		
	queue_t *q = queue_create();
	if(q != NULL) {
		q->sort = 1;
		q->asc_order = asc;
		q->cmp_el = cmp;
	}
	
	return q;
}

queue_t *queue_create_limited_sorted(uintX_t max_elements, int8_t asc, int (*cmp)(void *, void *)) {
	if(cmp == NULL)
		return NULL;
		
	queue_t *q = queue_create();
	if(q != NULL) {
		q->max_els = max_elements;
		q->sort = 1;
		q->asc_order = asc;
		q->cmp_el = cmp;
	}
	
	return q;
}

int8_t queue_destroy(queue_t *q) {
	return queue_destroy_internal(q, 0, NULL);
}

int8_t queue_destroy_complete(queue_t *q, void (*ff)(void *)) {
	return queue_destroy_internal(q, 1, ff);
}

int8_t queue_flush(queue_t *q) {
	return queue_flush_internal(q, 0, NULL);
}

int8_t queue_flush_complete(queue_t *q, void (*ff)(void *)) {
	return queue_flush_internal(q, 1, ff);
}

uintX_t queue_elements(queue_t *q) {
	uintX_t ret = UINTX_MAX;
	
	if(q == NULL) // queue not valid
		return ret;
	
	if(0 != pthread_mutex_lock(q->mutex))
		return ret;

	ret = q->num_els;
	pthread_mutex_unlock(q->mutex);
	
	return ret;
}

int8_t queue_empty(queue_t *q) {
	if(q == NULL) // queue not valid
		return Q_ERR_INVALID;
	
	uint8_t ret;
	if(0 != pthread_mutex_lock(q->mutex))
		return Q_ERR_LOCK;
	
	if(q->first_el == NULL || q->last_el == NULL)
		ret = 1;
	else
		ret = 0;
	
	pthread_mutex_unlock(q->mutex);
	
	return ret;
}

int8_t queue_set_new_data(queue_t *q, uint8_t v) {
	if(q == NULL) // queue not valid
		return Q_ERR_INVALID;
	
	if(0 != pthread_mutex_lock(q->mutex))
		return Q_ERR_LOCK;

	q->new_data = v;
	if(q->new_data == 0) {
		// notify waiting threads, when new data isn't accepted
		pthread_cond_broadcast(q->cond_get);
		pthread_cond_broadcast(q->cond_put);
	}

	pthread_mutex_unlock(q->mutex);
	return Q_OK;
}

uint8_t queue_get_new_data(queue_t *q) {
	uint8_t r = 0;
	if(0 != pthread_mutex_lock(q->mutex))
		return r;
	r = q->new_data;
	pthread_mutex_unlock(q->mutex);
	return r;
}

int8_t queue_put(queue_t *q, void *el) {
	return queue_put_internal(q, el, NULL);
}

int8_t queue_put_wait(queue_t *q, void *el) {
	return queue_put_internal(q, el, pthread_cond_wait);
}

int8_t queue_get(queue_t *q, void **e) {
	return queue_get_internal(q, e, NULL, NULL, NULL);
}

int8_t queue_get_wait(queue_t *q, void **e) {
	return queue_get_internal(q, e, pthread_cond_wait, NULL, NULL);
}

int8_t queue_get_filtered(queue_t *q, void **e, int (*cmp)(void *, void *), void *cmpel) {
	return queue_get_internal(q, e, NULL, cmp, cmpel);
}
