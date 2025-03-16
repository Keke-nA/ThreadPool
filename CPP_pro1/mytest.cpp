#include<iostream>
#include"mythreadpool.h"

int main() {
	MyThreadPool pool;
	pool.start(4);
	for (int i = 0; i < 5; i++) {
		auto temp_task = std::make_shared<Task>();
		pool.submitTask(temp_task);
	}
	//std::this_thread::sleep_for(std::chrono::seconds(2));
	return 0;
}