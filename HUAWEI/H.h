#include <iostream>
#include <algorithm>
#include <string>
#include <vector>
#include <unordered_map>
#include <ctime>
#include <fstream>
#include <unordered_set>
using namespace std;

//#define TEST_PARSEINPUT
#define REDIRECT

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
    int server_id;
    int total_core_num;
    int total_memory_size;
    int A_remain_core_num;
    int A_remain_memory_size;
    int B_remain_core_num;
    int B_remain_memory_size;
    unordered_set<int> a_vm_id;
    unordered_set<int> b_vm_id;
    unordered_set<int> ab_vm_id;
};
struct VmIdInfo {
    PurchasedServer* purchase_server;
    string node;  //A 代表A节点  B代表B节点  C代表C节点
    string vm_name;
    int cpu_cores;
    int memory_size;
}

class Evaluate {
public:

};
class CMP {
public:
    static bool Continuous () {
        
    }
};