/*
 * @Author: your name
 * @Date: 2021-04-04 19:30:38
 * @LastEditTime: 2021-04-13 15:08:50
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \planC\CodeCraft-2021\H1.h
 */
#ifndef data_H
#define data_H
#include <iostream>
#include <algorithm>
#include <string>
#include <vector>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <ctime>
#include <fstream>
#include <cmath>
#include <climits>
#include <cfloat>
using namespace std;

int total_days_num;            //总请求天数
int foreseen_days_num;          //可预见的天数

class SoldServer {
public:
    SoldServer() {}
    ~SoldServer() {}
public:
    string server_name;
    int cpu;
    int memory;
    int hardware_cost;
    int daily_cost;
};
class SoldVm {
public:
    SoldVm() {}
    ~SoldVm() {}
public:
    string vm_name;
    int cpu;
    int memory;
    int deployment_way;
};
class RequestData {
public:
    RequestData() {}
    ~RequestData() {}
public:
    string operation;
    string vm_name;
    int vm_id;
    int duration_day;
    int offer_price;
};
class Game_result {
public:
    Game_result() {}
    ~Game_result() {}
public:
    int iswin;
    int opponent_offer;    
};
ostream& operator<<(ostream& os, const SoldServer& server);
ostream& operator<<(ostream& os, const SoldVm& VM);
ostream& operator<<(ostream& os, const RequestData& request_data);

class Parse {
public:
    Parse() {}
    ~Parse() {}
public:
    void parse_input(unordered_map<string, SoldServer>&, unordered_map<string, SoldVm>&, queue<vector<RequestData>>&);
    void print(unordered_map<string, SoldServer>&, unordered_map<string, SoldVm>&, queue<vector<RequestData>>);
    void parse_game(queue<vector<Game_result>>&, int);
    queue<vector<RequestData>> request_datas;
private:
    void parse_server_info(unordered_map<string, SoldServer>&);
    void parse_vm_info(unordered_map<string, SoldVm>&);
    void parse_request(queue<vector<RequestData>>&, int);
};
void Parse::parse_input(unordered_map<string, SoldServer>& server_info, unordered_map<string, SoldVm>& VM_info, queue<vector<RequestData>>& request_datas) {
    parse_server_info(server_info);
    parse_vm_info(VM_info);
    cin >> total_days_num >> foreseen_days_num;
    parse_request(request_datas, foreseen_days_num);
}
void Parse::parse_server_info(unordered_map<string, SoldServer>& server_info) {
    int server_num;
    cin >> server_num;
    string server_name, cpu, memory, hardware_cost, daily_cost;
    for (int i = 0; i < server_num; ++i) {
        SoldServer sold_server;
        cin >> server_name >> cpu >> memory >> hardware_cost >> daily_cost;
        sold_server.server_name = server_name.substr(1, server_name.size() - 2);
        sold_server.cpu = stoi(cpu.substr(0, cpu.size() - 1)) / 2;
        sold_server.memory = stoi(memory.substr(0, memory.size() - 1)) / 2;
        sold_server.hardware_cost = stoi(hardware_cost.substr(0, hardware_cost.size() - 1));
        sold_server.daily_cost = stoi(daily_cost.substr(0, daily_cost.size() - 1));
        
        server_info[sold_server.server_name] = sold_server;
    }
}
void Parse::parse_vm_info(unordered_map<string, SoldVm>& VM_info) {
    int vm_num;
    cin >> vm_num;
    string vm_name, cpu, memory, deployment_way;
    for (int i = 0; i < vm_num; ++i) {
        SoldVm sold_vm;
        cin >> vm_name >> cpu >> memory >> deployment_way;
        sold_vm.vm_name = vm_name.substr(1, vm_name.size() - 2);
        sold_vm.cpu = stoi(cpu.substr(0, cpu.size() - 1));
        sold_vm.memory = stoi(memory.substr(0, memory.size() - 1));
        sold_vm.deployment_way = stoi(deployment_way.substr(0, deployment_way.size() - 1));
        if (sold_vm.deployment_way == 1) {
            sold_vm.cpu /= 2;
            sold_vm.memory /= 2;
        }
        VM_info[sold_vm.vm_name] = sold_vm;
    }    
}
void Parse::parse_request(queue<vector<RequestData>>& request_datas, int days_num) {
    int operation_num;
    string operation, vm_name, vm_id, vm_duration, vm_offer;
    for (int i = 0; i < days_num; ++i) {
        cin >> operation_num;
        vector<RequestData> request_data;
        for (int j = 0; j < operation_num; ++j) {
            cin >> operation;
            operation = operation.substr(1, operation.size() - 2);
            if (operation == "add") {
                RequestData data;
                data.operation = "add";
                cin >> vm_name >> vm_id >> vm_duration >> vm_offer;
                data.vm_name = vm_name.substr(0, vm_name.size() - 1);
                data.vm_id = stoi(vm_id.substr(0, vm_id.size() - 1));
                data.duration_day = stoi(vm_duration.substr(0, vm_duration.size() - 1));
                data.offer_price = stoi(vm_offer.substr(0, vm_offer.size() - 1));
                
                request_data.emplace_back(data);
            } else if (operation == "del") {
                RequestData data;
                data.operation = "del";
                cin >> vm_id;
                data.vm_id = stoi(vm_id.substr(0, vm_id.size() - 1));
                request_data.emplace_back(data);
            }
        }
        request_datas.emplace(request_data);
    }    
}
void Parse::parse_game(queue<vector<Game_result>>& game_results, int num) {
    string iswin, opponent_offer;
    vector<Game_result> game_result;
    for (int i = 0; i < num; ++i) {
        cin >> iswin >> opponent_offer;
        Game_result result;
        result.iswin = stoi(iswin.substr(1, iswin.size() - 2));
        result.opponent_offer = stoi(opponent_offer.substr(0, opponent_offer.size() - 1));
        game_result.emplace_back(result);
    }
    game_results.push(game_result);
}
void Parse::print(unordered_map<string, SoldServer>& server_info, unordered_map<string, SoldVm>& VM_info, queue<vector<RequestData>> request_datas) {
    for (auto& server : server_info) {
        cout << server.second << endl;
    }
    for (auto& VM : VM_info) {
        cout << VM.second << endl;
    }
    int queue_len = request_datas.size();
    for (int i = 0; i < queue_len; ++i) {
        auto requests = request_datas.front();
        for (auto request : requests) {
            cout << request << endl;
        }
        request_datas.pop();
    }
}

ostream& operator<<(ostream& os, const SoldServer& server) {
    os << "server name : " << server.server_name << ", server cpu : " << server.cpu << ", server memory : " << server.memory
        << ", server hardware cost : " << server.hardware_cost << ", server daily cost : " << server.daily_cost;
    return os;
}
ostream& operator<<(ostream& os, const SoldVm& VM) {
    os << "VM name : " << VM.vm_name << ", VM cpu : " << VM.cpu << ", VM memory : " << VM.memory
        << ", VM deployment way : " << VM.deployment_way;
    return os;
}
ostream& operator<<(ostream& os, const RequestData& request_data) {
    if (request_data.operation == "add") {
        os << request_data.operation << ' ' << request_data.vm_name << ' ' << request_data.vm_id << ' ' << request_data.duration_day 
            << ' ' << request_data.offer_price;
    } else {
        os << request_data.operation << ' ' << request_data.vm_id;
    }
    return os;
}
ostream& operator<<(ostream& os, const Game_result& game_result) {
    cout << '(' << game_result.iswin << ", " << game_result.opponent_offer << ')';
}

#endif