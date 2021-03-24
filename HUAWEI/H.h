#include <iostream>
#include <algorithm>
#include <string>
#include <vector>
#include <unordered_map>
#include <ctime>
using namespace std;
/*例如(NV603, 92, 324, 53800, 500)表示一种服务
器类型，其型号为 NV603，包含 92 个 CPU 核心， 324G 内存，硬件成本为
53800，每日能耗成本为 500。*/
struct SoldServer {   
    string type;
    int cpu_cores;
    int memory_sizes;
    int hardware_cost;
    int daily_energy_cost;
};
/*例如(s3.small.1, 1, 1, 0)表示一种虚拟机类型，其型号为
s3.small.1，所需 CPU 核数为 1，所需内存为 1G，单节点部署； (c3.8xlarge.2, 32,
64, 1)表示一种虚拟机类型，其型号为 c3.8xlarge.2，所需 CPU 核数为 32，所需内
存为 64G，双节点部署。 */
struct SoldVM {
    string type;
    int cpu_cores;
    int memory_sizes;
    int deployment_way;
};
/*例如(add, c3.large.4, 1)表示创建一台型号为 c3.large.4， ID 为 1 的虚拟机； (del, 1)
表示删除 ID 为 1 的虚拟机。*/
struct RequestData {
    string operation;
    string VM_type;
    string VM_ID;
};