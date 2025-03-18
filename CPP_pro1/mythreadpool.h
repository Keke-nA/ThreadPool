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
	MODE_FIXED,	//�̶��߳�ģʽ
	MODE_CACHED,		//��̬�߳�ģʽ
};

class MyThreadPool {
public:
	MyThreadPool();
	~MyThreadPool();
	MyResult submitTask(std::shared_ptr<Task> task);		//���̳߳��ύ����
	void myThreadFun(int threadid);	//�̺߳���
	void start(int start = 4);	//�����̳߳أ�Ĭ���߳�����Ϊ4
	void setMaxThreadSize(uint32_t size);	//��������߳�����
	void setMaxTaskSize(uint32_t size);	//���������������
	bool checkPoolState() const;		//����̳߳�����״̬
	void setPoolMode(PoolMode mode);		//�����̳߳ع���ģʽ
private:
	std::unordered_map<uint32_t, std::unique_ptr<MyThread>> my_threads;	//1.�߳�map
	uint32_t init_thread_size;	//2.��ʼ�߳�����
	std::atomic_uint32_t cur_thread_size;		//3.��ǰ�̳߳��߳�����
	std::atomic_uint32_t idle_thread_size;	//�����߳�����
	uint32_t thread_size_maxthreshold;        // �߳�����������ֵ

	std::queue<std::shared_ptr<Task>> task_queue;	//�������
	std::condition_variable cv_not_empty;	//������в���
	std::condition_variable cv_not_full;		//������в���
	uint32_t task_queue_maxthreshhold;                   // �����������������ֵ
	std::mutex taskq_mutex;	//��֤��������������

	PoolMode pool_mode;	//�̳߳�����ģʽ
	std::condition_variable exit_condition;	//�߳��˳�״̬
	std::atomic_bool is_threadpool_run;	//�̳߳ؿ���״̬
};