//
// Created by cat on 2023/2/9.
//

#include "Task.h"
#include "Theadpool.h"
#include <iostream>
void look(int i) {
    std::cout << std::this_thread::get_id() << i << std::endl;
}

int main() {
    Theadpool *pool = new Theadpool(5);

    for (int i = 0; i < 20; ++i) {
        pool->addtask(look, i);
    }
    pool->starttask();
    std::this_thread::sleep_for(std::chrono::seconds(20));
    return 0;
}