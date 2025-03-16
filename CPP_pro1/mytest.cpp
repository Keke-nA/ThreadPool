#include<iostream>
#include"mythreadpool.h"
#include<vector>

int main() {
	/*MyThreadPool pool;
	pool.start(4);
	for (int i = 0; i < 5; i++) {
		auto temp_task = std::make_shared<Task>();
		pool.submitTask(temp_task);
	}*/
	std::vector<int> vec{ 1,2,3,4,5 };
	MyAny any(vec);
	std::vector<int> y = any.to_cast<std::vector<int>>();
	for (const auto& tmp : y) {
		std::cout << tmp << " ";
	}
	//std::this_thread::sleep_for(std::chrono::seconds(2));
	return 0;
}