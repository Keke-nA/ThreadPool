#include"mythreadpool.h"
#include<functional>

constexpr uint32_t MAX_TASKQUEUE_SIZE = UINT32_MAX;
constexpr uint32_t MAX_THREAD_SIZE = 2;
constexpr uint16_t MAX_DURATION_TIME = 60;

int MyThread::generate_thread_id = 0;

MyThread::MyThread(threadfunc func) : my_func(func), thread_id(generate_thread_id++) {
}

MyThread::~MyThread() {
}

int MyThread::getThreadId() const {
	return this->thread_id;
}

void MyThread::start() {
	std::thread t(my_func, thread_id);
	t.detach();
}

MyThreadPool::MyThreadPool() :
	init_thread_size(0), cur_thread_size(0), idle_thread_size(0),
	thread_size_maxthreshold(MAX_THREAD_SIZE), task_queue_maxthreshhold(MAX_TASKQUEUE_SIZE),
	pool_mode(PoolMode::MODE_FIXED), is_threadpool_run(false) {
}

MyThreadPool::~MyThreadPool() {
	is_threadpool_run = false;
	{
		std::unique_lock<std::mutex> lock(taskq_mutex);
		cv_not_empty.notify_all();  // 唤醒所有线程退出循环
	}
	//cv_not_empty.notify_all();
	std::unique_lock<std::mutex> lock(taskq_mutex);
	exit_condition.wait(lock, [this] {return my_threads.size() == 0; });
}

MyResult MyThreadPool::submitTask(std::shared_ptr<Task> task) {
	std::unique_lock<std::mutex> lock(taskq_mutex);
	if (!cv_not_full.wait_for(lock, std::chrono::seconds(2),
		[this] {return task_queue.size() < task_queue_maxthreshhold; })) {
		std::cout << "当前任务队列已满所以阻塞了，提交任务失败！" << std::endl;
		return MyResult(task, false);
	}
	task_queue.emplace(task);
	cv_not_empty.notify_all();
	if (pool_mode == PoolMode::MODE_CACHED &&
		(task_queue.size() > idle_thread_size) && (cur_thread_size < thread_size_maxthreshold)) {
		auto ptr = std::make_unique<MyThread>
			(std::bind(&MyThreadPool::myThreadFun, this, std::placeholders::_1));
		int threadid = ptr->getThreadId();
		my_threads.emplace(threadid, std::move(ptr));
		my_threads[threadid]->start();
		cur_thread_size++;
		idle_thread_size++;
		std::cout << "新增一个线程，线程id:" << threadid << std::endl;
	}
	return MyResult(task);
}

void MyThreadPool::myThreadFun(int threadid) {
	auto lasttime = std::chrono::high_resolution_clock::now();
	while (is_threadpool_run) {
		std::shared_ptr<Task> task;
		{
			std::unique_lock<std::mutex> lock(taskq_mutex);
			while (task_queue.size() == 0) {
				if (pool_mode == PoolMode::MODE_CACHED) {
					if (std::cv_status::timeout == cv_not_full.wait_for(lock, std::chrono::seconds(1))) {
						auto nowtime = std::chrono::high_resolution_clock::now();
						auto dur = std::chrono::duration_cast<std::chrono::seconds>(nowtime - lasttime);
						if (dur.count() > MAX_DURATION_TIME && cur_thread_size > init_thread_size) {
							my_threads.erase(threadid);
							cur_thread_size--;
							idle_thread_size--;
							std::cout << "线程：" << threadid << "退出了" << std::endl;
							return;
						}
					}
				}
				else {
					cv_not_empty.wait(lock);
				}
				if (!is_threadpool_run) {
					my_threads.erase(threadid);
					std::cout << "thread:  " << threadid << "  exit" << std::endl;
					exit_condition.notify_all();
					return;
				}
			}
			std::cout << "thread:  " << threadid << "  gettask" << std::endl;
			idle_thread_size--;
			task = task_queue.front();
			task_queue.pop();
		}
		if (task_queue.size() > 0) {
			cv_not_empty.notify_all();
		}
		if (task != nullptr) {
			task->exec();
		}
		cv_not_full.notify_all();
		idle_thread_size++;
	}
	my_threads.erase(threadid);
	exit_condition.notify_all();
}

void MyThreadPool::start(int startthreadsize) {
	if (startthreadsize > MAX_THREAD_SIZE) {
		startthreadsize = MAX_THREAD_SIZE;
		std::cout << "线程池线程最大数量为：" << MAX_THREAD_SIZE <<
			"，您设置的值超过此值，因此直接设置为线程池最大数量" << std::endl;
	}
	is_threadpool_run = true;
	init_thread_size = startthreadsize;
	cur_thread_size = startthreadsize;
	for (int i = 0; i < startthreadsize; i++) {
		auto ptr = std::make_unique<MyThread>(std::bind(&MyThreadPool::myThreadFun, this, std::placeholders::_1));
		//auto ptr = std::make_unique<MyThread>(std::bind(&MyThreadPool::threadFunc, this, std::placeholders::_1));
		int thread_id = ptr->getThreadId();
		my_threads.emplace(thread_id, std::move(ptr));
	}

	for (auto& t : my_threads) {
		t.second->start();
		idle_thread_size++;
	}
}

void MyThreadPool::setMaxThreadSize(uint32_t size) {
	if (checkPoolState()) {
		std::cout << "当前线程池已经关闭，不能修改线程数量" << std::endl;
	}
	if (pool_mode != PoolMode::MODE_CACHED) {
		std::cout << "当前线程池工作模式为FIXED模式，不支持修改线程数量" << std::endl;
		return;
	}
	thread_size_maxthreshold = size;
}
void MyThreadPool::setMaxTaskSize(uint32_t size) {
	this->task_queue_maxthreshhold = size;
}
bool MyThreadPool::checkPoolState() const {
	return is_threadpool_run;
}

void MyThreadPool::setPoolMode(PoolMode mode) {
	if (checkPoolState()) {
		std::cout << "当前线程池已经关闭，不能修改线程模式" << std::endl;
	}
	pool_mode = mode;
}

void MySemaphore::mysemaphoreWait() {
	std::unique_lock<std::mutex> lock(semaphore_mutex);
	cv_semaphore.wait(lock, [this] {return resourse > 0; });
	resourse--;
}
void MySemaphore::mysemaphorePost() {
	std::lock_guard<std::mutex> lock(semaphore_mutex);
	resourse++;
	cv_semaphore.notify_all();
}

MyAny Task::run() {
	//std::lock_guard<std::mutex> lock(tast_run_mutex);
	std::cout << "Task::run():  " << std::endl;
	std::cout.flush(); // 显式刷新
	return MyAny();
}

void Task::exec() {
	my_result->setMyAny(run());
}

void Task::setResult(MyResult* res) {
	my_result = res;
}

MyResult::MyResult(std::shared_ptr<Task> task, bool is_valid) :
	my_task(task), result_is_valid(is_valid) {
	my_task->setResult(this);
}

void MyResult::setMyAny(MyAny any) {
	my_any = std::move(any);
	my_semaphore.mysemaphorePost();
}
MyAny MyResult::getResult() {
	if (result_is_valid == false) {
		return "";
	}
	my_semaphore.mysemaphoreWait();
	return std::move(my_any);
}