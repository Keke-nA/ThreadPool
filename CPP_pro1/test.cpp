#include <chrono>
#include <iostream>
#include <thread>

#include "threadpool.h"

using ULong = unsigned long long;

class MyTask : public Task {
public:
    MyTask(long begin, long end)
        : begin_(begin), end_(end) {}
    // 问题一：怎么设计run函数的返回值，可以表示任意的类型
    Any run() {  // run方法最终就在线程池分配的线程中去做事情！
        std::cout << "tid:" << std::this_thread::get_id() << "bigin!" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(3));
        ULong sum = 0;
        for (ULong i = begin_; i <= end_; ++i) {
            sum += i;
        }
        std::cout << "tid:" << std::this_thread::get_id() << "end!" << std::endl;
        return sum;
        ;
    }

private:
    long begin_;
    long end_;
};

int main() {
    {
        ThreadPool pool;
        // 用户自己设置线程池的工作模式
        pool.setMode(PoolMode::MODE_CACHED);
        // 开始启动线程池
        pool.start(4);

        Result res1 = pool.submitTask(std::make_shared<MyTask>(1, 10000));
        Result res2 = pool.submitTask(std::make_shared<MyTask>(10001, 20000));
        Result res3 = pool.submitTask(std::make_shared<MyTask>(20001, 30000));
        pool.submitTask(std::make_shared<MyTask>(20001, 30000));
        pool.submitTask(std::make_shared<MyTask>(20001, 30000));
        pool.submitTask(std::make_shared<MyTask>(20001, 30000));

        // 随着task被执行完，task对象没了，依赖于task对象的Result对象也没了
        ULong sum1 = res1.get().cast_<ULong>();  // get返回了一个Any类型，怎么转成具体的类型呢
        ULong sum2 = res2.get().cast_<ULong>();
        ULong sum3 = res3.get().cast_<ULong>();

        // Master-Slave线程模型
        // Master线程用来分解任务，然后给各个Slave线程分配任务
        // 等待各个Slave线程执行完任务，返回结果
        // Master线程合并各个任务结果，输出
        std::cout << sum1 + sum2 + sum3 << std::endl;

        ULong sum = 0;
        for (ULong i = 0; i <= 300000000; i++) {
            sum += i;
        }
        std::cout << sum << std::endl;

        /*
            pool.submitTask(std::make_shared<MyTask>());
            pool.submitTask(std::make_shared<MyTask>());
            pool.submitTask(std::make_shared<MyTask>());
            pool.submitTask(std::make_shared<MyTask>());
            pool.submitTask(std::make_shared<MyTask>());
            pool.submitTask(std::make_shared<MyTask>());
            pool.submitTask(std::make_shared<MyTask>());
            pool.submitTask(std::make_shared<MyTask>());
            pool.submitTask(std::make_shared<MyTask>());
            pool.submitTask(std::make_shared<MyTask>());
        */
    }

	getchar();
    return 0;
}