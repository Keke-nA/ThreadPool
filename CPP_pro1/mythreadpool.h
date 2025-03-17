#pragma once
#include<iostream>
#include<functional>
#include<memory>
#include<atomic>
#include<condition_variable>
#include<thread>
#include<queue>
#include <stdexcept>
#include <typeinfo> 

class MyAny {
public:
	MyAny() = default;
	~MyAny() = default;
	MyAny(const MyAny&) = delete;
	MyAny& operator=(const MyAny&) = delete;
	MyAny(MyAny&&) = default;
	MyAny& operator=(MyAny&&) = default;

	template<typename T>
	MyAny(T data) :myany_base(std::make_unique<Derive<T>>(data)) {}

	template<typename T>
	T to_cast() {
		auto ptr = dynamic_cast<Derive<T>*>(myany_base.get());
		if (ptr == nullptr) {
			throw std::runtime_error(
				"dynamic_cast to Derive<T> failed. Actual type: " +
				std::string(typeid(*myany_base).name())
			);
		}
		return ptr->derive_data;
	}
private:
	class Base {
	public:
		virtual ~Base() = default;
	};
	template<typename T>
	class Derive :public Base {
	public:
		Derive(T data) :derive_data(data) {}
		T derive_data;
	};
private:
	std::unique_ptr<Base> myany_base;
};

class MySemaphore {
public:
	MySemaphore() :resourse(0) {}
	~MySemaphore() = default;
	void mysemaphoreWait();
	void mysemaphorePost();

private:
	int resourse;
	std::mutex semaphore_mutex;
	std::condition_variable cv_semaphore;
};

class MyResult;

class Task {
public:
	Task() = default;
	~Task() = default;
	void setResult(MyResult* res);
	void exec();
	virtual MyAny run();
private:
	//std::mutex tast_run_mutex;
	MyResult* my_result;
};

class MyResult {
public:
	MyResult(std::shared_ptr<Task> task, bool is_valid = true);
	~MyResult() = default;
	void setMyAny(MyAny any);
	MyAny getResult();
private:
	MyAny my_any;
	std::shared_ptr<Task> my_task;
	MySemaphore my_semaphore;
	std::atomic_bool result_is_valid;
};

class MyThread {
public:
	using threadfunc = std::function<void(int)>;
	MyThread(threadfunc func);
	~MyThread();

	int getThreadId() const;
	void start();
private:
	threadfunc my_func;
	int thread_id;
	static int generate_thread_id;
};

class MyThreadPool {
public:
	MyThreadPool();
	~MyThreadPool();
	MyResult submitTask(std::shared_ptr<Task> task);
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