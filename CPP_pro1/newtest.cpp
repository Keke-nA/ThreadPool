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

class Any {
public:
	Any() = default;
	~Any() = default;
	Any(const Any&) = delete;
	Any& operator=(const Any&) = delete;
	Any(Any&&) = default;
	Any& operator=(Any&&) = default;
	template<typename T>
	Any(T data) :any_base(std::make_unique<Derive<T>>(data)) {}
	template<typename T>
	T cast() {
		auto ptr = dynamic_cast<Derive<T>*>(any_base.get());
		if (ptr == nullptr) {
			throw "type is unmatch!";
		}
		return ptr->de_data;
	}


private:
	class Base {
	public:
		virtual ~Base() = default;
	};
	template<typename T>
	class Derive : public Base {
	public:
		Derive(T data_) :de_data(data_) {}
		T de_data;
		~Derive() override {
			std::cout << "Derive::~Base" << std::endl;
		}
	};
private:
	std::unique_ptr<Base> any_base;
};

class Base {
public:
	virtual ~Base() {
		std::cout << "Base::~Base()" << std::endl;
	}
};
template<typename T>
class Derive : public Base {
public:
	Derive(T data_) :de_data(data_) {}
	T de_data;
	~Derive() override {
		std::cout << "Derive::~Derive()" << std::endl;
	}
};

int main() {
	Base* test_base = new Derive<std::string>("adf");
	delete test_base;
	/*auto bound = std::bind(printxy, 5, 10);
	bound();*/
	/*int x = 10;
	Any test_any(10);*/
	//std::cout << test_any.cast<int>();
	/*auto ptr = std::make_unique<testFunc1>();
	ptr->start(10);*/
	return 0;
}