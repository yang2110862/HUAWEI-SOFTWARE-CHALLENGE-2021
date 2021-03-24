#include <iostream>
#include <algorithm>
#include <string>
#include <vector>
#include <unordered_map>
#include <ctime>
#include <fstream>
using namespace std;

//#define TEST_PARSEINPUT
#define REDIRECT

struct SoldServer {   
    string server_type;
    int cpu_cores;
    int memory_size;
    int hardware_cost;
    int daily_energy_cost;
};
struct SoldVM {
    //string VM_type;
    int cpu_cores;
    int memory_size;
    int deployment_way;
};
struct RequestData {
    string operation;
    string VM_type;
    int VM_ID;
};
struct PurchasedServer {
    string server_name;
    int server_ID;
    int total_core_num;
    int total_memory_size;
    int A_remain_core_num;
    int A_remain_memory_size;
    int B_remain_core_num;
    int B_remain_memory_size;
};

class Evaluate {
public:

};
class CMP {
public:
    static bool Continuous () {
        
    }
};