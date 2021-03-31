#include <iostream>
#include <algorithm>
#include <string>
#include <vector>
#include <queue>
#include <unordered_map>
#include <ctime>
#include <fstream>
#include <unordered_set>
#include <cmath>
#include <cfloat>
using namespace std;

//#define TEST_PARSEINPUT
// #define REDIRECT

struct SoldServer {
    string server_name;
    int cpu_cores;
    int memory_size;
    int hardware_cost;
    int daily_energy_cost;
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
        return (a.cpu_cores + a.memory_size) * (a.deployment_way + 1) > (b.cpu_cores + b.memory_size) * (b.deployment_way + 1);
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
class Load {
public:
    static double CalculateDistance(double target, double current) {
        if (target > 1) {
            if (current > 1) {
                return fabs(target - current);
            } else {
                return target - 1 + (1 / current) - 1;
            }
        } else {
            if (current < 1) {
                return fabs (1 / target - 1 / current);
            } else {
                return current - 1 + (1 / target - 1);
            }
        }
    }
    static void CalculateInterval(double& left, double& right, double load_ratio, double threshold) {
        if (load_ratio > 1) {
            left = load_ratio + threshold;
            if (load_ratio - threshold > 1) {
                right = load_ratio - threshold;
            } else {
                right = 1.0 / (threshold - (load_ratio - 1) + 1);
            }
        } else {
            right = 1.0 / (1 / load_ratio + threshold);
            if ((1 / load_ratio - threshold) > 1) {
                left = 1 / (1 / load_ratio - threshold);
            } else {
                left = threshold - (1 / load_ratio - 1) + 1;
            }
        }
    }
};