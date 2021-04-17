/*
 * @Author: your name
 * @Date: 2021-04-04 19:30:38
 * @LastEditTime: 2021-04-14 22:06:40
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

extern int total_days_num;            //总请求天数
extern int foreseen_days_num;          //可预见的天数

extern unordered_map<string, SoldServer> server_info;
extern unordered_map<string, SoldVm> VM_info;
extern vector<SoldServer> sold_servers;
extern queue<vector<RequestData>> request_datas;

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
    int duration;
    int user_price;
};
class PurchasedServer {
public:
    PurchasedServer() {}
    ~PurchasedServer() {}
public:
    string server_name;
    int server_id = -1;
    int total_cpu;
    int total_memory;
    int daily_cost;
    int A_remain_cpu;
    int A_remain_memory;
    int B_remain_cpu;
    int B_remain_memory;
    bool can_deploy_A = false;
    bool can_deploy_B = false;
    unordered_set<int> A_vm_id;
    unordered_set<int> B_vm_id;
    unordered_set<int> AB_vm_id;
};
class MigrationInfo {
public:
    MigrationInfo() {}
    ~MigrationInfo() {}
public:
    int vm_id;
    PurchasedServer* server;
    char node;
};
class VmIdfo {
public:
    VmIdfo() {}
    ~VmIdfo() {}
public:
    PurchasedServer* purchase_server;
    char node;
    int vm_id;
    string vm_name;
    int cpu;
    int memory;
    int delete_day;
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
        sold_servers.emplace_back(sold_server); 
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
                data.duration = stoi(vm_duration.substr(0, vm_duration.size() - 1));
                data.user_price = stoi(vm_offer.substr(0, vm_offer.size() - 1));
                
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
        os << request_data.operation << ' ' << request_data.vm_name << ' ' << request_data.vm_id << ' ' << request_data.duration 
            << ' ' << request_data.user_price;
    } else {
        os << request_data.operation << ' ' << request_data.vm_id;
    }
    return os;
}

class Statistics {  //选计算虚拟机的统计量，再去掉劣势服务器，再计算去掉劣势服务器后的服务器统计量，再使用统计数据
public:
    Statistics() {} 
    ~Statistics() {}
    void compute_statistics_of_VM(vector<SoldVm>& sold_VM);       //计算虚拟机的统计量
    void del_bad_server(vector<SoldServer>& sold_servers);          //去掉相对劣势的服务器
    void compute_statistics_of_server(vector<SoldServer>& sold_servers);  //计算服务器的统计量
    vector<double> linear_regression(vector<SoldServer>& sold_servers);
    //需要什么值可以加上对应的返回函数
public:
    int server_max_cpu = 0;
    int server_max_memory = 0;
    int server_min_cpu = 512;          //每个节点上是1024的一半
    int server_min_memory = 512;
    double server_average_cpu;  //单个节点的平均cpu
    double server_average_memory;
    double server_middle_cpu;
    double server_middle_memory;
public:
    int VM_max_cpu = 0;
    int VM_max_memory = 0;
    int VM_min_cpu = 512;       //每个节点至多512      
    int VM_min_memory = 512;
    double VM_average_cpu;     //单个节点的平均cpu
    double VM_average_memory;
    double VM_middle_cpu;
    double VM_middle_memory;

    int k = 10;
    int kth_small_VM_cpu;
    int kth_small_VM_memory;
private:
    double cost_function(vector<vector<double>>& var_x, vector<double>& var_y, vector<double>& coeff);
    void gradient_descent(vector<vector<double>>& var_x, vector<double>& var_y, vector<double>& coeff, double alpha, int iters);
};
void Statistics::compute_statistics_of_server(vector<SoldServer>& sold_servers) {
    int len = sold_servers.size();
    double total_server_cpu = 0;
    double total_server_memory = 0;
    for (auto& sold_server : sold_servers) {
        total_server_cpu += sold_server.cpu;
        total_server_memory += sold_server.memory;
    }
    server_average_cpu = total_server_cpu / len;
    server_average_memory = total_server_memory / len;
    sort(sold_servers.begin(), sold_servers.end(),                              
        [](SoldServer& a, SoldServer&b){ return a.cpu < b.cpu;});  //可以不排序改成找第k大(后面有效果再优化)
    server_max_cpu = sold_servers[len - 1].cpu;
    server_min_cpu = sold_servers[0].cpu;
    server_middle_cpu = len & 1 == 1 ? sold_servers[len >> 1].cpu : 1.0 * (sold_servers[len >> 1].cpu + sold_servers[(len >> 1) - 1].cpu ) / 2;

    sort(sold_servers.begin(), sold_servers.end(),                              
        [](SoldServer& a, SoldServer&b){ return a.memory < b.memory;});  
    server_max_memory = sold_servers[len - 1].memory;
    server_min_memory = sold_servers[0].memory;
    server_middle_memory = len & 1 == 1 ? sold_servers[len >> 1].memory : 1.0 * (sold_servers[len >> 1].memory + sold_servers[(len >> 1) - 1].memory ) / 2;
}
void Statistics::compute_statistics_of_VM(vector<SoldVm>& sold_VMs) {
    int len = sold_VMs.size();
    int node_num = 0; //计算一共需要多少个节点
    for (auto& sold_VM : sold_VMs) {
        if (sold_VM.deployment_way == 1) node_num += 2;
        else ++node_num;
    }
    double total_VM_cpu = 0;                 //计算的是一个节点上所需要的最大cpu
    double total_VM_memory = 0;
    for (auto& sold_VM : sold_VMs) {
        if (sold_VM.deployment_way == 1) {
            total_VM_cpu += sold_VM.cpu * 2;
            total_VM_memory += sold_VM.memory * 2;
        } else {
            total_VM_cpu += sold_VM.cpu;
            total_VM_memory += sold_VM.memory;
        }
    }
    VM_average_cpu = total_VM_cpu / node_num;
    VM_average_memory = total_VM_memory / node_num;
    sort(sold_VMs.begin(), sold_VMs.end(),                              
        [](SoldVm& a, SoldVm&b){ return a.cpu < b.cpu;});  //可以不排序改成找第k大(后面有效果再优化)
    VM_max_cpu = sold_VMs[len - 1].cpu;
    VM_min_cpu = sold_VMs[0].cpu;
    VM_middle_cpu = len & 1 == 1 ? sold_VMs[len >> 1].cpu : 1.0 * (sold_VMs[len >> 1].cpu + sold_VMs[(len >> 1) - 1].cpu ) / 2;
    kth_small_VM_cpu = k <= len ? sold_VMs[k - 1].cpu : sold_VMs[len - 1].cpu;

    sort(sold_VMs.begin(), sold_VMs.end(),                              
        [](SoldVm& a, SoldVm&b){ return a.memory < b.memory;});  
    VM_max_memory = sold_VMs[len - 1].memory;
    VM_min_memory = sold_VMs[0].memory;
    VM_middle_memory = len & 1 == 1 ? sold_VMs[len >> 1].memory : 1.0 * (sold_VMs[len >> 1].memory + sold_VMs[(len >> 1) - 1].memory ) / 2;
    kth_small_VM_memory = k <= len ? sold_VMs[k - 1].memory : sold_VMs[len - 1].memory;
}
void Statistics::del_bad_server(vector<SoldServer>& sold_servers) {       //去掉特别劣势的服务器
    vector<SoldServer> new_sold_servers;
    int len = sold_servers.size();
    bool flag;  //是否要删除
    for (int i = 0; i < len; ++i) {  
        flag = false;         
        for (int j = 0; j < len; ++j) {   //能否找到一个可以代替sold_servers[i] 的服务器
            if (j == i) continue;     //跳过自身
            if (sold_servers[j].cpu < VM_max_cpu || sold_servers[j].memory < VM_max_memory) continue;      //要是不能容下所有可能的虚拟机，替换个毛线
            if (sold_servers[j].hardware_cost <= sold_servers[i].hardware_cost) {           //硬件成本低
                if (sold_servers[j].daily_cost <= sold_servers[i].daily_cost) {      //电费也更低
                    if (sold_servers[j].cpu >= sold_servers[i].cpu && sold_servers[j].memory >= sold_servers[i].memory) {   //cpu,memory都更好
                        flag = true;
                        cout << "删除服务器：" << sold_servers[i].server_name << ", cpu : " << sold_servers[i].cpu << ", memory : " << sold_servers[i].memory
                            << " 硬件成本 : " << sold_servers[i].hardware_cost << " 电费 ： " << sold_servers[i].daily_cost << endl;
                        cout << "该服务器的上位替代服务器" << endl;
                        cout << "服务器：" << sold_servers[j].server_name << ", cpu : " << sold_servers[j].cpu << ", memory : " << sold_servers[j].memory
                            << " 硬件成本 : " << sold_servers[j].hardware_cost << " 电费 ： " << sold_servers[j].daily_cost << endl;
                        cout << endl;
                        break;
                    }
                    if ((sold_servers[j].cpu + sold_servers[j].memory) >= sold_servers[i].cpu + sold_servers[i].memory) {  //cpu + memory总量更高
                        if (sold_servers[j].cpu < sold_servers[i].cpu) {
                            if (sold_servers[i].cpu - sold_servers[j].cpu < sold_servers[j].cpu / 5) {
                                flag = true;
                                cout << "删除服务器：" << sold_servers[i].server_name << ", cpu : " << sold_servers[i].cpu << ", memory : " << sold_servers[i].memory
                                    << " 硬件成本 : " << sold_servers[i].hardware_cost << " 电费 ： " << sold_servers[i].daily_cost << endl;
                                cout << "该服务器的上位替代服务器" << endl;
                                cout << "服务器：" << sold_servers[j].server_name << ", cpu : " << sold_servers[j].cpu << ", memory : " << sold_servers[j].memory
                                    << " 硬件成本 : " << sold_servers[j].hardware_cost << " 电费 ： " << sold_servers[j].daily_cost << endl;
                                cout << endl;
                                break;
                            }
                        }
                        if (sold_servers[j].memory < sold_servers[i].memory) {
                            if (sold_servers[i].memory - sold_servers[j].memory < sold_servers[j].memory / 5) {
                                flag = true;
                                cout << "删除服务器：" << sold_servers[i].server_name << ", cpu : " << sold_servers[i].cpu << ", memory : " << sold_servers[i].memory
                                    << " 硬件成本 : " << sold_servers[i].hardware_cost << " 电费 ： " << sold_servers[i].daily_cost << endl;
                                cout << "该服务器的上位替代服务器" << endl;
                                cout << "服务器：" << sold_servers[j].server_name << ", cpu : " << sold_servers[j].cpu << ", memory : " << sold_servers[j].memory
                                    << " 硬件成本 : " << sold_servers[j].hardware_cost << " 电费 ： " << sold_servers[j].daily_cost << endl;
                                cout << endl;
                                break;
                            }
                        }
                    }
                }
            }
        }
        if (!flag) {
            new_sold_servers.emplace_back(sold_servers[i]);
        }
    }
    sold_servers = new_sold_servers;
}
double Statistics::cost_function(vector<vector<double>>& var_x, vector<double>& var_y, vector<double>& coeff) {
    const int row = var_x.size();
    const int col = coeff.size();
    double cost = 0;
    vector<double> dest(row, 0);  //目的矩阵
    for (int i = 0; i < row; ++i) {     //dest = X @ coeff
        for (int j = 0; j < col; ++j) {
            dest[i] += var_x[i][j] * coeff[j];
        }
    }
    for (int i = 0; i < row; ++i) {   //dest -= var_y
        dest[i] -= var_y[i];
    }
    for (int i = 0; i < row; ++i) {
        cost += pow(dest[i], 2);
    }
    return cost;
}
void Statistics::gradient_descent(vector<vector<double>>& var_x, vector<double>& var_y, vector<double>& coeff, double alpha, int iters) {
    const int row = var_x.size();
    const int col = coeff.size();
    while(iters--) {
        vector<vector<double>> var_x_T(3, vector<double>(row, 0));  //x的转置
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < row; ++j) {
                var_x_T[i][j] = var_x[j][i];
            }
        }
        vector<double> dest1(row, 0);  //目的矩阵
        for (int i = 0; i < row; ++i) {     //dest1 = X @ coeff
            for (int j = 0; j < col; ++j) {
                dest1[i] += var_x[i][j] * coeff[j];
            }
        }
        for (int i = 0; i < row; ++i) {   //dest1 -= var_y
            dest1[i] -= var_y[i];
        }
        vector<double> dest2(3, 0);            
        for (int i = 0; i < 3; ++i) {             //dest2 = X.T @ dest1 * alpha / len(X)
            for (int j = 0; j < row; ++j) {
                dest2[i] += var_x_T[i][j] * dest1[j];
            }
            dest2[i] = dest2[i] * alpha / row;
        }
        for (int i = 0; i < 3; ++i) {
            coeff[i] -= dest2[i];
        }
    }
}
vector<double> Statistics::linear_regression(vector<SoldServer>& sold_servers) {
    const int servers_num = sold_servers.size();
    vector<double> coeff(3, 1);       
    vector<vector<double>> var_x(servers_num, vector<double>(3, 0));
    vector<double> var_hardware(servers_num, 0);
    double alpha = 0.0000001;  //学习率；
    int iters = 10000;   //迭代次数
    for (int i = 0; i < servers_num; ++i) {
        var_x[i][0] = sold_servers[i].cpu;
        var_x[i][1] = sold_servers[i].memory;
        var_x[i][2] = 1;
        var_hardware[i] = sold_servers[i].hardware_cost;
    }
    gradient_descent(var_x, var_hardware, coeff, alpha, iters);
    double sum = coeff[0] + coeff[1];
    coeff[0] = coeff[0] / sum;
    coeff[1] = coeff[1] / sum;
    return coeff;
}

#endif