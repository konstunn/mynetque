
#include "myqueue.h"
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <pthread.h>

// TODO: test, debug

struct node {
	struct msg msg;
	struct node *next;
};

int N = 10;	// queue length 
int nodes_cnt = 0; // no need of

struct node *head = NULL;
struct node *back = NULL;

sem_t sem_got;
sem_t sem_left;

pthread_mutex_t mutex;
pthread_mutexattr_t mutattr;

void myqueue_init(const int max_count)\
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
	return nodes_cnt;
}

void myqueue_clear() {	
	pthread_mutex_lock(& mutex);
	
	while (head != NULL)
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

void myqueue_push(const struct msg *mp) 
{
	sem_wait(& sem_left);

	pthread_mutex_lock(& mutex);

	struct node *np = (struct node*) malloc(sizeof(struct node));
	memcpy(& np->msg, mp, sizeof(struct msg));

	if (back != NULL) {
		back->next = np;
		back = back->next;
	} else {
		back = np;				
		head = np;
	}

	np->next = NULL;
	
	nodes_cnt ++;
	sem_post(& sem_got);

	pthread_mutex_unlock(& mutex);

	return;
}

void myqueue_front(const struct msg *frontmsg)
{
	sem_wait(& sem_got);

	pthread_mutex_lock(& mutex);

	sem_post(& sem_got);

	memcpy((void*) frontmsg, & head->msg, sizeof(struct msg));

	pthread_mutex_unlock(& mutex);
}

void myqueue_pop() 
{
	sem_wait(& sem_got);

	pthread_mutex_lock(& mutex);

	struct node *pt = NULL;

	if (head == back)
		back = NULL;
	pt = head;
	head = head->next;
	free(pt);

	nodes_cnt --;

	sem_post(& sem_left);

	pthread_mutex_unlock(& mutex);
}
