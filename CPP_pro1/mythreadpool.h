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
	MySemaphore() :resource(0) {}
	~MySemaphore() = default;
	void mysemaphoreWait();
	void mysemaphorePost();

private:
	int resource;
	std::mutex semaphore_mutex;
	std::condition_variable cv_semaphore;
};

class MyResult;

class Task {
public:
	Task();
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
	MyResult(const MyResult&) = delete;
	MyResult& operator=(const MyResult&) = delete;
	MyResult(MyResult&& other) noexcept;
	MyResult& operator=(MyResult&& other) noexcept;
	void setMyAny(MyAny any);
	MyAny getResult();
private:
	MyAny my_any;
	std::shared_ptr<Task> my_task;
	std::unique_ptr<MySemaphore> my_semaphore;
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

enum class PoolMode {
	MODE_FIXED,	//固定线程模式
	MODE_CACHED,		//动态线程模式
};

class MyThreadPool {
public:
	MyThreadPool();
	~MyThreadPool();
	MyResult submitTask(std::shared_ptr<Task> task);		//向线程池提交任务
	void myThreadFun(int threadid);	//线程函数
	void start(int start = 4);	//启动线程池，默认线程数量为4
	void setMaxThreadSize(uint32_t size);	//设置最大线程数量
	void setMaxTaskSize(uint32_t size);	//设置最大任务数量
	bool checkPoolState() const;		//检查线程池运行状态
	void setPoolMode(PoolMode mode);		//设置线程池工作模式
private:
	std::unordered_map<uint32_t, std::unique_ptr<MyThread>> my_threads;	//1.线程map
	uint32_t init_thread_size;	//2.初始线程数量
	std::atomic_uint32_t cur_thread_size;		//3.当前线程池线程数量
	std::atomic_uint32_t idle_thread_size;	//空闲线程数量
	uint32_t thread_size_maxthreshold;        // 线程数量上限阈值

	std::queue<std::shared_ptr<Task>> task_queue;	//任务队列
	std::condition_variable cv_not_empty;	//任务队列不空
	std::condition_variable cv_not_full;		//任务队列不满
	uint32_t task_queue_maxthreshhold;                   // 任务队列数量上限阈值
	std::mutex taskq_mutex;	//保证互斥访问任务队列

	PoolMode pool_mode;	//线程池运行模式
	std::condition_variable exit_condition;	//线程退出状态
	std::atomic_bool is_threadpool_run;	//线程池开关状态
};