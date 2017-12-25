#include "cs537.h"
#include "request.h"
#include <pthread.h>
#include <stdlib.h>

// 
// server.c: A very, very simple web server
//
// To run:
//  server <portnum (above 2000)>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
//

// CS537: Parse the new arguments too

int fill_ptr = 0;
int use_ptr = 0;
int count = 0;
int num_of_threads;
int buffer_max;
int *buffer;
pthread_cond_t  worker = PTHREAD_COND_INITIALIZER;
pthread_cond_t  master = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void getargs(int *port, int *num_of_threads, int *buffer_max, int argc, char *argv[])
{
	if (argc != 4) {
		fprintf(stderr, "Usage: %s <port> <threads> <buffers>\n", argv[0]);
		exit(1);
	}
	if (atoi(argv[2]) <= 0) {
		fprintf(stderr, "Number of threads should be greater than 0!\n");
		exit(1);
	}
	if (atoi(argv[3]) <= 0) {
		fprintf(stderr, "Buffer size should be greater than 0!\n");
		exit(1);
	}
	*port = atoi(argv[1]);
	*num_of_threads=atoi(argv[2]);
	*buffer_max=atoi(argv[3]);
}

void put(int value) {
	buffer[fill_ptr] = value;
	fill_ptr = (fill_ptr + 1) % buffer_max;
	count++;
}

int get() {
	int tmp = buffer[use_ptr];
	use_ptr = (use_ptr + 1) % buffer_max;
	count--;
	return tmp;
}

void *worker_threads(void *arg) {
	while(1)
	{
		pthread_mutex_lock(&mutex);
		while (count == 0)
		{
			pthread_cond_wait(&worker, &mutex);
		}
		int tmp = get();
		if(count<buffer_max)
			pthread_cond_signal(&master);
		pthread_mutex_unlock(&mutex);
		requestHandle(tmp);
		Close(tmp);
	}
        return NULL;
}

int main(int argc, char *argv[])
{
	int listenfd, connfd, port, clientlen;
	struct sockaddr_in clientaddr;
	getargs(&port, &num_of_threads, &buffer_max, argc, argv);

	buffer = malloc(buffer_max*sizeof(int));
	pthread_t *tid;			
	tid=malloc(sizeof(pthread_t)*num_of_threads);

	int i;
	for(i=0;i<num_of_threads;i++)
	{
		pthread_create(&tid[i], NULL, &worker_threads, NULL);
	}		

	listenfd = Open_listenfd(port);
	while (1) {
		clientlen = sizeof(clientaddr);
		connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);
		pthread_mutex_lock(&mutex);
		while (count == buffer_max)
		{
			pthread_cond_wait(&master, &mutex);
		}
		put(connfd);
		pthread_cond_signal(&worker);
		pthread_mutex_unlock(&mutex);
	} 
	return 0;
}
