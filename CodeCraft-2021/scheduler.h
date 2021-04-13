/*
 * @Author: your name
 * @Date: 2021-04-04 19:30:43
 * @LastEditTime: 2021-04-13 16:16:26
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \planC\CodeCraft-2021\H2.h
 */
#include "data.h"
#ifndef SCHEDULER_H
#define SCHEDULER_H

extern int total_days_num;  
extern int foreseen_days_num;
class Scheduler {
public:
    Scheduler() {}
    ~Scheduler() {}
    void solve_problem();
private: 
    void simulate_deploy();
    void purchase();
    void deployment();
};
void Scheduler::solve_problem() {
    for (int i = 0; i < total_days_num; ++i) {
        
    }
}


#endif