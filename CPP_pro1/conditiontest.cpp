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
std::mutex mtx;
std::condition_variable cv;
int active_readers = 0;
int waiting_writers = 0;
bool writer_active = false;
int shared_data = 0;

void reader(int id) {
	std::unique_lock<std::mutex> lock(mtx);
	cv.wait(lock, [] {return !writer_active && waiting_writers == 0; });
	active_readers++;
	lock.unlock();

	std::cout << "reader:" << id << "  read_data:" << shared_data << std::endl;

	lock.lock();
	active_readers--;
	if (active_readers == 0) {
		cv.notify_all();
	}
	lock.unlock();
}

void writer(int id) {
	std::unique_lock<std::mutex> lock(mtx);
	waiting_writers++;
	cv.wait(lock, [] {return active_readers == 0 && !writer_active; });
	waiting_writers--;
	writer_active = true;
	lock.unlock();

	shared_data++;
	std::cout << "writer:" << id << "  write_data:" << shared_data << std::endl;

	lock.lock();
	writer_active = false;
	cv.notify_all();
	lock.unlock();
}

int main() {
	/*std::thread t1(Producer);
	std::thread t2(Consumer);
	t1.join();
	t2.join();*/
	std::vector<std::thread> vec;
	for (int i = 0; i < 5; i++) {
		vec.emplace_back(writer, i);
		vec.emplace_back(reader, i);
	}
	for (auto& th : vec) {
		th.join();
	}
	return 0;
}