#include"mythreadpool.h"
#include<functional>


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
	t.join();
}

MyThreadPool::MyThreadPool() :
	init_thread_size(0), cur_thread_size(0), idle_thread_size(0), is_threadpool_run(false) {
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

void MyThreadPool::submitTask(std::shared_ptr<Task> task) {
	std::unique_lock<std::mutex> lock(taskq_mutex);
	task_queue.emplace(task);
	cv_not_empty.notify_one();
}

void MyThreadPool::myThreadFun(int threadid) {
	while (is_threadpool_run) {
		std::shared_ptr<Task> task;
		{
			std::unique_lock<std::mutex> lock(taskq_mutex);
			while (task_queue.size() == 0) {
				cv_not_empty.wait(lock);
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

			task->run(threadid);
		}
		cv_not_full.notify_all();
		idle_thread_size++;
	}
	my_threads.erase(threadid);
	exit_condition.notify_all();
}

void MyThreadPool::start(int start) {
	is_threadpool_run = true;
	init_thread_size = start;
	cur_thread_size = start;
	for (int i = 0; i < start; i++) {
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

void Task::run(int threadid) {
	std::lock_guard<std::mutex> lock(tast_run_mutex);
	std::cout << "Task::run():  " << threadid << std::endl;
	std::cout.flush(); // 显式刷新
}