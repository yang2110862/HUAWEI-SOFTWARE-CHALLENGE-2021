/*
 * @Author: your name
 * @Date: 2021-04-04 19:30:29
 * @LastEditTime: 2021-04-09 22:44:58
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \planC\CodeCraft-2021\CodeCraft-2021.cpp
 */
#include "data.h"
unordered_map<string, SoldServer> server_info;
unordered_map<string, SoldVm> VM_info;
queue<vector<RequestData>> request_datas;

int main() {
    freopen("training.txt", "r", stdin);
    Parse parse;
    parse.parse_input(server_info, VM_info, request_datas);
    parse.print(server_info, VM_info, request_datas);
    return 0;
}