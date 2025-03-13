#include <iostream>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

std::mutex mtx;
std::condition_variable cv;
std::queue<int> q;
constexpr int MAX_SIZE = 10;

void Producer() {
	for (int i = 0; i < 20; i++) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		{
			std::unique_lock<std::mutex> lock(mtx);
			cv.wait(lock, [] {return q.size() < MAX_SIZE; });
			q.emplace(i);
			std::cout << "Producer:" << i << std::endl;
		}
		cv.notify_all();
	}
}

void Consumer() {
	while (true) {
		int data;
		{
			std::unique_lock<std::mutex> lock(mtx);
			cv.wait(lock, [] {return !q.empty(); });
			data = q.front();
			std::cout << "Consumer:" << data << std::endl;
			q.pop();
		}
		cv.notify_all();
		if (data == 19) {
			break;
		}
	}
}


int main() {
	std::thread t1(Producer);
	std::thread t2(Consumer);
	t1.join();
	t2.join();
	return 0;
}