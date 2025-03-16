#include"mythreadpool.h"


int MyThread::thread_size = 0;

MyThread::MyThread(threadfunc func) : my_func(func) {
	thread_size++;
}

MyThread::~MyThread() {

}

void MyThread::start() {
	std::thread t(my_func, thread_id);
	t.join();
}

MyThreadPool::MyThreadPool() :
	init_thread_size(0), cur_thread_size(0), idle_thread_size(0) {
}

MyThreadPool::~MyThreadPool() {

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
					std::cout << "thread:" << threadid << "exit" << std::endl;
					exit_condition.notify_all();
					return;
				}
			}
			idle_thread_size--;
			task = task_queue.front();
			task_queue.pop();
		}
		if(task
			)
		task->run(threadid);
	}
}

void MyThreadPool::start(int start) {

}

void Task::run(int threadid) {
	std::cout << "Task::run()" << threadid << std::endl;
}