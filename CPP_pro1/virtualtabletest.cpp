#include<iostream>

class A {
public:
	virtual void fa() {
		std::cout << "A::fa()" << std::endl;
	}
	virtual void fb() {
		std::cout << "A::fb()" << std::endl;
	}
	int x = 5;
};

class B : public A {
	void fa() override {
		std::cout << "B::fa()" << std::endl;
	}
};

int main() {
	using u64 = long long;
	using fuc = void(*)();
	A* a = new A();
	//a.x = 4;
	B* b = new B();
	B* c = new B();
	//B b{};
	A* a_b = new B();
	std::cout << typeid(decltype(*a_b)).name() << "  " << typeid(*a_b).name() << std::endl;
	u64* p_1 = (u64*)(a);
	u64* p_2 = (u64*)(b);
	u64* p_3 = (u64*)(c);
	u64* arr_1 = (u64*)(*p_1);
	u64* arr_2 = (u64*)(*p_2);
	u64* arr_3 = (u64*)(*p_3);
	arr_1 = arr_3;
	std::cout << *p_1 << "  " << *p_2 << "  " << *p_3 << std::endl;
	std::cout << arr_1 << "  " << arr_2 << "  " << arr_3 << std::endl;
	fuc f_1 = (fuc)arr_1[0];
	f_1();
	fuc f_2 = (fuc)arr_2[0];
	f_2();
	int* y = (int*)(p_1 + 1);
	std::cout << *y << std::endl;
	return 0;
}