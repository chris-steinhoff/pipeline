
#ifndef PIPELINE_HPP
#define PIPELINE_HPP 1

#include <iostream>
#include <pthread.h>

namespace pipeline {

template <class T>
class Future {
private:
	pthread_mutex_t value_mutex;
	pthread_cond_t value_set_cond;
	bool value_set;
	T* value;

public:
	Future() : value_set(false) {
		pthread_mutex_init(&value_mutex, NULL);
		pthread_cond_init(&value_set_cond, NULL);
	}

	~Future() {
		pthread_mutex_destroy(&value_mutex);
		pthread_cond_destroy(&value_set_cond);
	}

	T* get() {
		pthread_mutex_lock(&value_mutex);
		if(!value_set) {
			pthread_cond_wait(&value_set_cond, &value_mutex);
		}
		T* value = this->value;
		pthread_mutex_unlock(&value_mutex);
		return value;
	}

	void set_value(T* value) {
		pthread_mutex_lock(&value_mutex);
		this->value = value;
		value_set = true;
		pthread_cond_signal(&value_set_cond);
		pthread_mutex_unlock(&value_mutex);
	}

};

template <class T>
class Work {
private:
	bool running;
	T* const value;
	Future<T>* const future;

public:
	Work(bool running, T* const value, Future<T>*const  future)
			: running(running), value(value), future(future) {
	}

	bool is_running() const {
		return this->running;
	}

	void set_running(bool running) {
		this->running = running;
	}

	T* get_value() const {
		return this->value;
	}

	Future<T>* get_future() const {
		return this->future;
	}

};

template <class T>
std::ostream& operator<<(std::ostream& out, Work<T>& work) {
	T* v = work.get_value();
	out << (v ? *v : NULL);
	return out;
}

template <class T>
class Queue {
private:
	pthread_mutex_t work_mutex;
	pthread_cond_t work_set_cond;
	pthread_barrier_t exit_barrier;
	bool work_set;
	Work<T>* work;

public:
	Queue() {
		pthread_mutex_init(&work_mutex, NULL);
		pthread_cond_init(&work_set_cond, NULL);
		pthread_barrier_init(&exit_barrier, NULL, 2);
		work_set = false;
	}

	~Queue() {
		pthread_mutex_destroy(&work_mutex);
		pthread_cond_destroy(&work_set_cond);
		pthread_barrier_destroy(&exit_barrier);
	}

	void put(Work<T>* const work) {
		//std::clog << "offering " << *work << std::endl;
		pthread_mutex_lock(&work_mutex);
		this->work = work;
		work_set = true;
		pthread_cond_signal(&work_set_cond);
		pthread_mutex_unlock(&work_mutex);
		pthread_barrier_wait(&exit_barrier);
	}

	Work<T>* take() {
		pthread_mutex_lock(&work_mutex);
		if(!work_set) {
			pthread_cond_wait(&work_set_cond, &work_mutex);
		}
		Work<T>* work = this->work;
		work_set = false;
		pthread_mutex_unlock(&work_mutex);
		pthread_barrier_wait(&exit_barrier);
		//std::clog << "taking " << *work << std::endl;
		return work;
	}

};

template <class T>
class Task {
public:
	virtual void process(T* work)=0;

};

/*
 * A thread that will pull from an input queue, process the data,
 * then push into an output queue.
 */
template <class T>
class Flow {
protected:
	Task<T>* const task;
	Queue<T>* const in_queue;
	Queue<T>* const out_queue;
	pthread_t thread;

public:
	Flow(Task<T>* const task, Queue<T>* const input_queue,
			Queue<T>* const output_queue)
			: task(task), in_queue(input_queue), out_queue(output_queue) {
	}

	~Flow() {
		pthread_cancel(thread);
	}

	static void* start_flow(void* arg) {
		static_cast<pipeline::Flow<T>*>(arg)->run();
		pthread_exit(NULL);
	}

	void start() {
		pthread_create(&thread, NULL, start_flow, this);
	}

	virtual void run() {
		bool running = true;
		while(running) {
			Work<T>* work = in_queue->take();
			if((running = work->is_running())) {
				task->process(work->get_value());
			}
			out_queue->put(work);
		}
	}

	Queue<T>* input_queue() const {
		return in_queue;
	}

	Queue<T>* output_queue() const {
		return out_queue;
	}

};

template <class T>
class TerminalFlow : public Flow<T> {
public:
	TerminalFlow(Queue<T>* const input_queue)
			: Flow<T>(NULL, input_queue, NULL) {
	}

	void run() {
		bool running = true;
		while(running) {
			Work<T>* work = this->in_queue->take();
			if((running = work->is_running())) {
				work->get_future()->set_value(work->get_value());
			}
			delete work;
		}
	}

};

template <class T>
class Pipeline {
private:
	Flow<T>** flows;
	const int size;
	Queue<T>* entry_queue;

	Pipeline(Flow<T>** const flows, int size) : flows(flows), size(size),
			entry_queue(flows[0]->input_queue()) {
	}

public:
	~Pipeline() {
		for(int i = 0 ; i < size ; ++i) {
			delete this->flows[i]->input_queue();
			delete this->flows[i];
		}
		delete[] flows;
	}

	static Pipeline* createPipeline(Task<T>** const tasks, int size) {
		if(size == 0) {
			throw "Zero_Tasks";
		}

		Flow<T>** flows = new Flow<T>*[size + 1];
		Queue<T>* prev_queue = new Queue<T>();
		for(int i = 0 ; i < (size) ; ++i) {
			Queue<T>* out_queue = new Queue<T>();
			flows[i] = new Flow<T>(tasks[i], prev_queue, out_queue);
			flows[i]->start();
			prev_queue = out_queue;
		}
		flows[size] = new TerminalFlow<T>(prev_queue);
		flows[size]->start();
		return new Pipeline<T>(flows, size + 1);
	}

	Future<T>* add(T* const work) {
		Future<T>* future = new Future<T>();
		entry_queue->put(new Work<T>(true, work, future));
		return future;
	}

	void close() {
		this->entry_queue->put(new Work<T>(false, NULL, NULL));
	}

};

}

#endif

