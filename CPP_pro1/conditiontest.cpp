#include <iostream>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

//std::mutex mtx;
//std::condition_variable cv;
//std::queue<int> q;
//constexpr int MAX_SIZE = 10;
//
//void Producer() {
//	for (int i = 0; i < 20; i++) {
//		std::this_thread::sleep_for(std::chrono::milliseconds(100));
//		{
//			std::unique_lock<std::mutex> lock(mtx);
//			cv.wait(lock, [] {return q.size() < MAX_SIZE; });
//			q.emplace(i);
//			std::cout << "Producer:" << i << std::endl;
//		}
//		cv.notify_all();
//	}
//}
//
//void Consumer() {
//	while (true) {
//		int data;
//		{
//			std::unique_lock<std::mutex> lock(mtx);
//			cv.wait(lock, [] {return !q.empty(); });
//			data = q.front();
//			std::cout << "Consumer:" << data << std::endl;
//			q.pop();
//		}
//		cv.notify_all();
//		if (data == 19) {
//			break;
//		}
//	}
//}
//std::mutex mtx;
//std::condition_variable cv;
//int active_readers = 0;
//int waiting_writers = 0;
//bool writer_active = false;
//int shared_data = 0;

//void reader(int id) {
//	std::unique_lock<std::mutex> lock(mtx);
//	cv.wait(lock, [] {return !writer_active && waiting_writers == 0; });
//	active_readers++;
//	lock.unlock();
//
//	std::cout << "reader:" << id << "  read_data:" << shared_data << std::endl;
//
//	lock.lock();
//	active_readers--;
//	if (active_readers == 0) {
//		cv.notify_all();
//	}
//	lock.unlock();
//}
//
//void writer(int id) {
//	std::unique_lock<std::mutex> lock(mtx);
//	waiting_writers++;
//	cv.wait(lock, [] {return active_readers == 0 && !writer_active; });
//	waiting_writers--;
//	writer_active = true;
//	lock.unlock();
//
//	shared_data++;
//	std::cout << "writer:" << id << "  write_data:" << shared_data << std::endl;
//
//	lock.lock();
//	writer_active = false;
//	cv.notify_all();
//	lock.unlock();
//}

//std::mutex mtx;
//void func(int& x) {
//	for (int i = 0; i < 1000; i++) {
//		std::lock_guard<std::mutex> lock(mtx);
//		x++;
//	}
//}
constexpr int BUFFER_SIZE = 5;
std::queue<int> buffer;
int buffer_empty = BUFFER_SIZE;
int buffer_full = 0;
std::mutex mtx;
std::condition_variable full;
std::condition_variable empty;

void producer() {
	for (int i = 0; i < 10; i++) {
		{
			std::unique_lock<std::mutex> lock(mtx);
			empty.wait(lock, [] {return buffer_empty > 0; });
			buffer.push(i);
			buffer_empty--;
			buffer_full++;
			std::cout << "produce:" << i << std::endl;
		}
		full.notify_all();
	}
}

void consumer() {
	while (true) {
		int temp;
		{
			//std::this_thread::sleep_for(std::chrono::milliseconds(50));
			std::unique_lock<std::mutex> lock(mtx);
			full.wait(lock, [] {return buffer_full > 0; });
			temp = buffer.front();
			buffer.pop();
			buffer_empty++;
			buffer_full--;
			std::cout << "consumer:" << temp << std::endl;
			empty.notify_all();
			lock.unlock();
		}
		if (temp == 9) {
			break;
		}
	}
}

int main() {
	/*std::thread t1(Producer);
	std::thread t2(Consumer);
	t1.join();
	t2.join();*/
	/*std::vector<std::thread> vec;
	for (int i = 0; i < 5; i++) {
		vec.emplace_back(writer, i);
		vec.emplace_back(reader, i);
	}
	for (auto& th : vec) {
		th.join();
	}*/
	/*int x = 0;
	func(x);
	std::vector<std::thread> vec_thread;*/
	/*for (int i = 0; i < 5; i++) {
		vec_thread.emplace_back(func,x);
	}
	for (auto& thd : vec_thread) {
		thd.join();
	}
	std::cout << x << std::endl;*/

	std::thread t1(producer);
	std::thread t2(consumer);
	t2.join();
	t1.join();
	return 0;
}