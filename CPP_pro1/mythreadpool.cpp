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
	t.detach();
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

MyResult MyThreadPool::submitTask(std::shared_ptr<Task> task) {
	std::unique_lock<std::mutex> lock(taskq_mutex);
	task_queue.emplace(task);
	cv_not_empty.notify_all();
	return MyResult(task);
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
			task->exec();
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

void MySemaphore::mysemaphoreWait() {
	std::unique_lock<std::mutex> lock(semaphore_mutex);
	cv_semaphore.wait(lock, [this]{return resourse > 0; });
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

MyResult::MyResult(std::shared_ptr<Task> task, bool is_valid) :my_task(task), result_is_valid(is_valid) {
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