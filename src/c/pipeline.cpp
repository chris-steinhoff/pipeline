
#include <pthread.h>
#include <iostream>
#include <list>
#include "pipeline.hpp"

#define ITER 50

typedef pipeline::Queue<int>* Que1;

void* putter1(void* arg) {
	Que1 queue = (Que1)arg;
	for(int i = 1 ; i <= ITER ; ++i) {
		int* ip = new int;
		*ip = i;
		pipeline::Work<int>* work =
				new pipeline::Work<int>(true, ip,
						new pipeline::Future<int>());
		queue->put(work);
	}
	queue->put(new pipeline::Work<int>(false, NULL, NULL));
	pthread_exit(NULL);
}

void* taker1(void* arg) {
	Que1 queue = (Que1)arg;
	bool running = true;
	while(running) {
		pipeline::Work<int>* work = queue->take();
		if((running = work->is_running())) {
			int* val = work->get_value();
			std::clog << "val = " << *val << std::endl;
			delete val;
			delete work->get_future();
		}
		delete work;
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

template <class T>
class PrintTask : public pipeline::Task<T> {
public:
	virtual void process(T* work) {
		std::clog << "finished pipe " << *work << std::endl;
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
	pipeline::Task<int>* tasks[3];
	tasks[0] = new AddTask<int>(1);
	tasks[1] = new AddTask<int>(1);
	tasks[2] = new PrintTask<int>();
	pipeline::Pipeline<int>* pipe =
			pipeline::Pipeline<int>::createPipeline(tasks, 3);

	std::list<pipeline::Future<int>*> futures;
	for(int i = 0 ; i < 1000 ; ++i) {
		int* ii = new int;
		*ii = i * 10;
		std::clog << "adding " << i << std::endl;
		futures.push_back(pipe->add(ii));
	}
	pipe->close();

	int i = 0;
	std::list<pipeline::Future<int>*>::iterator it;
	for(it = futures.begin() ; it != futures.end() ; ++it) {
		pipeline::Future<int>* future = *it;
		int* ii = future->get();
		std::clog << (i*10+2) << " == " << *ii << std::endl;
		++i;
		delete ii;
		delete future;
	}

	delete pipe;
	delete tasks[0];
	delete tasks[1];
	delete tasks[2];

	return 0;
}

