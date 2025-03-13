#include "threadpool.h"

#include <functional>
#include <iostream>
#include <thread>

const int TASK_MAX_THRESHHOLD = INT32_MAX;
const int THREAD_MAX_THRESHHOLD = 100;
const int THREAD_MAX_IDLE_TIME = 60;  // 单位：s

// 线程池构造
ThreadPool::ThreadPool()
	: initThreadSize_(0), taskSize_(0), curThreadSize_(0), idleThreadSize_(0), threadSizeThreshHold_(THREAD_MAX_THRESHHOLD), taskQueMaxThreshHold_(TASK_MAX_THRESHHOLD), poolMode_(PoolMode::MODE_FIXED), isPoolRunning_(false) {
}

// 线程池析构
ThreadPool::~ThreadPool() {
	isPoolRunning_ = false;
	notEmpty_.notify_all();

	//等待线程池里面所有的线程返回   有两种状态：阻塞 & 正在执行任务中
	std::unique_lock<std::mutex> lock(taskQuemtx_);
	exitCond_.wait(lock, [&]()->bool {return threads_.size() == 0; });
}

// 设置线程工作模式
void ThreadPool::setMode(PoolMode mode) {
	if (checkRunningState()) {
		return;
	}
	poolMode_ = mode;
}

// 设置task任务队列上线的阈值
void ThreadPool::setTaskQueMaxThreshHold(int threshhold) {
	taskQueMaxThreshHold_ = threshhold;
}

// 设置线程池chched模式下的线程阈值
void ThreadPool::setThraedSizeThreshHold(int threshhold) {
	if (checkRunningState()) {
		return;
	}
	if (poolMode_ == PoolMode::MODE_CACHED) {
		threadSizeThreshHold_ = threshhold;
	}
}

// 给线程池提交任务  用户调用该接口，传入任务对象，生产任务
Result ThreadPool::submitTask(std::shared_ptr<Task> sp) {
	// 获取锁
	std::unique_lock<std::mutex> lock(taskQuemtx_);

	// 线程的通信 等待任务队列有空余
	// 用户提交任务，最长不能阻塞超过1s，否则判断提交任务失败，返回
	if (!notFull_.wait_for(lock, std::chrono::seconds(1), [&]() -> bool { return taskQue_.size() < (size_t)taskQueMaxThreshHold_; })) {
		// 表示notFull_等待1秒钟，条件依然没有满足
		std::cerr << "tast queue is full, submit task fail." << std::endl;
		// return task->getResult();  // Task Result  线程执行完Task，Task就被析构掉了
		return Result(sp, false);
	}

	// 如果有空余，把任务放入任务队列中
	taskQue_.emplace(sp);
	taskSize_++;

	// 因为新放了任务，任务队列肯定不空了，notEmpty_上进行通知，赶快分配线程执行任务
	notEmpty_.notify_all();

	// chche模式 任务处理比较紧急 场景：小而快的任务  需要根据任务数量和空闲线程的数量，判断是否需要创建新的线程出来？
	if (poolMode_ == PoolMode::MODE_CACHED && taskSize_ > idleThreadSize_ && curThreadSize_ < threadSizeThreshHold_) {
		// 创建新线程
		std::cout << ">>> create new thread..." << std::endl;
		auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1));
		int threadId = ptr->getId();
		threads_.emplace(threadId, std::move(ptr));
		// 启动线程
		threads_[threadId]->start();
		// 修改线程个数相关的变量
		idleThreadSize_++;
		curThreadSize_++;
	}

	// 返回任务的Result对象
	// return task->getResult();
	return Result(sp);
}

// 开启线程池
void ThreadPool::start(int initThreadSize) {
	// 设置线程池的运行状态
	isPoolRunning_ = true;

	// 记录初始线程个数
	initThreadSize_ = initThreadSize;
	curThreadSize_ = initThreadSize;

	// 创建线程对象
	for (int i = 0; i < initThreadSize_; ++i) {
		// 创建thread线程对象的时候，把线程函数给到thread线程对象
		//auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1));
		auto ptr = std::make_unique<Thread>(
			[this](int arg) {
				this->threadFunc(arg);
			}
		);
		int threadId = ptr->getId();
		threads_.emplace(threadId, std::move(ptr));
		// threads_.emplace_back(std::move(ptr));
	}

	// 启动所有线程 std::vector<Thread*> threads_;
	for (int i = 0; i < initThreadSize_; ++i) {
		threads_[i]->start();  // 需要去执行一个线程函数
		idleThreadSize_++;     // 记录初始空闲线程的数量
	}
}

bool ThreadPool::checkRunningState() const {
	return isPoolRunning_;
}

// 定义线程函数  线程池的所有线程从任务队列里面消费任务
void ThreadPool::threadFunc(int threadid) {  // 线程函数返回，相应的线程也就结束了
	auto lastTime = std::chrono::high_resolution_clock().now();

	while (isPoolRunning_) {
		std::shared_ptr<Task> task;
		{
			// 先获取锁
			std::unique_lock<std::mutex> lock(taskQuemtx_);

			std::cout << "tid:" << std::this_thread::get_id() << "尝试获取任务..." << std::endl;

			// cached模式下，有可能已经创建了很多的线程，但是空闲时间超过60s，
			// 应该把多余的线程结束回收掉（超过initThreadSize_数量的线程要进行回收）
			// 当前时间-上一次线程执行的时间>60s

				// 每一秒钟返回一次  怎么区分超时返回还是有任务待执行返回
			while (taskQue_.size() == 0) {
				if (poolMode_ == PoolMode::MODE_CACHED) {
					// 条件变量超时返回了
					if (std::cv_status::timeout == notEmpty_.wait_for(lock, std::chrono::seconds(1))) {
						auto nowTime = std::chrono::high_resolution_clock().now();
						auto dur = std::chrono::duration_cast<std::chrono::seconds>(nowTime - lastTime);
						if (dur.count() >= 60 && curThreadSize_ > initThreadSize_) {
							// 开始回收当前线程
							// 记录线程数量的相关变量的值的修改
							// 把线程对象从线程列表容器中删除  没有办法匹配 threadFunc 《=》 thread对象
							// threadid =》thread对象 =》删除
							threads_.erase(threadid);  // 不要用std::this_thread::getid()这是cpp库生成的
							curThreadSize_--;
							idleThreadSize_--;
							std::cout << "threadid:" << std::this_thread::get_id() << "exit" << std::endl;
							return;
						}
					}

				}
				else {
					// 等待notEmpty条件
					notEmpty_.wait(lock);
				}

				// 线程池要结束，回收线程资源
				if (!isPoolRunning_) {
					threads_.erase(threadid);  // 不要用std::this_thread::getid()这是cpp库生成的
					std::cout << "threadid:" << std::this_thread::get_id() << "exit" << std::endl;
					exitCond_.notify_all();
					return;
				}
			}


			idleThreadSize_--;

			std::cout << "tid:" << std::this_thread::get_id() << "获取任务成功" << std::endl;

			// 从任务队列中取一个任务出来
			task = taskQue_.front();
			taskQue_.pop();
			taskSize_--;
			// 如果依然有剩余任务，继续统治其他的线程执行任务
			if (taskQue_.size() > 0) {
				notEmpty_.notify_all();
			}
			// 取出一个任务，进行通知,通知可以继续提交生产任务
			notFull_.notify_all();
		}  // 应该把锁释放掉

		// 当前线程负责执行这个任务
		if (task != nullptr) {
			// task->run();  // 执行任务，把任务的返回值setVal方法给到Result
			task->exec();
		}
		idleThreadSize_++;
		lastTime = std::chrono::high_resolution_clock().now();  // 更新线程执行完任务的时间
	}

	threads_.erase(threadid);
	std::cout << "threadid:" << std::this_thread::get_id() << "exit" << std::endl;
	exitCond_.notify_all();
}

//////线程方法实现
int Thread::generateId_ = 0;

// 线程构造
Thread::Thread(ThreadFunc func)
	: func_(func), threadId_(generateId_++) {
}
// 线程析构
Thread::~Thread() {}

// 启动线程
void Thread::start() {
	// 创建一个线程来执行一个线程函数
	std::thread t(func_, threadId_);  // c++11来说 线程对象t 和线程函数func_
	t.detach();                       // 设置分离线程 pthread_detach pthread_t设置成分离线程
}

int Thread::getId() const {
	return threadId_;
}

///////////////////Task方法的实现
Task::Task()
	: result_(nullptr) {
}

void Task::exec() {
	if (result_ != nullptr) {
		result_->setVal(run());  // 这里发生多态调用
	}
}

void Task::setResult(Result* res) {
	result_ = res;
}

///////////////////Result方法的实现
Result::Result(std::shared_ptr<Task> task, bool isValid)
	: isValid_(isValid), task_(task) {
	task_->setResult(this);
}

Any Result::get() {  // 用户调用的
	if (!isValid_) {
		return "";
	}
	sem_.wait();  // task任务如果没有执行完，这里会阻塞用户的线程
	return std::move(any_);
}

void Result::setVal(Any any) {
	// 存储task的返回值
	this->any_ = std::move(any);
	sem_.post();  // 已经获取的任务的返回值，增加信号量资源
}
