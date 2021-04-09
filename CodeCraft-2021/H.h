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
#include<set>
#include <climits>
#include <cfloat>
// #include <omp.h>

using namespace std;

// #define TEST_PARSEINPUT
// #define REDIRECT
// #define PRINTINFO
// #define MULTIPROCESS

struct SoldServer {
    string server_name;
    int cpu_cores;
    int memory_size;
    int hardware_cost;
    int daily_cost;
};
struct SoldVm {
    //string VM_name;
    int cpu_cores;
    int memory_size;
    int deployment_way;
};
struct RequestData {
    string operation;
    string vm_name;
    int vm_id;
};
struct PurchasedServer {
    string server_name;
    int server_id = -1;
    int total_core_num;
    int total_memory_size;
    int daily_cost;
    int A_remain_core_num;
    int A_remain_memory_size;
    int B_remain_core_num;
    int B_remain_memory_size;
    bool can_deploy_A = false;
    bool can_deploy_B = false;
    unordered_set<int> A_vm_id;
    unordered_set<int> B_vm_id;
    unordered_set<int> AB_vm_id;
};
struct VmIdInfo {
    PurchasedServer* purchase_server;
    char node;  //A 代表A节点  B代表B节点  C代表C节点
    int vm_id;
    string vm_name;
    int cpu_cores;
    int memory_size;
};
struct AddData {
    AddData() = default;
    AddData(int _deployment_way, int _cpu_cores, int _memory_size, int _vm_id, string _vm_name) :
            deployment_way(_deployment_way), cpu_cores(_cpu_cores), memory_size(_memory_size), vm_id(_vm_id), vm_name(_vm_name) {}
    int deployment_way;
    int cpu_cores;
    int memory_size;
    int vm_id;
    string vm_name;
};
struct DelData {
    DelData() = default;
    DelData(int _vm_id) :
            vm_id(_vm_id) {}
    int vm_id;
};

struct MigrationInfo {
    int vm_id;
    PurchasedServer *server;
    char node;
    MigrationInfo() {}
    MigrationInfo(int _vm_id, PurchasedServer *_server, char _node) : vm_id(_vm_id), server(_server), node(_node) {}
};

class Evaluate {
    private:
    const double threshold1 = 4;

    const double threshold_abs1 = 10;
    const double threshold_abs2 = 3;
    bool check1(double ratio_a, double ratio_b) {
        if (ratio_a >= 1) {
            if (fabs(ratio_a - ratio_b) > threshold_abs1) {
                return false;
            }
        } else {
            if (fabs(1 / ratio_a - 1 / ratio_b) > threshold_abs1) {
                return false;
            }
        }
        return true;
    }
    bool check2(double ratio_a, double ratio_b) {
        if (ratio_a >= 1) {
            if (fabs(ratio_a - ratio_b) > threshold_abs1) {
                return false;
            }
        } else {
            if (fabs(1 / ratio_a - 1 / ratio_b) > threshold_abs1) {
                return false;
            }
        }
        return true;
    }
public:
    bool StrongPurchasedServerAB(PurchasedServer* purchased_server, int cpu_cores, int memory_size) {  //评价要不要插到双节点
        if (purchased_server->AB_vm_id.size() == 0) {
            return false;
        }
        return true;
    }
    bool WeakPurchasedServerAB(PurchasedServer* purchased_server, int cpu_cores, int memory_size) {  //评价要不要插到双节点
        
        return true;
    }
    bool PurchasedServerA(PurchasedServer* purchased_server, int cpu_cores, int memory_size) {  //评价要不要插到A节点
        if (purchased_server->A_remain_core_num >= cpu_cores && purchased_server->A_remain_memory_size >= memory_size) {
            /*double ratio = 1.0 * purchased_server->A_remain_core_num / purchased_server->A_remain_memory_size;
            if (purchased_server)
            if (ratio > 1 && ratio < 10) {
                if (1.0 * (purchased_server->A_remain_core_num - cpu_cores) / (purchased_server->A_remain_memory_size - memory_size) > 20) {
                    return false;
                }
                if (1.0 * (purchased_server->A_remain_core_num - cpu_cores) / (purchased_server->A_remain_memory_size - memory_size) < 1 / 20) {
                    return false;
                }
            }
            if (ratio <= 1 && 1.0 / ratio < 10) {
                if (1.0 / (purchased_server->A_remain_core_num - cpu_cores) * (purchased_server->A_remain_memory_size - memory_size) > 20) {
                    return false;
                }
                if (1.0 * (purchased_server->A_remain_core_num - cpu_cores) / (purchased_server->A_remain_memory_size - memory_size) < 1 /20) {
                    return false;
                }
            }*/
            return true;
        }
        return false;
    }
    bool PurchasedServerB(PurchasedServer* purchased_server, int cpu_cores, int memory_size) {  //评价要不要插到B节点
        if (purchased_server->B_remain_core_num >= cpu_cores && purchased_server->B_remain_memory_size >= memory_size) {
            /*double ratio = 1.0 * purchased_server->B_remain_core_num / purchased_server->B_remain_memory_size;
            if (ratio > 1 && ratio < 10) {
                if (1.0 * (purchased_server->A_remain_core_num - cpu_cores) / (purchased_server->A_remain_memory_size - memory_size) > 20) {
                    return false;
                }
            }
            if (ratio <= 1 && 1.0 / ratio < 10) {
                if (1.0 / (purchased_server->A_remain_core_num - cpu_cores) * (purchased_server->A_remain_memory_size - memory_size) > 20) {
                    return false;
                }
            }*/
            return true;
        }
        return false;
    }
public:
    static bool CanMigrateTo(VmIdInfo *vm_info, PurchasedServer *original_server, PurchasedServer *target_server, char target_node) {
        //if (target_server->A_vm_id.size() + target_server->B_vm_id.size() + target_server->AB_vm_id.size() <= 2) return false;
        /*if (original_server == target_server) {
            if (vm_id2info[vm_id].node == target_node) return false;
            
        }*/
        int cpu_cores = vm_info->cpu_cores, memory_size = vm_info->memory_size;
        int A_remain_core_num = target_server->A_remain_core_num, B_remain_core_num = target_server->B_remain_core_num;
        int A_remain_memory_size = target_server->A_remain_memory_size, B_remain_memory_size = target_server->B_remain_memory_size;
        bool fit_node_A = cpu_cores <= A_remain_core_num && memory_size <= A_remain_memory_size;
        bool fit_node_B = cpu_cores <= B_remain_core_num && memory_size <= B_remain_memory_size;
        if (target_node == 'C') {
            return fit_node_A && fit_node_B;
        } else if (target_node == 'A') {
            if (!fit_node_A) return false;
            else if (!fit_node_B) return true;
            else return A_remain_core_num + A_remain_memory_size <= B_remain_core_num + B_remain_memory_size;
        } else {
            if (!fit_node_B) return false;
            else if (!fit_node_A) return true;
            else return A_remain_core_num + A_remain_memory_size >= B_remain_core_num + B_remain_memory_size;
        }
    }
};
class Cmp {
public:
    static bool SoldServers (SoldServer& a, SoldServer& b) {
        return a.hardware_cost < b.hardware_cost;
    }
    /*static bool SoldServers (SoldServer& a, SoldServer& b) {
        double x = a.hardware_cost
        return a.hardware_cost < b.hardware_cost;
    }*/
    static bool ContinuousADD (AddData& a, AddData& b) {
        /*if (a.deployment_way != b.deployment_way) {
            return a.deployment_way > b.deployment_way;
        } else {
            return (a.cpu_cores + a.memory_size) > (b.cpu_cores + b.memory_size);
        }*/
        // if ((a.cpu_cores + a.memory_size) * (a.deployment_way==1?1 : 1) > (b.cpu_cores + b.memory_size) * (b.deployment_way == 1?1:  1)) return true;
        // else if((a.cpu_cores + a.memory_size) * (a.deployment_way==1?1 : 1)  ==  (b.cpu_cores + b.memory_size) * (b.deployment_way == 1?1:  1)){
        //     return fabs(log(1.0 * a.cpu_cores / a.memory_size)) < fabs(log(1.0 * b.cpu_cores / b.memory_size));
        // }else{
        //     return false;
        // }
        return (a.cpu_cores + a.memory_size) * (a.deployment_way==1?1 : 1) > (b.cpu_cores + b.memory_size) * (b.deployment_way == 1?1:  1);
        // return (a.cpu_cores + a.memory_size) * (a.deployment_way + 1) > (b.cpu_cores + b.memory_size) * (b.deployment_way + 1);
    }

    static bool CanDeployDouble (PurchasedServer* a, PurchasedServer* b) {
        double surplus_ratio_a = (a->A_remain_core_num + a->A_remain_memory_size + a->B_remain_core_num + a->B_remain_memory_size) * 1.0 / (a->total_core_num + a->total_memory_size) * 2;
        double surplus_ratio_b = (b->A_remain_core_num + b->A_remain_memory_size + b->B_remain_core_num + b->B_remain_memory_size) * 1.0 / (b->total_core_num + b->total_memory_size) * 2;
        return surplus_ratio_a < surplus_ratio_b;
    }
    static bool CanDeploySingle (PurchasedServer* a, PurchasedServer* b) {
        double surplus_ratio_a = (a->A_remain_core_num + a->A_remain_memory_size + a->B_remain_core_num + a->B_remain_memory_size) * 1.0 / (a->total_core_num + a->total_memory_size) * 2;
        double surplus_ratio_b = (b->A_remain_core_num + b->A_remain_memory_size + b->B_remain_core_num + b->B_remain_memory_size) * 1.0 / (b->total_core_num + b->total_memory_size) * 2;
        return surplus_ratio_a < surplus_ratio_b;
    }
};

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
        total_server_cpu += sold_server.cpu_cores;
        total_server_memory += sold_server.memory_size;
    }
    server_average_cpu = total_server_cpu / len;
    server_average_memory = total_server_memory / len;
    sort(sold_servers.begin(), sold_servers.end(),                              
        [](SoldServer& a, SoldServer&b){ return a.cpu_cores < b.cpu_cores;});  //可以不排序改成找第k大(后面有效果再优化)
    server_max_cpu = sold_servers[len - 1].cpu_cores;
    server_min_cpu = sold_servers[0].cpu_cores;
    server_middle_cpu = len & 1 == 1 ? sold_servers[len >> 1].cpu_cores : 1.0 * (sold_servers[len >> 1].cpu_cores + sold_servers[(len >> 1) - 1].cpu_cores ) / 2;

    sort(sold_servers.begin(), sold_servers.end(),                              
        [](SoldServer& a, SoldServer&b){ return a.memory_size < b.memory_size;});  
    server_max_memory = sold_servers[len - 1].memory_size;
    server_min_memory = sold_servers[0].memory_size;
    server_middle_memory = len & 1 == 1 ? sold_servers[len >> 1].memory_size : 1.0 * (sold_servers[len >> 1].memory_size + sold_servers[(len >> 1) - 1].memory_size ) / 2;
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
            total_VM_cpu += sold_VM.cpu_cores * 2;
            total_VM_memory += sold_VM.memory_size * 2;
        } else {
            total_VM_cpu += sold_VM.cpu_cores;
            total_VM_memory += sold_VM.memory_size;
        }
    }
    VM_average_cpu = total_VM_cpu / node_num;
    VM_average_memory = total_VM_memory / node_num;
    sort(sold_VMs.begin(), sold_VMs.end(),                              
        [](SoldVm& a, SoldVm&b){ return a.cpu_cores < b.cpu_cores;});  //可以不排序改成找第k大(后面有效果再优化)
    VM_max_cpu = sold_VMs[len - 1].cpu_cores;
    VM_min_cpu = sold_VMs[0].cpu_cores;
    VM_middle_cpu = len & 1 == 1 ? sold_VMs[len >> 1].cpu_cores : 1.0 * (sold_VMs[len >> 1].cpu_cores + sold_VMs[(len >> 1) - 1].cpu_cores ) / 2;
    kth_small_VM_cpu = k <= len ? sold_VMs[k - 1].cpu_cores : sold_VMs[len - 1].cpu_cores;

    sort(sold_VMs.begin(), sold_VMs.end(),                              
        [](SoldVm& a, SoldVm&b){ return a.memory_size < b.memory_size;});  
    VM_max_memory = sold_VMs[len - 1].memory_size;
    VM_min_memory = sold_VMs[0].memory_size;
    VM_middle_memory = len & 1 == 1 ? sold_VMs[len >> 1].memory_size : 1.0 * (sold_VMs[len >> 1].memory_size + sold_VMs[(len >> 1) - 1].memory_size ) / 2;
    kth_small_VM_memory = k <= len ? sold_VMs[k - 1].memory_size : sold_VMs[len - 1].memory_size;
}
void Statistics::del_bad_server(vector<SoldServer>& sold_servers) {       //去掉特别劣势的服务器
    vector<SoldServer> new_sold_servers;
    int len = sold_servers.size();
    bool flag;  //是否要删除
    for (int i = 0; i < len; ++i) {  
        flag = false;         
        for (int j = 0; j < len; ++j) {   //能否找到一个可以代替sold_servers[i] 的服务器
            if (j == i) continue;     //跳过自身
            if (sold_servers[j].cpu_cores < VM_max_cpu || sold_servers[j].memory_size < VM_max_memory) continue;      //要是不能容下所有可能的虚拟机，替换个毛线
            if (sold_servers[j].hardware_cost <= sold_servers[i].hardware_cost) {           //硬件成本低
                if (sold_servers[j].daily_cost <= sold_servers[i].daily_cost) {      //电费也更低
                    if (sold_servers[j].cpu_cores >= sold_servers[i].cpu_cores && sold_servers[j].memory_size >= sold_servers[i].memory_size) {   //cpu,memory都更好
                        flag = true;
                        cout << "删除服务器：" << sold_servers[i].server_name << ", cpu : " << sold_servers[i].cpu_cores << ", memory : " << sold_servers[i].memory_size
                            << " 硬件成本 : " << sold_servers[i].hardware_cost << " 电费 ： " << sold_servers[i].daily_cost << endl;
                        cout << "该服务器的上位替代服务器" << endl;
                        cout << "服务器：" << sold_servers[j].server_name << ", cpu : " << sold_servers[j].cpu_cores << ", memory : " << sold_servers[j].memory_size
                            << " 硬件成本 : " << sold_servers[j].hardware_cost << " 电费 ： " << sold_servers[j].daily_cost << endl;
                        cout << endl;
                        break;
                    }
                    if ((sold_servers[j].cpu_cores + sold_servers[j].memory_size) >= sold_servers[i].cpu_cores + sold_servers[i].memory_size) {  //cpu + memory总量更高
                        if (sold_servers[j].cpu_cores < sold_servers[i].cpu_cores) {
                            if (sold_servers[i].cpu_cores - sold_servers[j].cpu_cores < sold_servers[j].cpu_cores / 5) {
                                flag = true;
                                cout << "删除服务器：" << sold_servers[i].server_name << ", cpu : " << sold_servers[i].cpu_cores << ", memory : " << sold_servers[i].memory_size
                                    << " 硬件成本 : " << sold_servers[i].hardware_cost << " 电费 ： " << sold_servers[i].daily_cost << endl;
                                cout << "该服务器的上位替代服务器" << endl;
                                cout << "服务器：" << sold_servers[j].server_name << ", cpu : " << sold_servers[j].cpu_cores << ", memory : " << sold_servers[j].memory_size
                                    << " 硬件成本 : " << sold_servers[j].hardware_cost << " 电费 ： " << sold_servers[j].daily_cost << endl;
                                cout << endl;
                                break;
                            }
                        }
                        if (sold_servers[j].memory_size < sold_servers[i].memory_size) {
                            if (sold_servers[i].memory_size - sold_servers[j].memory_size < sold_servers[j].memory_size / 5) {
                                flag = true;
                                cout << "删除服务器：" << sold_servers[i].server_name << ", cpu : " << sold_servers[i].cpu_cores << ", memory : " << sold_servers[i].memory_size
                                    << " 硬件成本 : " << sold_servers[i].hardware_cost << " 电费 ： " << sold_servers[i].daily_cost << endl;
                                cout << "该服务器的上位替代服务器" << endl;
                                cout << "服务器：" << sold_servers[j].server_name << ", cpu : " << sold_servers[j].cpu_cores << ", memory : " << sold_servers[j].memory_size
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
        var_x[i][0] = sold_servers[i].cpu_cores;
        var_x[i][1] = sold_servers[i].memory_size;
        var_x[i][2] = 1;
        var_hardware[i] = sold_servers[i].hardware_cost;
    }
    gradient_descent(var_x, var_hardware, coeff, alpha, iters);
    double sum = coeff[0] + coeff[1];
    coeff[0] = coeff[0] / sum;
    coeff[1] = coeff[1] / sum;
    return coeff;
}
