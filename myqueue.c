
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <pthread.h>
#include <stdint.h>
#include "myqueue.h"

// TODO: test, debug

struct node {
	struct msg msg;
	struct node *next;
};

int N = 0;	// max queue elements count 

struct node *head = NULL;
struct node *back = NULL;

sem_t sem_got;
sem_t sem_left;

pthread_mutex_t mutex;
pthread_mutexattr_t mutattr;

void myqueue_init(const int max_count)
{
	pthread_mutexattr_init(& mutattr);
	pthread_mutexattr_settype(& mutattr, PTHREAD_MUTEX_RECURSIVE_NP);
	pthread_mutex_init(& mutex, NULL);

	N = max_count;
	sem_init(& sem_got, 0, 0);
	sem_init(& sem_left, 0, N);
}

const int myqueue_max_count() {
	return N;
}

const int myqueue_count() 
{
	pthread_mutex_lock(& mutex);
	int sval;
	sem_getvalue(& sem_got, & sval);
	pthread_mutex_unlock(& mutex);
	return sval;
}

void myqueue_clear() {	
	pthread_mutex_lock(& mutex);
	
	while (myqueue_count() != 0)
		myqueue_pop();

	pthread_mutex_unlock(& mutex);
}

void myqueue_destroy()
{
	myqueue_clear();

	sem_destroy(& sem_left);	
	sem_destroy(& sem_got);

	pthread_mutexattr_destroy(& mutattr);
	pthread_mutex_destroy(& mutex);
}

void myqueue_push(const struct msg *src) 
{
	sem_wait(& sem_left);

	// TODO: also block signals for current thread ?
	pthread_mutex_lock(& mutex); 

	struct node *np = (struct node*) malloc(sizeof(struct node));
	memcpy(& np->msg, src, sizeof(struct msg));

	uint8_t *p = (uint8_t *) malloc(np->msg.len * sizeof(uint8_t)); 
	memcpy(p, src->data, np->msg.len);

	np->msg.data = p;
	np->next = NULL;

	if (back != NULL) {
		back->next = np;
		back = back->next;
	} else { 
		back = np;				
		head = np;
	}
	
	sem_post(& sem_got);

	// TODO: also make current thread catch signals ?
	pthread_mutex_unlock(& mutex);

	return;
}

// Memory at frontmsg must be preallocated.
// Memory of size MAX_MSG_LEN bytes at frontmsg->data must be preallocated.
void myqueue_front(struct msg *dest)
{
	sem_wait(& sem_got);

	pthread_mutex_lock(& mutex); 

	sem_post(& sem_got);

	dest->T = head->msg.T; 
	dest->len = head->msg.len;

	memcpy(dest->data, head->msg.data, dest->len * sizeof(char));

	pthread_mutex_unlock(& mutex);
}

void myqueue_pop() 
{
	sem_wait(& sem_got);

	// TODO: also block signals for current thread ?
	pthread_mutex_lock(& mutex);

	if (head == back) 
		back = NULL;

	struct node *pt = head;
	head = head->next; 

	free(pt->msg.data);
	free(pt); 

	sem_post(& sem_left);

	// TODO: also make current thread catch signals ? 
	pthread_mutex_unlock(& mutex);
}
