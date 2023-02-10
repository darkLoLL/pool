//
// Created by cat on 2023/2/9.
//
#pragma once
#define TEST
#ifdef TEST
#include <iostream>
#endif

#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>
#include <chrono>
#include "Task.h"

class Theadpool {
private:
    std::queue<Task> tasks;
    std::thread managerid;
    std::vector<std::thread*> workerids;

    int maxnum; // 最多可工作的线程数
    int busynum; // 正在工作的线程数
    int exitnum; // 要自我销毁的线程个数
    std::mutex mutexpool; // 锁线程池
    std::condition_variable notempty; // 任务队列的锁

    bool stop; // 停止所有任务线程
    bool disapear; // 终止所有线程
private:
    static void manager(void *arg);
    static void worker(void *arg);
    void exitthread();
public:
    Theadpool(int max);
    ~Theadpool();
    void starttask();
    void stoptask();
    template<class Func>
    void addtask(Func f);
    template<class Func, class ... Args>
    void addtask(Func f, Args...args);
};

///
//int maxnum; // 最多可工作的线程数
//int busynum; // 正在工作的线程数
//std::mutex mutexpool; // 锁线程池
//std::condition_variable notempty; // 任务队列的锁
//bool stop; // 停止所有任务线程
//bool disapear; // 终止所有线程

Theadpool::Theadpool(int max) {
    this->maxnum = max;
    this->busynum = 0;
    this->stop = true;
    this->disapear = false;
    this->exitnum = 0;
    this->managerid = std::thread(Theadpool::manager, this);
}

Theadpool::~Theadpool() {
    this->mutexpool.lock();
    this->disapear = true;
    this->mutexpool.unlock();
}

void Theadpool::manager(void *arg) {
    Theadpool* pool = static_cast<Theadpool*>(arg);
    while (!pool->disapear) {
        // 线程池是否停止
        if (pool->stop) {

        } else {
            pool->mutexpool.lock();
            int maxnum = pool->maxnum;
            int tasknum = pool->tasks.size();
            int busy = pool->busynum;
            int livenum = pool->workerids.size();
            pool->mutexpool.unlock();
            // 任务数 > 现有worker线程数 现有线程个数 < 最大线程数
            if (tasknum > livenum && livenum < maxnum) {
                for (int i = 0; i < 2 && pool->workerids.size() <= pool->maxnum; ++i) {
                    pool->mutexpool.lock();
                    pool->workerids.push_back(new std::thread(Theadpool::worker, arg));
                    pool->mutexpool.unlock();
                }
            }
            pool->mutexpool.lock();
            if (busy * 2 < maxnum) {
                pool->exitnum = 2;
                pool->notempty.notify_all();
            }
            pool->mutexpool.unlock();
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}

void Theadpool::worker(void *arg) {
    Theadpool* pool = static_cast<Theadpool*>(arg);
    if (pool == nullptr) {return;}

    while (true) {
        while (!pool->mutexpool.try_lock()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        std::unique_lock<std::mutex> lock(pool->mutexpool);
        if (pool->tasks.empty() && !pool->stop) {
            pool->notempty.wait(lock);
        }
        if (pool->disapear){
            pool->exitthread();
            pool->mutexpool.unlock();
            return; // 线程退出
        }
        Task task;
        task = pool->tasks.front();
        pool->tasks.pop();
        pool->busynum++;
#ifdef TEST
        std::cout << "thread: " << std::this_thread::get_id() << " work." << std::endl;
#endif
        pool->mutexpool.unlock();
        // 运行任务函数
        task();
        // 任务结束
        pool->mutexpool.lock();
        pool->busynum--;
#ifdef TEST
        std::cout << "thread: " << std::this_thread::get_id() << " end." << std::endl;
#endif
        pool->mutexpool.unlock();
    }
}

void Theadpool::starttask() {
    this->stop = false;
}

void Theadpool::stoptask() {
    this->stop = true;
}

void Theadpool::exitthread() {
    std::thread::id id = std::this_thread::get_id();
    int i = 0;
    for (std::thread*& it:this->workerids) {
        if (it->get_id() == id) {
            it->detach();
            workerids.erase(workerids.begin() + i);
        }
        ++i;
    }
}

template<class Func, class... Args>
void Theadpool::addtask(Func f, Args...args) {
    this->mutexpool.lock();
    this->tasks.push(Task(f, args...));
    this->mutexpool.unlock();
}

template<class Func>
void Theadpool::addtask(Func f) {
    this->mutexpool.lock();
    this->tasks.push(Task(f));
    this->mutexpool.unlock();
}