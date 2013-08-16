
#include <gtest/gtest.h>
#include <iostream>
#include <cstring>
#include "pipeline.hpp"


using namespace pipeline;

template <class T>
struct putter_arg {
	Queue<T>* queue;
	T* (*factory)(int);
};

extern "C" void* putter_int(void* arg);
template <class T>
void* putter(void* arg) {
	putter_arg<T>* pa = static_cast<putter_arg<T>*>(arg);
	for(int i = 0 ; i < 10 ; ++i) {
		Work<T>* work = new Work<T>(true, pa->factory(i), NULL);
		pa->queue->put(work);
	}
	pa->queue->put(new Work<T>(false, NULL, NULL));
	pthread_exit(NULL);
}

int* int_factory(int i) {
	int* ii = new int;
	*ii = i;
	return ii;
}

TEST(QueueTest, Int) {
	Queue<int>* queue = new Queue<int>();
	putter_arg<int> arg = { queue, int_factory };
	pthread_t thread;
	pthread_create(&thread, NULL, putter<int>, &arg);
	Work<int>* wk;
	int i = 0;
	while((wk = queue->take())->is_running()) {
		int* ii = wk->get_value();
		EXPECT_EQ(i++, *ii);
		delete ii;
		delete wk;
	}
	delete queue;
}

char* cstring_factory(int i) {
	char* str = new char[i+1];
	str[i] = '\0';
	while(--i >= 0) {
		str[i] = 'a';
	}
	return str;
}

TEST(QueueTest, CString) {
	Queue<char>* queue = new Queue<char>();
	putter_arg<char> arg = { queue, cstring_factory };
	pthread_t thread;
	pthread_create(&thread, NULL, putter<char>, &arg);
	Work<char>* wk;
	int i = 0;
	while((wk = queue->take())->is_running()) {
		char* ii = wk->get_value();
		EXPECT_EQ(i++, strlen(ii));
		delete ii;
		delete wk;
	}
	delete queue;
}

struct point {
	int x, y;
};
/*std::ostream& operator<<(std::ostream& out, point& p) {
	out << "point{ x = " << p.x << ", y = " << p.y << " }";
	return out;
}*/

point* struct_factory(int i) {
	point* p = new point();
	p->x = i;
	p->y = i * 2;
	return p;
}

TEST(QueueTest, Struct) {
	Queue<point>* queue = new Queue<point>();
	putter_arg<point> arg = { queue, struct_factory };
	pthread_t thread;
	pthread_create(&thread, NULL, putter<point>, &arg);
	Work<point>* wk;
	int i = 0;
	while((wk = queue->take())->is_running()) {
		point* p = wk->get_value();
		EXPECT_EQ(i, p->x);
		EXPECT_EQ(i++ * 2, p->y);
		delete p;
		delete wk;
	}
	delete queue;
}

class Person {
private:
	const char* name;
public:
	Person(const char* name) : name(name) {}
	const char* get_name() const { return name; }
};

Person* class_factory(int i) {
	Person* p = new Person("Full Name");
	return p;
}

TEST(QueueTest, Class) {
	Queue<Person>* queue = new Queue<Person>();
	putter_arg<Person> arg = { queue, class_factory };
	pthread_t thread;
	pthread_create(&thread, NULL, putter<Person>, &arg);
	Work<Person>* wk;
	while((wk = queue->take())->is_running()) {
		Person* p = wk->get_value();
		EXPECT_STREQ("Full Name", p->get_name());
		delete p;
		delete wk;
	}
	delete queue;
}

template <class T>
class AddTask : public pipeline::Task<T> {
private:
	T add;
public:
	AddTask(T add) : add(add) {}
	void process(T* work) { *work += add; }
};

#define tsize 2
#define fsize 100
TEST(PipelineTest, Int) {
	AddTask<int>** tasks = new AddTask<int>*[tsize];
	tasks[0] = new AddTask<int>(5);
	tasks[1] = new AddTask<int>(10);
	Task<int>** tas = (Task<int>**)tasks;

	Pipeline<int>* pipe = Pipeline<int>::createPipeline(tas, tsize);

	Future<int>* futures[fsize];
	for(int i = 0 ; i < fsize ; ++i) {
		int* ii = new int;
		*ii = i;
		futures[i] = pipe->add(ii);
	}

	pipe->close();
	for(int i = 0 ; i < fsize ; ++i) {
		EXPECT_EQ((i + 5 + 10), *futures[i]->get());
		delete futures[i]->get();
	}

	delete pipe;
	for(int i = 0 ; i < tsize ; ++i) {
		delete tasks[i];
	}
	delete[] tasks;
}

int main(int argc, char** argv) {
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}

