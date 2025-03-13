#include<iostream>
#include<functional>
#include<memory>


class testFunc {
public:
	using Function = std::function<void(int)>;
	testFunc(Function f) :fuc(f) {}
	void startrun(int x) {
		fuc(x);
	}
private:
	Function fuc;
};

class testFunc1 {
public:
	void start(int stt) {
		//auto temp = std::make_unique<testFunc>(std::bind(&testFunc1::funcRun, this, std::placeholders::_1));
		auto temp = std::make_unique<testFunc>(
			[this](int arg) {
				this->funcRun(arg);
			}
		);
		temp->startrun(stt);
	}
	void funcRun(int x) {
		std::cout << "testFunc:" << x << std::endl;
	}
};

void printxy(int x, int y) {
	std::cout << x << "  " << y << std::endl;
}

int main() {
	/*auto bound = std::bind(printxy, 5, 10);
	bound();*/
	auto ptr = std::make_unique<testFunc1>();
	ptr->start(10);
	return 0;
}