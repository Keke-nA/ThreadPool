#include<iostream>
#include"mythreadpool.h"
#include<vector>

class MyTask :public Task {
public:
	MyTask(std::string str) :mytask_string(str) {}
	MyAny run() override {
		return MyAny(mytask_string);
	}
public:
	std::string mytask_string;
};

int main() {
	MyThreadPool pool;
	pool.start(4);
	auto task1 = pool.submitTask(std::make_shared<MyTask>("hello!"));
	auto task2 = pool.submitTask(std::make_shared<MyTask>("world!"));
	auto task3 = pool.submitTask(std::make_shared<MyTask>("love!"));
	auto task4 = pool.submitTask(std::make_shared<MyTask>("ustc!"));
	std::cout << "12345" << std::endl;
	auto ans1 = task1.getResult().to_cast<std::string>();
	auto ans2 = task2.getResult().to_cast<std::string>();
	auto ans3 = task3.getResult().to_cast<std::string>();
	auto ans4 = task4.getResult().to_cast<std::string>();
	std::cout << ans1 + " " + ans2 + " " + ans3 + " " + ans4 << std::endl;
	/*std::vector<int> vec{ 1,2,3,4,5 };
	MyAny any(vec);
	std::vector<int> y = any.to_cast<std::vector<int>>();
	for (const auto& tmp : y) {
		std::cout << tmp << " ";
	}*/
	std::this_thread::sleep_for(std::chrono::seconds(3));
	return 0;
}