
#include <stdio.h>

#define MAX_MSG_LEN 1024

struct msg {
	int T;
	int len;
	char *data;
};

void myqueue_init(int count);
void myqueue_destroy();

void myqueue_pop();

void myqueue_push(const struct msg*);

void myqueue_front(struct msg*);

const int myqueue_count();
const int myqueue_max_count();
