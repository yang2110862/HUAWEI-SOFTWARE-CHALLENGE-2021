/*
 * @Author: your name
 * @Date: 2021-04-04 19:30:43
 * @LastEditTime: 2021-04-13 17:15:16
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \planC\CodeCraft-2021\H2.h
 */
#include "data.h"
#ifndef SCHEDULER_H
#define SCHEDULER_H

class Scheduler {
public:
    Scheduler() {}
    ~Scheduler() {}
    void solve_problem();
private: 
    void pricing(vector<RequestData>&);
    void simulate_deploy();
    void purchase();
    void deployment();
private:
    vector<RequestData> intraday_requests;
};
void Scheduler::solve_problem() {
    for (int i = 0; i < total_days_num; ++i) {
        
    }
}
void Scheduler::pricing(vector<RequestData>& request_data) {

}

#endif