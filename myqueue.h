
#ifndef MYQUEUE_H
#define MYQUEUE_H

#define MAX_MSG_DATA_LEN		40
#define MAX_MSG_PROCTIME_SEC	10
#define SERVER_PORT				65535

#define MAX_MSG_SIZE 1024

struct msg {
	int T;
	int len;
	uint8_t *data;
};

void myqueue_init(int count);
void myqueue_destroy();

void myqueue_pop();

void myqueue_push(const struct msg*);

void myqueue_front(struct msg*);

const int myqueue_count();
const int myqueue_max_count();

#endif
