
#ifndef PIPELINE_HPP
#define PIPELINE_HPP 1

#include <iostream>
#include <pthread.h>

namespace pipeline {

template <class T>
class Queue;

template <class T>
class Work {
private:
	bool running;
	T* value;
	Queue<T>* future_queue;

public:
	Work(bool running, T* value, Queue<T>* future_queue)
			: running(running), value(value), future_queue(future_queue) {
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

	Queue<T>* get_future_queue() const {
		return this->future_queue;
	}

};

template <class T>
std::ostream& operator<<(std::ostream& out, Work<T>& work) {
	out << work.get_value();
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
		this->work_set = false;
	}

	~Queue() {
		pthread_mutex_destroy(&work_mutex);
		pthread_cond_destroy(&work_set_cond);
		pthread_barrier_destroy(&exit_barrier);
	}

	void put(Work<T>* work) {
		std::clog << "offering " << *work << std::endl;
		pthread_mutex_lock(&work_mutex);
		this->work = work;
		this->work_set = true;
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
		this->work_set = false;
		pthread_mutex_unlock(&work_mutex);
		pthread_barrier_wait(&exit_barrier);
		std::clog << "taking " << *work << std::endl;
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
	Task<T>* task;
	Queue<T>* in_queue;
	Queue<T>* out_queue;
	pthread_t thread;

public:
	Flow(Task<T>* task, Queue<T>* input_queue, Queue<T>* output_queue)
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
		return this->in_queue;
	}

	Queue<T>* output_queue() const {
		return this->out_queue;
	}

};

template <class T>
class TerminalFlow : public Flow<T> {
public:
	TerminalFlow(Queue<T>* input_queue)
			: Flow<T>(NULL, input_queue, NULL) {
	}

	void run() {
		bool running = true;
		while(running) {
			Work<T>* work = this->in_queue->take();
			if((running = work->is_running())) {
				work->get_future_queue()->put(work);
			}
		}
	}

};

template <class T>
class Future {
private:
	Queue<T>* queue;

public:
	Future(Queue<T>* queue) : queue(queue) {
	}

	T* get() {
		return queue->take()->value;
	}

};

template <class T>
class Pipeline {
private:
	Flow<T>** flows;
	const int size;
	Queue<T>* entry_queue;
	Queue<T>* exitry_queue;

	Pipeline(Flow<T>* flows[], int size) : flows(flows), size(size),
			entry_queue(flows[0]->input_queue()),
			exitry_queue(flows[size - 1]->output_queue()) {
	}

public:
	~Pipeline() {
		for(int i = 0 ; i < this->size ; ++i) {
			delete this->flows[i]->input_queue();
			delete this->flows[i];
		}
		delete this->exitry_queue;
		delete[] flows;
	}

	static Pipeline* createPipeline(Task<T>** tasks, int size) {
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
		/*Queue<T>* exitry_queue = new Queue<T>();
		flows[size - 1] = new Flow<T>(tasks[size - 1], prev_queue,
				exitry_queue);
		flows[size - 1]->start();*/
		flows[size] = new TerminalFlow<T>(prev_queue);
		flows[size]->start();
		return new Pipeline<T>(flows, size + 1);
	}

	Future<T>* add(T* work) {
		Work<T>* wk = new Work<T>(true, work, new Queue<T>());
		Future<T>* future = new Future<T>(wk->get_future_queue());
		this->entry_queue->put(wk);
		return future;
	}

	void close() {
		Work<T>* wk = new Work<T>(false, NULL, NULL);
		this->entry_queue->put(wk);
	}

};

}

#endif

