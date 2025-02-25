#include <chrono>
#include <iostream>
#include <thread>

#include "threadpool.h"

using ULong = unsigned long long;

class MyTask : public Task {
public:
    MyTask(long begin, long end)
        : begin_(begin), end_(end) {}
    // ����һ����ô���run�����ķ���ֵ�����Ա�ʾ���������
    Any run() {  // run�������վ����̳߳ط�����߳���ȥ�����飡
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
        // �û��Լ������̳߳صĹ���ģʽ
        pool.setMode(PoolMode::MODE_CACHED);
        // ��ʼ�����̳߳�
        pool.start(4);

        Result res1 = pool.submitTask(std::make_shared<MyTask>(1, 10000));
        Result res2 = pool.submitTask(std::make_shared<MyTask>(10001, 20000));
        Result res3 = pool.submitTask(std::make_shared<MyTask>(20001, 30000));
        pool.submitTask(std::make_shared<MyTask>(20001, 30000));
        pool.submitTask(std::make_shared<MyTask>(20001, 30000));
        pool.submitTask(std::make_shared<MyTask>(20001, 30000));

        // ����task��ִ���꣬task����û�ˣ�������task�����Result����Ҳû��
        ULong sum1 = res1.get().cast_<ULong>();  // get������һ��Any���ͣ���ôת�ɾ����������
        ULong sum2 = res2.get().cast_<ULong>();
        ULong sum3 = res3.get().cast_<ULong>();

        // Master-Slave�߳�ģ��
        // Master�߳������ֽ�����Ȼ�������Slave�̷߳�������
        // �ȴ�����Slave�߳�ִ�������񣬷��ؽ��
        // Master�̺߳ϲ����������������
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