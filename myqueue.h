
#include <stdio.h>

struct msg {
	int T;
	int len;
	char msg[BUFSIZ];
};

void myqueue_init(int count);
void myqueue_destroy();

void myqueue_pop();

void myqueue_push(const struct msg*);

void myqueue_front(const struct msg*);

const int myqueue_count();
const int myqueue_max_count();
