#pragma once
#include<iostream>
#include<functional>
#include<memory>
#include<atomic>
#include<condition_variable>
#include<thread>
#include<queue>

class Task {
	Task() = default;
	~Task() = default;
	void run(int threadid);
};

class MyThread {
public:
	using threadfunc = std::function<void(int)>;
	MyThread(threadfunc func);
	~MyThread();

	void start();
private:
	threadfunc my_func;
	int thread_id;
	static int thread_size;
};

class MyThreadPool {
public:
	MyThreadPool();
	~MyThreadPool();
	void myThreadFun(int threadid);
	void start(int start = 4);
private:
	std::unordered_map<int, std::unique_ptr<MyThread>> my_threads;
	uint16_t init_thread_size;
	std::atomic_int cur_thread_size;
	std::atomic_int idle_thread_size;
	std::condition_variable cv_not_empty;
	std::condition_variable cv_not_full;
	std::condition_variable exit_condition;
	std::mutex taskq_mutex;
	std::queue<std::shared_ptr<Task>> task_queue;
	std::atomic_bool is_threadpool_run;
};