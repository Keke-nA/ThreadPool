#include "threadpool.h"

#include <functional>
#include <iostream>
#include <thread>

const int TASK_MAX_THRESHHOLD = INT32_MAX;
const int THREAD_MAX_THRESHHOLD = 100;
const int THREAD_MAX_IDLE_TIME = 60;  // ��λ��s

// �̳߳ع���
ThreadPool::ThreadPool()
	: initThreadSize_(0), taskSize_(0), curThreadSize_(0), idleThreadSize_(0), threadSizeThreshHold_(THREAD_MAX_THRESHHOLD), taskQueMaxThreshHold_(TASK_MAX_THRESHHOLD), poolMode_(PoolMode::MODE_FIXED), isPoolRunning_(false) {
}

// �̳߳�����
ThreadPool::~ThreadPool() {
	isPoolRunning_ = false;
	notEmpty_.notify_all();

	//�ȴ��̳߳��������е��̷߳���   ������״̬������ & ����ִ��������
	std::unique_lock<std::mutex> lock(taskQuemtx_);
	exitCond_.wait(lock, [&]()->bool {return threads_.size() == 0; });
}

// �����̹߳���ģʽ
void ThreadPool::setMode(PoolMode mode) {
	if (checkRunningState()) {
		return;
	}
	poolMode_ = mode;
}

// ����task����������ߵ���ֵ
void ThreadPool::setTaskQueMaxThreshHold(int threshhold) {
	taskQueMaxThreshHold_ = threshhold;
}

// �����̳߳�chchedģʽ�µ��߳���ֵ
void ThreadPool::setThraedSizeThreshHold(int threshhold) {
	if (checkRunningState()) {
		return;
	}
	if (poolMode_ == PoolMode::MODE_CACHED) {
		threadSizeThreshHold_ = threshhold;
	}
}

// ���̳߳��ύ����  �û����øýӿڣ��������������������
Result ThreadPool::submitTask(std::shared_ptr<Task> sp) {
	// ��ȡ��
	std::unique_lock<std::mutex> lock(taskQuemtx_);

	// �̵߳�ͨ�� �ȴ���������п���
	// �û��ύ�����������������1s�������ж��ύ����ʧ�ܣ�����
	if (!notFull_.wait_for(lock, std::chrono::seconds(1), [&]() -> bool { return taskQue_.size() < (size_t)taskQueMaxThreshHold_; })) {
		// ��ʾnotFull_�ȴ�1���ӣ�������Ȼû������
		std::cerr << "tast queue is full, submit task fail." << std::endl;
		// return task->getResult();  // Task Result  �߳�ִ����Task��Task�ͱ���������
		return Result(sp, false);
	}

	// ����п��࣬������������������
	taskQue_.emplace(sp);
	taskSize_++;

	// ��Ϊ�·�������������п϶������ˣ�notEmpty_�Ͻ���֪ͨ���Ͽ�����߳�ִ������
	notEmpty_.notify_all();

	// chcheģʽ ������ȽϽ��� ������С���������  ��Ҫ�������������Ϳ����̵߳��������ж��Ƿ���Ҫ�����µ��̳߳�����
	if (poolMode_ == PoolMode::MODE_CACHED && taskSize_ > idleThreadSize_ && curThreadSize_ < threadSizeThreshHold_) {
		// �������߳�
		std::cout << ">>> create new thread..." << std::endl;
		auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1));
		int threadId = ptr->getId();
		threads_.emplace(threadId, std::move(ptr));
		// �����߳�
		threads_[threadId]->start();
		// �޸��̸߳�����صı���
		idleThreadSize_++;
		curThreadSize_++;
	}

	// ���������Result����
	// return task->getResult();
	return Result(sp);
}

// �����̳߳�
void ThreadPool::start(int initThreadSize) {
	// �����̳߳ص�����״̬
	isPoolRunning_ = true;

	// ��¼��ʼ�̸߳���
	initThreadSize_ = initThreadSize;
	curThreadSize_ = initThreadSize;

	// �����̶߳���
	for (int i = 0; i < initThreadSize_; ++i) {
		// ����thread�̶߳����ʱ�򣬰��̺߳�������thread�̶߳���
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

	// ���������߳� std::vector<Thread*> threads_;
	for (int i = 0; i < initThreadSize_; ++i) {
		threads_[i]->start();  // ��Ҫȥִ��һ���̺߳���
		idleThreadSize_++;     // ��¼��ʼ�����̵߳�����
	}
}

bool ThreadPool::checkRunningState() const {
	return isPoolRunning_;
}

// �����̺߳���  �̳߳ص������̴߳��������������������
void ThreadPool::threadFunc(int threadid) {  // �̺߳������أ���Ӧ���߳�Ҳ�ͽ�����
	auto lastTime = std::chrono::high_resolution_clock().now();

	while (isPoolRunning_) {
		std::shared_ptr<Task> task;
		{
			// �Ȼ�ȡ��
			std::unique_lock<std::mutex> lock(taskQuemtx_);

			std::cout << "tid:" << std::this_thread::get_id() << "���Ի�ȡ����..." << std::endl;

			// cachedģʽ�£��п����Ѿ������˺ܶ���̣߳����ǿ���ʱ�䳬��60s��
			// Ӧ�ðѶ�����߳̽������յ�������initThreadSize_�������߳�Ҫ���л��գ�
			// ��ǰʱ��-��һ���߳�ִ�е�ʱ��>60s

				// ÿһ���ӷ���һ��  ��ô���ֳ�ʱ���ػ����������ִ�з���
			while (taskQue_.size() == 0) {
				if (poolMode_ == PoolMode::MODE_CACHED) {
					// ����������ʱ������
					if (std::cv_status::timeout == notEmpty_.wait_for(lock, std::chrono::seconds(1))) {
						auto nowTime = std::chrono::high_resolution_clock().now();
						auto dur = std::chrono::duration_cast<std::chrono::seconds>(nowTime - lastTime);
						if (dur.count() >= 60 && curThreadSize_ > initThreadSize_) {
							// ��ʼ���յ�ǰ�߳�
							// ��¼�߳���������ر�����ֵ���޸�
							// ���̶߳�����߳��б�������ɾ��  û�а취ƥ�� threadFunc ��=�� thread����
							// threadid =��thread���� =��ɾ��
							threads_.erase(threadid);  // ��Ҫ��std::this_thread::getid()����cpp�����ɵ�
							curThreadSize_--;
							idleThreadSize_--;
							std::cout << "threadid:" << std::this_thread::get_id() << "exit" << std::endl;
							return;
						}
					}

				}
				else {
					// �ȴ�notEmpty����
					notEmpty_.wait(lock);
				}

				// �̳߳�Ҫ�����������߳���Դ
				if (!isPoolRunning_) {
					threads_.erase(threadid);  // ��Ҫ��std::this_thread::getid()����cpp�����ɵ�
					std::cout << "threadid:" << std::this_thread::get_id() << "exit" << std::endl;
					exitCond_.notify_all();
					return;
				}
			}


			idleThreadSize_--;

			std::cout << "tid:" << std::this_thread::get_id() << "��ȡ����ɹ�" << std::endl;

			// �����������ȡһ���������
			task = taskQue_.front();
			taskQue_.pop();
			taskSize_--;
			// �����Ȼ��ʣ�����񣬼���ͳ���������߳�ִ������
			if (taskQue_.size() > 0) {
				notEmpty_.notify_all();
			}
			// ȡ��һ�����񣬽���֪ͨ,֪ͨ���Լ����ύ��������
			notFull_.notify_all();
		}  // Ӧ�ð����ͷŵ�

		// ��ǰ�̸߳���ִ���������
		if (task != nullptr) {
			// task->run();  // ִ�����񣬰�����ķ���ֵsetVal��������Result
			task->exec();
		}
		idleThreadSize_++;
		lastTime = std::chrono::high_resolution_clock().now();  // �����߳�ִ���������ʱ��
	}

	threads_.erase(threadid);
	std::cout << "threadid:" << std::this_thread::get_id() << "exit" << std::endl;
	exitCond_.notify_all();
}

//////�̷߳���ʵ��
int Thread::generateId_ = 0;

// �̹߳���
Thread::Thread(ThreadFunc func)
	: func_(func), threadId_(generateId_++) {
}
// �߳�����
Thread::~Thread() {}

// �����߳�
void Thread::start() {
	// ����һ���߳���ִ��һ���̺߳���
	std::thread t(func_, threadId_);  // c++11��˵ �̶߳���t ���̺߳���func_
	t.detach();                       // ���÷����߳� pthread_detach pthread_t���óɷ����߳�
}

int Thread::getId() const {
	return threadId_;
}

///////////////////Task������ʵ��
Task::Task()
	: result_(nullptr) {
}

void Task::exec() {
	if (result_ != nullptr) {
		result_->setVal(run());  // ���﷢����̬����
	}
}

void Task::setResult(Result* res) {
	result_ = res;
}

///////////////////Result������ʵ��
Result::Result(std::shared_ptr<Task> task, bool isValid)
	: isValid_(isValid), task_(task) {
	task_->setResult(this);
}

Any Result::get() {  // �û����õ�
	if (!isValid_) {
		return "";
	}
	sem_.wait();  // task�������û��ִ���꣬����������û����߳�
	return std::move(any_);
}

void Result::setVal(Any any) {
	// �洢task�ķ���ֵ
	this->any_ = std::move(any);
	sem_.post();  // �Ѿ���ȡ������ķ���ֵ�������ź�����Դ
}
