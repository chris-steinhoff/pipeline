
#include <pthread.h>
#include <iostream>
#include "pipeline.hpp"

#define ITER 50

typedef pipeline::Queue<int>* Que1;

void* putter1(void* arg) {
	Que1 queue = (Que1)arg;
	for(int i = 1 ; i <= ITER ; ++i) {
		int* ip = new int;
		*ip = i;
		queue->put(ip);
	}
	pthread_exit(NULL);
}

void* taker1(void* arg) {
	Que1 queue = (Que1)arg;
	for(int i = 0 ; i < ITER ; ++i) {
		int* val = queue->take();
		std::clog << "val = " << *val << std::endl;
		delete val;
	}
	pthread_exit(NULL);
}

template <class T>
class AddTask : public pipeline::Task<T> {
private:
	T add;

public:
	AddTask(T add) : add(add) {
	}

	virtual void process(T* work) {
		std::clog << "processing " << *work << std::endl;
		*work += add;
		std::clog << "finished processing " << *work << std::endl;
	}

};

int main() {
	pthread_t put_thread;
	pthread_t take_thread;

	Que1 queue1 = new pipeline::Queue<int>();

	std::clog << "Que1:" << std::endl;
	pthread_create(&put_thread, NULL, putter1, queue1);
	pthread_create(&take_thread, NULL, taker1, queue1);

	pthread_join(put_thread, NULL);
	pthread_join(take_thread, NULL);

	delete queue1;

	std::clog << std::endl << "Pipeline:" << std::endl;
	pipeline::Task<int>* tasks[2];
	tasks[0] = new AddTask<int>(1);
	tasks[1] = new AddTask<int>(1);
	pipeline::Pipeline<int>* pipe =
			pipeline::Pipeline<int>::createPipeline(tasks, 2);

	for(int i = 0 ; i < 10 ; ++i) {
		int* ii = new int;
		*ii = i;
		std::clog << "adding " << i << std::endl;
		pipe->add(ii);
	}
	for(int i = 0 ; i < 10 ; ++i) {
		int* ii = pipe->get();
		std::clog << "i = " << *ii << std::endl;
	}

	delete pipe;
	delete tasks[0];
	delete tasks[1];

	return 0;
}

