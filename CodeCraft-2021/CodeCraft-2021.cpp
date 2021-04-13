/*
 * @Author: your name
 * @Date: 2021-04-04 19:30:29
 * @LastEditTime: 2021-04-13 17:19:00
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \planC\CodeCraft-2021\CodeCraft-2021.cpp
 */
#include "data.h"
#include "scheduler.h"

int total_days_num;            //总请求天数
int foreseen_days_num;          //可预见的天数

unordered_map<string, SoldServer> server_info;
unordered_map<string, SoldVm> VM_info;
vector<SoldServer> sold_servers;
queue<vector<RequestData>> request_datas;
queue<vector<Game_result>> game_result;
int main() {
    freopen("training-0.txt", "r", stdin);
    Parse parse;
    parse.parse_input(server_info, VM_info, request_datas);
    //parse.print(server_info, VM_info, request_datas);
    return 0;
}
