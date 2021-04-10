#include "H.h"

vector<SoldServer> sold_servers;
unordered_map<string, SoldVm> vm_name2info;
unordered_map<string, SoldServer> server_name2info;
unordered_map<int, VmIdInfo> vm_id2info;
unordered_set<int> vmIDs;

queue<vector<RequestData>> request_datas;
vector<PurchasedServer *> purchase_servers;
unordered_map<string, vector<PurchasedServer *>> purchase_infos;

unordered_set<int> from_off_2_start;

vector<RequestData> intraday_requests;
vector<int> allResourceOfNReqs;

int count_continue_buy = 0;

int count_add_more_del = 0;

int total_days_num, foreseen_days_num; //总天数和可预知的天数
int now_day;                           //现在是第几天
int total_req_num = 0;

int number = 0; //给服务器编号
long long total_server_cost = 0;
long long total_power_cost = 0;
int total_migration_num = 0;

#ifdef PRINTINFO
clock_t _start, _end;
#endif

//状态值
int isDenseBuy = 0; // 0--非密度购买  1--密度购买
double _future_N_reqs_cpu_rate = 0;
double _future_N_reqs_memory_rate = 0;
double _migration_threshold = 0.03; //减小能增加迁移数量。
double _near_full_threshold = 0.02; //增大能去掉更多的服务器，减少时间；同时迁移次数会有轻微减少，成本有轻微增加。
double _near_full_threshold_2 = 0.2;
double k1 = 0.695, k2 = 1 - k1; //CPU和memory的加权系数
double r1 = 0.695, r2 = 1 - r1; //CPU和memory剩余率的加权系数

void init()
{
    sold_servers.erase(sold_servers.begin(), sold_servers.end());
    vm_name2info.erase(vm_name2info.begin(), vm_name2info.end());
    server_name2info.erase(server_name2info.begin(), server_name2info.end());
    vm_id2info.erase(vm_id2info.begin(), vm_id2info.end());
    vmIDs.erase(vmIDs.begin(), vmIDs.end());
    while (!request_datas.empty())
        request_datas.pop();
    purchase_servers.erase(purchase_servers.begin(), purchase_servers.end());
    purchase_infos.erase(purchase_infos.begin(), purchase_infos.end());
    from_off_2_start.erase(from_off_2_start.begin(), from_off_2_start.end());
    intraday_requests.erase(intraday_requests.begin(), intraday_requests.end());
    allResourceOfNReqs.erase(allResourceOfNReqs.begin(), allResourceOfNReqs.end());
    count_continue_buy = 0;

    count_add_more_del = 0;
    //现在是第几天
    total_req_num = 0;

    number = 0; //给服务器编号
    total_server_cost = 0;
    total_power_cost = 0;
    total_migration_num = 0;
    isDenseBuy = 0;
}

// 返回服务器里虚拟机的数量。
inline int vm_nums(PurchasedServer *server)
{
    return server->A_vm_id.size() + server->B_vm_id.size() + server->AB_vm_id.size();
}
// 返回服务器某结点的加权平均剩余率，CPU和memory的权重分别是r1和r2。
inline double remain_rate(PurchasedServer *server, char node)
{
    if (node == 'A')
    {
        return r1 * server->A_remain_core_num / server->total_core_num + r2 * server->A_remain_memory_size / server->total_memory_size;
    }
    else
    {
        return r1 * server->B_remain_core_num / server->total_core_num + r2 * server->B_remain_memory_size / server->total_memory_size;
    }
}

void ParseServerInfo()
{
    int server_num;
    cin >> server_num;
    string server_name, cpu_core, memory_size, hardware_cost, daily_cost;
    for (int i = 0; i < server_num; ++i)
    {
        SoldServer sold_server;
        cin >> server_name >> cpu_core >> memory_size >> hardware_cost >> daily_cost;
        sold_server.server_name = server_name.substr(1, server_name.size() - 2);
        sold_server.cpu_cores = stoi(cpu_core.substr(0, cpu_core.size() - 1)) / 2;
        sold_server.memory_size = stoi(memory_size.substr(0, memory_size.size() - 1)) / 2;
        sold_server.hardware_cost = stoi(hardware_cost.substr(0, hardware_cost.size() - 1));
        sold_server.daily_cost = stoi(daily_cost.substr(0, daily_cost.size() - 1));
        server_name2info[sold_server.server_name] = sold_server;
        sold_servers.emplace_back(sold_server); //可优化，不需要sold_server
    }
}
void ParseVmInfo()
{
    int vm_num;
    cin >> vm_num;
    string vm_name, cpu_core, memory_size, deployment_way;
    for (int i = 0; i < vm_num; ++i)
    {
        SoldVm sold_vm;
        cin >> vm_name >> cpu_core >> memory_size >> deployment_way;
        vm_name = vm_name.substr(1, vm_name.size() - 2);
        sold_vm.cpu_cores = stoi(cpu_core.substr(0, cpu_core.size() - 1));
        sold_vm.memory_size = stoi(memory_size.substr(0, memory_size.size() - 1));
        sold_vm.deployment_way = stoi(deployment_way.substr(0, deployment_way.size() - 1));
        if (sold_vm.deployment_way == 1)
        {
            sold_vm.cpu_cores /= 2;
            sold_vm.memory_size /= 2;
        }
        vm_name2info[vm_name] = sold_vm;
    }
}
void ParseRequest(int days_num)
{
    int operation_num;
    string operation, vm_name, vm_id;
    for (int i = 0; i < days_num; ++i)
    {
        cin >> operation_num;
        vector<RequestData> request_data;
        total_req_num += operation_num;
        for (int j = 0; j < operation_num; ++j)
        {
            cin >> operation;
            operation = operation.substr(1, operation.size() - 2);
            if (operation == "add")
            {
                RequestData data;
                data.operation = "add";
                cin >> vm_name >> vm_id;
                data.vm_name = vm_name.substr(0, vm_name.size() - 1);
                data.vm_id = stoi(vm_id.substr(0, vm_id.size() - 1));
                request_data.emplace_back(data);
            }
            else if (operation == "del")
            {
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
void ParseInput()
{
    ParseServerInfo();
    ParseVmInfo();
    cin >> total_days_num >> foreseen_days_num;
    ParseRequest(foreseen_days_num);
#ifdef TEST_PARSEINPUT
    cout << sold_servers.size() << endl;
    for (auto &&sold_server : sold_servers)
    {
        cout << sold_server.server_name << " " << sold_server.cpu_cores << " " << sold_server.memory_size
             << " " << sold_server.hardware_cost << " " << sold_server.daily_cost << endl;
    }
    cout << vm_name2info.size() << endl;
    for (auto &vm : vm_name2info)
    {
        cout << vm.first << " " << vm.second.cpu_cores << " " << vm.second.memory_size << " "
             << vm.second.deployment_way << endl;
    }
    cout << request_datas.size() << endl;
    for (auto &request : request_datas)
    {
        cout << request.size() << endl;
        for (auto &data : request)
        {
            if (data.operation == "add")
            {
                cout << data.operation << " " << data.vm_name << " " << data.vm_id << endl;
            }
            else if (data.operation == "del")
            {
                cout << data.operation << " " << data.vm_id << endl;
            }
            else
            {
                cerr << "operation error!";
            }
        }
    }
#endif
}
// 筛选出利用率较低的服务器，迁移走其中的虚拟机。
bool NeedMigration(PurchasedServer *server)
{
    if (vm_nums(server) == 0)
        return false;
    double _A_cpu_remain_rate = 1.0 * server->A_remain_core_num / server->total_core_num;
    double _B_cpu_remain_rate = 1.0 * server->B_remain_core_num / server->total_core_num;
    double _A_memory_remain_rate = 1.0 * server->A_remain_memory_size / server->total_memory_size;
    double _B_memory_remain_rate = 1.0 * server->B_remain_memory_size / server->total_memory_size;
    return (_A_cpu_remain_rate > _migration_threshold) + (_A_memory_remain_rate > _migration_threshold) + (_B_cpu_remain_rate > _migration_threshold) + (_B_memory_remain_rate > _migration_threshold) >= 1;
    // return ( (_A_cpu_remain_rate > _migration_threshold && _A_memory_remain_rate > _migration_threshold) + (_B_cpu_remain_rate > _migration_threshold && _B_memory_remain_rate > _migration_threshold) ) >=1;
}

bool NeedMigration_2(PurchasedServer *server)
{
    if (vm_nums(server) == 0)
        return false;
    double _A_cpu_remain_rate = 1.0 * server->A_remain_core_num / server->total_core_num;
    double _B_cpu_remain_rate = 1.0 * server->B_remain_core_num / server->total_core_num;
    double _A_memory_remain_rate = 1.0 * server->A_remain_memory_size / server->total_memory_size;
    double _B_memory_remain_rate = 1.0 * server->B_remain_memory_size / server->total_memory_size;
    return (_A_cpu_remain_rate > _migration_threshold) + (_A_memory_remain_rate > _migration_threshold) + (_B_cpu_remain_rate > _migration_threshold) + (_B_memory_remain_rate > _migration_threshold) >= 1;
}

int _HowManyCondionSuit(PurchasedServer *server)
{
    /**
 * @description: 接NeedMigration，返回服务器满足的条件个数
 * @param {*}
 * @return {个数}
 */
    double _A_cpu_remain_rate = 1.0 * server->A_remain_core_num / server->total_core_num;
    double _B_cpu_remain_rate = 1.0 * server->B_remain_core_num / server->total_core_num;
    double _A_memory_remain_rate = 1.0 * server->A_remain_memory_size / server->total_memory_size;
    double _B_memory_remain_rate = 1.0 * server->B_remain_memory_size / server->total_memory_size;
    return (_A_cpu_remain_rate > _migration_threshold) + (_A_memory_remain_rate > _migration_threshold) + (_B_cpu_remain_rate > _migration_threshold) + (_B_memory_remain_rate > _migration_threshold);
}

// 筛选出几乎满了的服务器，不作为目标服务器。
bool NearlyFull(PurchasedServer *server)
{
    return (1.0 * server->A_remain_core_num / server->total_core_num < _near_full_threshold || 1.0 * server->A_remain_memory_size / server->total_memory_size < _near_full_threshold) && (1.0 * server->B_remain_core_num / server->total_core_num < _near_full_threshold || 1.0 * server->B_remain_memory_size / server->total_memory_size < _near_full_threshold);
}

bool NearlyFull_2(PurchasedServer *server)
{
    return (1.0 * server->A_remain_core_num / server->total_core_num < _near_full_threshold_2 || 1.0 * server->A_remain_memory_size / server->total_memory_size < _near_full_threshold_2) && (1.0 * server->B_remain_core_num / server->total_core_num < _near_full_threshold_2 || 1.0 * server->B_remain_memory_size / server->total_memory_size < _near_full_threshold_2);
}

// 将某台虚拟机迁移至指定位置，调用前请确保能装入。
void migrate_to(VmIdInfo *vm_info, PurchasedServer *target_server, char target_node, vector<MigrationInfo> &migration_infos)
{
    int cpu_cores = vm_info->cpu_cores, memory_size = vm_info->memory_size, vm_id = vm_info->vm_id;
    char original_node = vm_info->node;
    PurchasedServer *original_server = vm_info->purchase_server;
    if (original_node == 'A')
    {
        original_server->A_remain_core_num += cpu_cores;
        original_server->A_remain_memory_size += memory_size;
        original_server->A_vm_id.erase(vm_id);
    }
    else if (original_node == 'B')
    {
        original_server->B_remain_core_num += cpu_cores;
        original_server->B_remain_memory_size += memory_size;
        original_server->B_vm_id.erase(vm_id);
    }
    else
    {
        original_server->A_remain_core_num += cpu_cores;
        original_server->A_remain_memory_size += memory_size;
        original_server->B_remain_core_num += cpu_cores;
        original_server->B_remain_memory_size += memory_size;
        original_server->AB_vm_id.erase(vm_id);
    }

    if (target_node == 'A')
    {
        target_server->A_remain_core_num -= cpu_cores;
        target_server->A_remain_memory_size -= memory_size;
        target_server->A_vm_id.emplace(vm_id);
        vm_info->purchase_server = target_server;
        vm_info->node = 'A';
        migration_infos.emplace_back(vm_id, target_server, 'A');
    }
    else if (target_node == 'B')
    {
        target_server->B_remain_core_num -= cpu_cores;
        target_server->B_remain_memory_size -= memory_size;
        target_server->B_vm_id.emplace(vm_id);
        vm_info->purchase_server = target_server;
        vm_info->node = 'B';
        migration_infos.emplace_back(vm_id, target_server, 'B');
    }
    else
    {
        target_server->A_remain_core_num -= cpu_cores;
        target_server->A_remain_memory_size -= memory_size;
        target_server->B_remain_core_num -= cpu_cores;
        target_server->B_remain_memory_size -= memory_size;
        target_server->AB_vm_id.emplace(vm_id);
        vm_info->purchase_server = target_server;
        vm_info->node = 'C';
        migration_infos.emplace_back(vm_id, target_server, 'C');
    }
}

// 将某服务器的某结点中所有单节点虚拟机迁移至指定位置，或将某服务器中所有双节点虚拟机迁移至指定服务器，调用前请确保能装入。
void migrate_to(PurchasedServer *original_server, char original_node, PurchasedServer *target_server, char target_node, vector<MigrationInfo> &migration_infos)
{
    int sum_cpu = 0, sum_memory = 0;
    unordered_set<int> vm_ids;
    if (original_node == 'A')
    {
        vm_ids = original_server->A_vm_id;
        original_server->A_vm_id.clear();
    }
    else if (original_node == 'B')
    {
        vm_ids = original_server->B_vm_id;
        original_server->B_vm_id.clear();
    }
    else
    {
        vm_ids = original_server->AB_vm_id;
        original_server->AB_vm_id.clear();
    }

    for (int vm_id : vm_ids)
    {
        VmIdInfo *vm_info = &vm_id2info[vm_id];
        sum_cpu += vm_info->cpu_cores;
        sum_memory += vm_info->memory_size;
        vm_info->purchase_server = target_server;
        vm_info->node = target_node;
        migration_infos.emplace_back(vm_id, target_server, target_node);
    }

    if (original_node == 'A' || original_node == 'C')
    {
        original_server->A_remain_core_num += sum_cpu;
        original_server->A_remain_memory_size += sum_memory;
    }
    if (original_node == 'B' || original_node == 'C')
    {
        original_server->B_remain_core_num += sum_cpu;
        original_server->B_remain_memory_size += sum_memory;
    }

    if (target_node == 'A')
    {
        target_server->A_remain_core_num -= sum_cpu;
        target_server->A_remain_memory_size -= sum_memory;
        target_server->A_vm_id.insert(vm_ids.begin(), vm_ids.end());
    }
    else if (target_node == 'B')
    {
        target_server->B_remain_core_num -= sum_cpu;
        target_server->B_remain_memory_size -= sum_memory;
        target_server->B_vm_id.insert(vm_ids.begin(), vm_ids.end());
    }
    else
    {
        target_server->A_remain_core_num -= sum_cpu;
        target_server->A_remain_memory_size -= sum_memory;
        target_server->B_remain_core_num -= sum_cpu;
        target_server->B_remain_memory_size -= sum_memory;
        target_server->AB_vm_id.insert(vm_ids.begin(), vm_ids.end());
    }
}

vector<int> print_req_num(vector<RequestData> &intraday_requests)
{
    int cpu_add = 0;
    int memory_add = 0;
    int cpu_del = 0;
    int memory_del = 0;

    for (auto &req : intraday_requests)
    {
        string operation = req.operation;
        if (operation == "add")
        {
            string vm_name = req.vm_name;
            cpu_add += (vm_name2info[vm_name].cpu_cores) * (vm_name2info[vm_name].deployment_way + 1);
            memory_add += (vm_name2info[vm_name].memory_size) * (vm_name2info[vm_name].deployment_way + 1);
        }
        else if (operation == "del")
        {

            int vm_id = req.vm_id;
            string vm_name = vm_id2info[vm_id].vm_name;
            cpu_del += (vm_name2info[vm_name].cpu_cores) * (vm_name2info[vm_name].deployment_way + 1);
            memory_del += (vm_name2info[vm_name].memory_size) * (vm_name2info[vm_name].deployment_way + 1);
        }
    }
    return {cpu_add, memory_add, cpu_del, memory_del};
    // cout<<add_count<<"   "<<del_count<<endl;
}

int NumOfOffServer()
{
    int cnt = 0;
    for (auto &server : purchase_servers)
    {
        if (vm_nums(server) == 0)
            cnt++;
    }
    return cnt;
}


vector<MigrationInfo> Migration()
{
    int max_migration_num = vmIDs.size() * 30 / 1000;
    // total_migration_num+=max_migration_num;
    vector<MigrationInfo> migration_infos;
    if (max_migration_num == 0)
        return migration_infos;

    // cout<<"1  :"<<NumOfOffServer()<<endl;
    // 第一步迁移。
    {

        vector<VmIdInfo *> migrating_vms;
        vector<PurchasedServer *> target_servers;
        for (auto server : purchase_servers)
        {
            if (NeedMigration(server))
            {
                for (auto &vm_id : server->AB_vm_id)
                    migrating_vms.emplace_back(&vm_id2info[vm_id]);
                for (auto &vm_id : server->A_vm_id)
                    migrating_vms.emplace_back(&vm_id2info[vm_id]);
                for (auto &vm_id : server->B_vm_id)
                    migrating_vms.emplace_back(&vm_id2info[vm_id]);
            }
            if (vm_nums(server) > 0 && !NearlyFull(server))
                target_servers.emplace_back(server);
        }

        sort(migrating_vms.begin(), migrating_vms.end(), [&](VmIdInfo *vm1, VmIdInfo *vm2) {
            PurchasedServer *server1 = vm1->purchase_server, *server2 = vm2->purchase_server;
            if (vm_nums(server1) < vm_nums(server2))
                return true;
            else if (vm_nums(server1) == vm_nums(server2))
            {
                return 1.0 * (vm1->cpu_cores + vm1->memory_size) * (vm1->node == 'C' ? 1 : 1) > 1.0 * (vm2->cpu_cores + vm2->memory_size) * (vm2->node == 'C' ? 1 : 1);
            }
            else
            {
                return false;
            }
        });

        // sort(migrating_vms.begin(), migrating_vms.end(), [&](VmIdInfo *vm1,VmIdInfo *vm2) {
        //     PurchasedServer *server1 = vm1->purchase_server, *server2 = vm2->purchase_server;
        //     if(1.0*(vm1->cpu_cores  + vm1->memory_size ) * (vm1->node == 'C' ? 1 : 1)  > 1.0* (vm2->cpu_cores  + vm2->memory_size ) * (vm2->node == 'C' ? 1 : 1) ) return true;
        //     else if( 1.0*(vm1->cpu_cores  + vm1->memory_size ) * (vm1->node == 'C' ? 1 : 1)  == 1.0* (vm2->cpu_cores  + vm2->memory_size ) * (vm2->node == 'C' ? 1 : 1)){
        //         return vm_nums(server1) > vm_nums(server2);
        //     }else{
        //         return false;
        //     }
        // });

        for (auto &vm_info : migrating_vms)
        {
            if (vm_info->node != 'C')
            {

                PurchasedServer *original_server = vm_info->purchase_server;
                int vm_id = vm_info->vm_id;
                int cpu_cores = vm_info->cpu_cores;
                int memory_size = vm_info->memory_size;
                double _original_rate = remain_rate(original_server, vm_info->node) * original_server->daily_cost;
                double min_rate = _original_rate;
                PurchasedServer *best_server;
                char which_node = '!';
                for (auto &target_server : target_servers)
                { //找最合适的服务器。
                    if (!(target_server == original_server && vm_info->node == 'A') && (target_server->A_remain_core_num >= cpu_cores && target_server->A_remain_memory_size >= memory_size))
                    {
                        double rate = r1 * (target_server->A_remain_core_num - cpu_cores) / target_server->total_core_num * target_server->daily_cost + r2 * (target_server->A_remain_memory_size - memory_size) / target_server->total_memory_size * target_server->daily_cost;

                        if (rate < min_rate)
                        {
                            min_rate = rate;
                            best_server = target_server;
                            which_node = 'A';
                        }
                    }
                    if (!(target_server == original_server && vm_info->node == 'B') && (target_server->B_remain_core_num >= cpu_cores && target_server->B_remain_memory_size >= memory_size))
                    {
                        double rate = r1 * (target_server->B_remain_core_num - cpu_cores) / target_server->total_core_num * target_server->daily_cost + r2 * (target_server->B_remain_memory_size - memory_size) / target_server->total_memory_size * target_server->daily_cost;

                        if (rate < min_rate)
                        {
                            min_rate = rate;
                            best_server = target_server;
                            which_node = 'B';
                        }
                    }
                }
                if (which_node != '!' && min_rate < 0.10 * _original_rate)
                { //开始迁移。
                    migrate_to(vm_info, best_server, which_node, migration_infos);
                    if (migration_infos.size() == max_migration_num)
                        return migration_infos;
                }
            }
            else
            {
                PurchasedServer *original_server = vm_info->purchase_server;
                int vm_id = vm_info->vm_id;
                int cpu_cores = vm_info->cpu_cores;
                int memory_size = vm_info->memory_size;
                double _original_rate = (remain_rate(original_server, 'A') + remain_rate(original_server, 'B')) / 2 * original_server->daily_cost;
                double min_rate = _original_rate;
                PurchasedServer *best_server = NULL;
                for (auto &target_server : target_servers)
                { //找最合适的服务器。
                    if (target_server == original_server)
                        continue;
                    if (target_server->A_remain_core_num >= cpu_cores && target_server->A_remain_memory_size >= memory_size && target_server->B_remain_core_num >= cpu_cores && target_server->B_remain_memory_size >= memory_size)
                    {

                        double rate = r1 * (target_server->A_remain_core_num - cpu_cores + target_server->B_remain_core_num - cpu_cores) / target_server->total_core_num / 2 * target_server->daily_cost + r2 * (target_server->A_remain_memory_size - memory_size + target_server->B_remain_memory_size - memory_size) / target_server->total_memory_size / 2 * target_server->daily_cost;
                        if (rate < min_rate)
                        {
                            min_rate = rate;
                            best_server = target_server;
                        }
                    }
                }
                if (best_server != NULL && min_rate < 0.10 * _original_rate)
                { //开始迁移。
                    migrate_to(vm_info, best_server, 'C', migration_infos);
                    if (migration_infos.size() == max_migration_num)
                        return migration_infos;
                }
            }
        }
    }

    // cout<<"1  end:"<<NumOfOffServer()<<endl;
    // cout<<"2  :"<<NumOfOffServer()<<endl;
    // 第二步迁移。
    {
        vector<PurchasedServer *> merging_servers; //要合并的服务器。
        for (auto server : purchase_servers)
        {
            double rate_a = remain_rate(server, 'A'), rate_b = remain_rate(server, 'B');
            if ((rate_a < _near_full_threshold && rate_b > 1 - _near_full_threshold) || (rate_b < _near_full_threshold && rate_a > 1 - _near_full_threshold))
                merging_servers.emplace_back(server);
        }
        for (auto original_server : merging_servers)
        {
            if (migration_infos.size() > max_migration_num - original_server->A_vm_id.size() && migration_infos.size() > max_migration_num - original_server->B_vm_id.size())
                continue;
            double rate1a = remain_rate(original_server, 'A'), rate1b = remain_rate(original_server, 'B');
            if ((rate1a < _near_full_threshold && rate1b > 1 - _near_full_threshold) || (rate1b < _near_full_threshold && rate1a > 1 - _near_full_threshold))
            {
                int sum_cpu[] = {0, 0, 0}, sum_memory[] = {0, 0, 0}; //分别代表A，B，C结点的已有大小。
                for (int vm_id : original_server->AB_vm_id)
                {
                    VmIdInfo *vm_info = &vm_id2info[vm_id];
                    sum_cpu[2] += vm_info->cpu_cores;
                    sum_memory[2] += vm_info->memory_size;
                }
                sum_cpu[0] = original_server->total_core_num - original_server->A_remain_core_num - sum_cpu[2];
                sum_cpu[1] = original_server->total_core_num - original_server->B_remain_core_num - sum_cpu[2];
                sum_memory[0] = original_server->total_memory_size - original_server->A_remain_memory_size - sum_memory[2];
                sum_memory[1] = original_server->total_memory_size - original_server->B_remain_memory_size - sum_memory[2];
                char original_node = rate1a < _near_full_threshold ? 'A' : 'B';
                double min_diff[] = {DBL_MAX, DBL_MAX};
                PurchasedServer *best_server[2];
                char which_node[2];
                for (int j = 0; j < merging_servers.size(); ++j)
                {
                    PurchasedServer *target_server = merging_servers[j];
                    if (original_server == target_server)
                        continue;
                    double rate2a = remain_rate(target_server, 'A'), rate2b = remain_rate(target_server, 'B');
                    if (rate2a < _near_full_threshold)
                    {
                        if (target_server->B_remain_core_num >= sum_cpu[original_node - 'A'] && target_server->B_remain_memory_size >= sum_memory[original_node - 'A'])
                        {
                            double diff = k1 * (target_server->B_remain_core_num - sum_cpu[original_node - 'A']) + k2 * (target_server->B_remain_memory_size - sum_memory[original_node - 'A']);
                            int thread_num = 0;
                            if (diff < min_diff[thread_num])
                            {
                                min_diff[thread_num] = diff;
                                best_server[thread_num] = target_server;
                                which_node[thread_num] = 'B';
                            }
                        }
                    }
                    else if (rate2b < _near_full_threshold)
                    {
                        if (target_server->A_remain_core_num >= sum_cpu[original_node - 'A'] && target_server->A_remain_memory_size >= sum_memory[original_node - 'A'])
                        {
                            double diff = k1 * (target_server->A_remain_core_num - sum_cpu[original_node - 'A']) + k2 * (target_server->A_remain_memory_size - sum_memory[original_node - 'A']);
                            int thread_num = 0;
                            if (diff < min_diff[thread_num])
                            {
                                min_diff[thread_num] = diff;
                                best_server[thread_num] = target_server;
                                which_node[thread_num] = 'A';
                            }
                        }
                    }
                }
                int index = min_diff[0] <= min_diff[1] ? 0 : 1;
                if (min_diff[index] < DBL_MAX)
                { //开始迁移。
                    if (original_node == 'A')
                    { //迁移A结点。
                        if (migration_infos.size() <= max_migration_num - original_server->A_vm_id.size())
                        {
                            migrate_to(original_server, 'A', best_server[index], which_node[index], migration_infos);
                            if (migration_infos.size() == max_migration_num)
                                return migration_infos;
                        }
                        // 如果能，顺带迁B。
                        bool flag = false; //是否两个结点都能迁，是那么双节点虚拟机也能迁走。
                        // int sum_cpu_b = original_server->total_core_num - original_server->B_remain_core_num;
                        // int sum_memory_b = original_server->total_memory_size - original_server->B_remain_memory_size;
                        if (best_server[index]->A_remain_core_num >= sum_cpu[1] && best_server[index]->A_remain_memory_size >= sum_memory[1] && which_node[index] == 'B')
                        { //B迁A。
                            if (migration_infos.size() <= max_migration_num - original_server->B_vm_id.size())
                            {
                                flag == true;
                                migrate_to(original_server, 'B', best_server[index], 'A', migration_infos);
                                if (migration_infos.size() == max_migration_num)
                                    return migration_infos;
                            }
                        }
                        if (best_server[index]->B_remain_core_num >= sum_cpu[1] && best_server[index]->B_remain_memory_size >= sum_memory[1] && which_node[index] == 'A')
                        { //B迁B。
                            if (migration_infos.size() <= max_migration_num - original_server->B_vm_id.size())
                            {
                                flag == true;
                                migrate_to(original_server, 'B', best_server[index], 'B', migration_infos);
                                if (migration_infos.size() == max_migration_num)
                                    return migration_infos;
                            }
                        }
                        if (flag)
                        { //双节点虚拟机迁移。
                            if (migration_infos.size() <= max_migration_num - original_server->AB_vm_id.size())
                            {
                                migrate_to(original_server, 'C', best_server[index], 'C', migration_infos);
                                if (migration_infos.size() == max_migration_num)
                                    return migration_infos;
                            }
                        }
                    }
                    else
                    {
                        if (migration_infos.size() <= max_migration_num - original_server->B_vm_id.size())
                        {
                            migrate_to(original_server, 'B', best_server[index], which_node[index], migration_infos);
                            if (migration_infos.size() == max_migration_num)
                                return migration_infos;
                        }
                        // 如果能，顺带迁A。
                        bool flag = false; //是否两个结点都能迁，是那么双节点虚拟机也能迁走。
                        // int sum_cpu_a = original_server->total_core_num - original_server->A_remain_core_num;
                        // int sum_memory_a = original_server->total_memory_size - original_server->A_remain_memory_size;
                        if (best_server[index]->A_remain_core_num >= sum_cpu[0] && best_server[index]->A_remain_memory_size >= sum_memory[0] && which_node[index] == 'B')
                        { //A迁A。
                            if (migration_infos.size() <= max_migration_num - original_server->A_vm_id.size())
                            {
                                flag == true;
                                migrate_to(original_server, 'A', best_server[index], 'A', migration_infos);
                                if (migration_infos.size() == max_migration_num)
                                    return migration_infos;
                            }
                        }
                        if (best_server[index]->B_remain_core_num >= sum_cpu[0] && best_server[index]->B_remain_memory_size >= sum_memory[0] && which_node[index] == 'A')
                        { //A迁B。
                            if (migration_infos.size() <= max_migration_num - original_server->A_vm_id.size())
                            {
                                flag == true;
                                migrate_to(original_server, 'A', best_server[index], 'B', migration_infos);
                                if (migration_infos.size() == max_migration_num)
                                    return migration_infos;
                            }
                        }
                        if (flag)
                        { //双节点虚拟机迁移。
                            if (migration_infos.size() <= max_migration_num - original_server->AB_vm_id.size())
                            {
                                migrate_to(original_server, 'C', best_server[index], 'C', migration_infos);
                                if (migration_infos.size() == max_migration_num)
                                    return migration_infos;
                            }
                        }
                    }
                }
            }
        }
        // 服务器内部结构调整。
        // #pragma omp parallel for num_threads(2)
        // vector<PurchasedServer*> needA2B;
        // vector<PurchasedServer*> needB2A;
        // for (int i = 0; i < purchase_servers.size(); ++i) {
        //     PurchasedServer *server = purchase_servers[i];
        //     if(server->A_vm_id.size() + server->B_vm_id.size()<=1) continue;
        //     double balance_rate = log(1.0 * (server->A_remain_core_num + server->A_remain_memory_size) / (server->B_remain_core_num + server->B_remain_memory_size)) ;
        //     if(fabs(balance_rate)  <0){
        //         continue;
        //     }else if(balance_rate < 0){
        //         needA2B.emplace_back(server);
        //     }else if(balance_rate>0){
        //         needB2A.emplace_back(server);
        //     }
        // }
        // // cout<<needA2B.size() <<"  "<<needB2A.size()<<endl;
        // for(auto& server:needA2B){
        //     vector<int> _tempVMID;
        //     for(auto& vmID:server->A_vm_id){
        //         _tempVMID.emplace_back(vmID);
        //     }
        //     sort(_tempVMID.begin(),_tempVMID.end(),[](int vmA,int vmB){
        //         return vm_id2info[vmA].cpu_cores + vm_id2info[vmA].memory_size  < vm_id2info[vmB].cpu_cores + vm_id2info[vmB].memory_size ;
        //     });
        //     double _balanceRateBefore = fabs( log(1.0 * (server->A_remain_core_num + server->A_remain_memory_size) / (server->B_remain_core_num + server->B_remain_memory_size))) ;
        //     for(auto&vmID:_tempVMID){
        //         if(server->B_remain_core_num < vm_id2info[vmID].cpu_cores || server->B_remain_memory_size < vm_id2info[vmID].memory_size) continue;
        //         double _balanceRateAfter = fabs( log(1.0 * (server->A_remain_core_num + vm_id2info[vmID].cpu_cores + server->A_remain_memory_size+ vm_id2info[vmID].memory_size) / (server->B_remain_core_num + server->B_remain_memory_size - vm_id2info[vmID].cpu_cores- vm_id2info[vmID].memory_size)))  ;
        //         if(_balanceRateAfter<_balanceRateBefore){
        //             // cout<<"asdasdasd"<<endl;
        //             _balanceRateBefore = _balanceRateAfter;
        //             migrate_to(&vm_id2info[vmID],server,'B',migration_infos);
        //         }
        //     }
        // }
        // for(auto& server:needB2A){
        //     vector<int> _tempVMID;
        //     for(auto& vmID:server->B_vm_id){
        //         _tempVMID.emplace_back(vmID);
        //     }
        //     sort(_tempVMID.begin(),_tempVMID.end(),[](int vmA,int vmB){
        //         return vm_id2info[vmA].cpu_cores + vm_id2info[vmA].memory_size  < vm_id2info[vmB].cpu_cores + vm_id2info[vmB].memory_size ;
        //     });
        //     double _balanceRateBefore = fabs( log(1.0 * (server->A_remain_core_num + server->A_remain_memory_size) / (server->B_remain_core_num + server->B_remain_memory_size))) ;
        //     for(auto&vmID:_tempVMID){
        //         if(server->A_remain_core_num < vm_id2info[vmID].cpu_cores || server->A_remain_memory_size < vm_id2info[vmID].memory_size) continue;
        //         double _balanceRateAfter = fabs( log(1.0 * (server->A_remain_core_num - vm_id2info[vmID].cpu_cores + server->A_remain_memory_size- vm_id2info[vmID].memory_size) / (server->B_remain_core_num + server->B_remain_memory_size + vm_id2info[vmID].cpu_cores+ vm_id2info[vmID].memory_size)))  ;
        //         if(_balanceRateAfter<_balanceRateBefore){
        //             _balanceRateBefore = _balanceRateAfter;
        //             migrate_to(&vm_id2info[vmID],server,'A',migration_infos);
        //         }
        //     }
        // }
    }

    // 第三步迁移。
    {
        vector<PurchasedServer *> original_servers, target_servers;
        for (auto server : purchase_servers)
        {
            if (NeedMigration_2(server))
                original_servers.emplace_back(server);
            if (!NearlyFull_2(server))
                target_servers.emplace_back(server);
            // if(vm_nums(server) == 0 ) target_servers.emplace_back(server);
        }
        sort(original_servers.begin(), original_servers.end(), [](PurchasedServer *server1, PurchasedServer *server2) {
            if (server1->daily_cost > server2->daily_cost)
                return true;
            else if (server1->daily_cost == server2->daily_cost)
            {
                return min(server1->A_remain_core_num, server1->A_remain_memory_size) < min(server2->A_remain_core_num, server2->A_remain_memory_size);
            }
            else
            {
                return false;
            }
        });
        sort(target_servers.begin(), target_servers.end(), [](PurchasedServer *server1, PurchasedServer *server2) {
            // return (vm_nums(server1)==0? server1->daily_cost:0) < (vm_nums(server2)==0? server2->daily_cost:0);
            if ((vm_nums(server1) == 0 ? server1->daily_cost : 0) < (vm_nums(server2) == 0 ? server2->daily_cost : 0))
            {
                return true;
            }
            else if ((vm_nums(server1) == 0 ? server1->daily_cost : 0) == (vm_nums(server2) == 0 ? server2->daily_cost : 0))
            {
                return max(remain_rate(server1, 'A'), remain_rate(server1, 'B')) < max(remain_rate(server2, 'A'), remain_rate(server2, 'B'));
                // return  server1->total_core_num - server1->A_remain_core_num - server1->B_remain_core_num + 2 * server1->total_memory_size - server1->A_remain_memory_size - server1->B_remain_memory_size < 2 * server2->total_core_num - server2->A_remain_core_num - server2->B_remain_core_num + 2 * server2->total_memory_size - server2->A_remain_memory_size - server2->B_remain_memory_size;
            }
            else
            {
                return false;
            }
        });
        for (auto original_server : original_servers)
        {
            for (auto target_server : target_servers)
            {
                if (target_server != original_server && (vm_nums(target_server) == 0 ? target_server->daily_cost : 0) <= 0.9 * original_server->daily_cost && migration_infos.size() <= max_migration_num - vm_nums(original_server) && target_server->A_remain_core_num >= original_server->total_core_num - original_server->A_remain_core_num && target_server->A_remain_memory_size >= original_server->total_memory_size - original_server->A_remain_memory_size && target_server->B_remain_core_num >= original_server->total_core_num - original_server->B_remain_core_num && target_server->B_remain_memory_size >= original_server->total_memory_size - original_server->B_remain_memory_size)
                {
                    migrate_to(original_server, 'C', target_server, 'C', migration_infos);
                    migrate_to(original_server, 'A', target_server, 'A', migration_infos);
                    migrate_to(original_server, 'B', target_server, 'B', migration_infos);
                    if (migration_infos.size() == max_migration_num)
                        return migration_infos;
                }
                else if (target_server != original_server && (vm_nums(target_server) == 0 ? target_server->daily_cost : 0) <= 0.9 * original_server->daily_cost && migration_infos.size() <= max_migration_num - vm_nums(original_server) && target_server->A_remain_core_num >= original_server->total_core_num - original_server->B_remain_core_num && target_server->A_remain_memory_size >= original_server->total_memory_size - original_server->B_remain_memory_size && target_server->B_remain_core_num >= original_server->total_core_num - original_server->A_remain_core_num && target_server->B_remain_memory_size >= original_server->total_memory_size - original_server->A_remain_memory_size)
                {
                    migrate_to(original_server, 'C', target_server, 'C', migration_infos);
                    migrate_to(original_server, 'A', target_server, 'B', migration_infos);
                    migrate_to(original_server, 'B', target_server, 'A', migration_infos);
                    if (migration_infos.size() == max_migration_num)
                        return migration_infos;
                }
            }
        }
    }




    {

        vector<VmIdInfo *> migrating_vms;
        vector<PurchasedServer *> target_servers;
        for (auto server : purchase_servers)
        {
            if (NeedMigration(server))
            {
                for (auto &vm_id : server->AB_vm_id)
                    migrating_vms.emplace_back(&vm_id2info[vm_id]);
                for (auto &vm_id : server->A_vm_id)
                    migrating_vms.emplace_back(&vm_id2info[vm_id]);
                for (auto &vm_id : server->B_vm_id)
                    migrating_vms.emplace_back(&vm_id2info[vm_id]);
            }
            if (vm_nums(server) > 0 && !NearlyFull(server))
                target_servers.emplace_back(server);
        }

        sort(migrating_vms.begin(), migrating_vms.end(), [&](VmIdInfo *vm1, VmIdInfo *vm2) {
            PurchasedServer *server1 = vm1->purchase_server, *server2 = vm2->purchase_server;
            if (vm_nums(server1) < vm_nums(server2))
                return true;
            else if (vm_nums(server1) == vm_nums(server2))
            {
                return 1.0 * (vm1->cpu_cores + vm1->memory_size) * (vm1->node == 'C' ? 1 : 1) > 1.0 * (vm2->cpu_cores + vm2->memory_size) * (vm2->node == 'C' ? 1 : 1);
            }
            else
            {
                return false;
            }
        });

        // sort(migrating_vms.begin(), migrating_vms.end(), [&](VmIdInfo *vm1,VmIdInfo *vm2) {
        //     PurchasedServer *server1 = vm1->purchase_server, *server2 = vm2->purchase_server;
        //     if(1.0*(vm1->cpu_cores  + vm1->memory_size ) * (vm1->node == 'C' ? 1 : 1)  > 1.0* (vm2->cpu_cores  + vm2->memory_size ) * (vm2->node == 'C' ? 1 : 1) ) return true;
        //     else if( 1.0*(vm1->cpu_cores  + vm1->memory_size ) * (vm1->node == 'C' ? 1 : 1)  == 1.0* (vm2->cpu_cores  + vm2->memory_size ) * (vm2->node == 'C' ? 1 : 1)){
        //         return vm_nums(server1) > vm_nums(server2);
        //     }else{
        //         return false;
        //     }
        // });

        for (auto &vm_info : migrating_vms)
        {
            if (vm_info->node != 'C')
            {

                PurchasedServer *original_server = vm_info->purchase_server;
                int vm_id = vm_info->vm_id;
                int cpu_cores = vm_info->cpu_cores;
                int memory_size = vm_info->memory_size;
                double _original_rate = remain_rate(original_server, vm_info->node) * original_server->daily_cost;
                double min_rate = _original_rate;
                PurchasedServer *best_server;
                char which_node = '!';
                for (auto &target_server : target_servers)
                { //找最合适的服务器。
                    if (!(target_server == original_server && vm_info->node == 'A') && (target_server->A_remain_core_num >= cpu_cores && target_server->A_remain_memory_size >= memory_size))
                    {
                        double rate = r1 * (target_server->A_remain_core_num - cpu_cores) / target_server->total_core_num * target_server->daily_cost + r2 * (target_server->A_remain_memory_size - memory_size) / target_server->total_memory_size * target_server->daily_cost;

                        if (rate < min_rate)
                        {
                            min_rate = rate;
                            best_server = target_server;
                            which_node = 'A';
                        }
                    }
                    if (!(target_server == original_server && vm_info->node == 'B') && (target_server->B_remain_core_num >= cpu_cores && target_server->B_remain_memory_size >= memory_size))
                    {
                        double rate = r1 * (target_server->B_remain_core_num - cpu_cores) / target_server->total_core_num * target_server->daily_cost + r2 * (target_server->B_remain_memory_size - memory_size) / target_server->total_memory_size * target_server->daily_cost;

                        if (rate < min_rate)
                        {
                            min_rate = rate;
                            best_server = target_server;
                            which_node = 'B';
                        }
                    }
                }
                if (which_node != '!' && min_rate < _original_rate)
                { //开始迁移。
                    migrate_to(vm_info, best_server, which_node, migration_infos);
                    if (migration_infos.size() == max_migration_num)
                        return migration_infos;
                }
            }
            else
            {
                PurchasedServer *original_server = vm_info->purchase_server;
                int vm_id = vm_info->vm_id;
                int cpu_cores = vm_info->cpu_cores;
                int memory_size = vm_info->memory_size;
                double _original_rate = (remain_rate(original_server, 'A') + remain_rate(original_server, 'B')) / 2 * original_server->daily_cost;
                double min_rate = _original_rate;
                PurchasedServer *best_server = NULL;
                for (auto &target_server : target_servers)
                { //找最合适的服务器。
                    if (target_server == original_server)
                        continue;
                    if (target_server->A_remain_core_num >= cpu_cores && target_server->A_remain_memory_size >= memory_size && target_server->B_remain_core_num >= cpu_cores && target_server->B_remain_memory_size >= memory_size)
                    {

                        double rate = r1 * (target_server->A_remain_core_num - cpu_cores + target_server->B_remain_core_num - cpu_cores) / target_server->total_core_num / 2 * target_server->daily_cost + r2 * (target_server->A_remain_memory_size - memory_size + target_server->B_remain_memory_size - memory_size) / target_server->total_memory_size / 2 * target_server->daily_cost;
                        if (rate < min_rate)
                        {
                            min_rate = rate;
                            best_server = target_server;
                        }
                    }
                }
                if (best_server != NULL && min_rate <   _original_rate)
                { //开始迁移。
                    migrate_to(vm_info, best_server, 'C', migration_infos);
                    if (migration_infos.size() == max_migration_num)
                        return migration_infos;
                }
            }
        }
    }




    {
        vector<PurchasedServer *> original_servers, target_servers;
        for (auto server : purchase_servers)
        {
            if (NeedMigration_2(server))
                original_servers.emplace_back(server);
            if (!NearlyFull_2(server))
                target_servers.emplace_back(server);
            // if(vm_nums(server) == 0 ) target_servers.emplace_back(server);
        }
        sort(original_servers.begin(), original_servers.end(), [](PurchasedServer *server1, PurchasedServer *server2) {
            if (server1->daily_cost > server2->daily_cost)
                return true;
            else if (server1->daily_cost == server2->daily_cost)
            {
                return min(server1->A_remain_core_num, server1->A_remain_memory_size) < min(server2->A_remain_core_num, server2->A_remain_memory_size);
            }
            else
            {
                return false;
            }
        });
        sort(target_servers.begin(), target_servers.end(), [](PurchasedServer *server1, PurchasedServer *server2) {
            // return (vm_nums(server1)==0? server1->daily_cost:0) < (vm_nums(server2)==0? server2->daily_cost:0);
            if ((vm_nums(server1) == 0 ? server1->daily_cost : 0) < (vm_nums(server2) == 0 ? server2->daily_cost : 0))
            {
                return true;
            }
            else if ((vm_nums(server1) == 0 ? server1->daily_cost : 0) == (vm_nums(server2) == 0 ? server2->daily_cost : 0))
            {
                return max(remain_rate(server1, 'A'), remain_rate(server1, 'B')) < max(remain_rate(server2, 'A'), remain_rate(server2, 'B'));
                // return  server1->total_core_num - server1->A_remain_core_num - server1->B_remain_core_num + 2 * server1->total_memory_size - server1->A_remain_memory_size - server1->B_remain_memory_size < 2 * server2->total_core_num - server2->A_remain_core_num - server2->B_remain_core_num + 2 * server2->total_memory_size - server2->A_remain_memory_size - server2->B_remain_memory_size;
            }
            else
            {
                return false;
            }
        });
        for (auto original_server : original_servers)
        {
            for (auto target_server : target_servers)
            {
                if (target_server != original_server && (vm_nums(target_server) == 0 ? target_server->daily_cost : 0) <= 1.0 * original_server->daily_cost && migration_infos.size() <= max_migration_num - vm_nums(original_server) && target_server->A_remain_core_num >= original_server->total_core_num - original_server->A_remain_core_num && target_server->A_remain_memory_size >= original_server->total_memory_size - original_server->A_remain_memory_size && target_server->B_remain_core_num >= original_server->total_core_num - original_server->B_remain_core_num && target_server->B_remain_memory_size >= original_server->total_memory_size - original_server->B_remain_memory_size)
                {
                    migrate_to(original_server, 'C', target_server, 'C', migration_infos);
                    migrate_to(original_server, 'A', target_server, 'A', migration_infos);
                    migrate_to(original_server, 'B', target_server, 'B', migration_infos);
                    if (migration_infos.size() == max_migration_num)
                        return migration_infos;
                }
                else if (target_server != original_server && (vm_nums(target_server) == 0 ? target_server->daily_cost : 0) <= 1.0 * original_server->daily_cost && migration_infos.size() <= max_migration_num - vm_nums(original_server) && target_server->A_remain_core_num >= original_server->total_core_num - original_server->B_remain_core_num && target_server->A_remain_memory_size >= original_server->total_memory_size - original_server->B_remain_memory_size && target_server->B_remain_core_num >= original_server->total_core_num - original_server->A_remain_core_num && target_server->B_remain_memory_size >= original_server->total_memory_size - original_server->A_remain_memory_size)
                {
                    migrate_to(original_server, 'C', target_server, 'C', migration_infos);
                    migrate_to(original_server, 'A', target_server, 'B', migration_infos);
                    migrate_to(original_server, 'B', target_server, 'A', migration_infos);
                    if (migration_infos.size() == max_migration_num)
                        return migration_infos;
                }
            }
        }
    }

    return migration_infos;
}

struct serverCmp1
{
    bool operator()(const PurchasedServer *a, const PurchasedServer *b) const
    {
        return max(a->A_remain_core_num, a->B_remain_core_num) + max(a->A_remain_memory_size, a->B_remain_memory_size) > max(b->A_remain_core_num, b->B_remain_core_num) + max(b->A_remain_memory_size, b->B_remain_memory_size);
    }
};

PurchasedServer *SearchSuitPurchasedServer(int deployed_way, int cpu_cores, int memory_size, bool from_open)
{
    /**
     * @description: 从已有服务器中搜寻合适服务器
     * @param {deployed_way:部署方式)(1:双节点,0:单节点);from_open: 从开机/关机的服务器中搜索 }
     * @return {合适的服务器PurchasedServer* ，没有则返回nullptr}
     */
    PurchasedServer *flag_server = 0;
    PurchasedServer *balance_server = 0;

    // multiset<PurchasedServer*,serverCmp1> temp_purchase_servers;
    // temp_purchase_servers.insert(purchase_servers.begin(),purchase_servers.end());

    bool use_Balance = true;
    if (deployed_way == 1)
    {
        if (from_open)
        {
            double min_remain_rate = 2.0;
            double min_balance_rate = 2.5;

            for (auto &purchase_server : purchase_servers)
            {
                if (purchase_server->A_remain_core_num >= cpu_cores && purchase_server->A_remain_memory_size >= memory_size && purchase_server->B_remain_core_num >= cpu_cores && purchase_server->B_remain_memory_size >= memory_size && (purchase_server->A_vm_id.size() + purchase_server->B_vm_id.size() + purchase_server->AB_vm_id.size() != 0))
                {
                    double _cpu_remain_rate = max(1.0 * (purchase_server->A_remain_core_num - cpu_cores) / purchase_server->total_core_num, 1.0 * (purchase_server->B_remain_core_num - cpu_cores) / purchase_server->total_core_num);
                    double _memory_remain_rate = max(1.0 * (purchase_server->A_remain_memory_size - memory_size) / purchase_server->total_memory_size, 1.0 * (purchase_server->B_remain_memory_size - memory_size) / purchase_server->total_memory_size);
                    // if(_cpu_remain_rate == 0 && _memory_remain_rate == 0) {
                    //     // cout<<"asdasdasda"<<endl;
                    //     use_Balance = false;
                    //     flag_server = purchase_server;
                    //     break;
                    // }

                    if (use_Balance)
                    {
                        double _cpu_remain = purchase_server->A_remain_core_num - cpu_cores + purchase_server->B_remain_core_num - cpu_cores;
                        double _memory_remain = purchase_server->A_remain_memory_size - memory_size + purchase_server->B_remain_memory_size - memory_size;
                        double _cpu_remain_A = purchase_server->A_remain_core_num - cpu_cores;
                        double _cpu_remain_B = purchase_server->B_remain_core_num - cpu_cores;
                        double _memory_remain_A = purchase_server->A_remain_memory_size - memory_size;
                        double _memory_remain_B = purchase_server->B_remain_memory_size - memory_size;
                        double balance_rate;

                        if (_memory_remain_A == 0)
                        {
                            //防止除0
                            balance_rate = 1000;
                        }
                        else if (_memory_remain_B == 0)
                        {
                            //防止除0
                            balance_rate = 1000;
                        }
                        else
                        {
                            balance_rate = (fabs(log(1.0 * _cpu_remain_A / _memory_remain_A)) + fabs(log(1.0 * _cpu_remain_B / _memory_remain_B)));
                        }
                        if (balance_rate < min_balance_rate)
                        {
                            min_balance_rate = balance_rate;
                            balance_server = purchase_server;
                        }
                    }

                    if (_cpu_remain_rate < 0.10 && _memory_remain_rate < 0.10)
                    {
                        use_Balance = false;
                    }

                    // if (2 * max(_cpu_remain_rate, _memory_remain_rate) < min_remain_rate)
                    if (_cpu_remain_rate + _memory_remain_rate < min_remain_rate)
                    {
                        min_remain_rate = _cpu_remain_rate + _memory_remain_rate;
                        // min_remain_rate = 2 * max(_cpu_remain_rate, _memory_remain_rate);
                        flag_server = purchase_server;
                    }
                }
            }
            if (use_Balance)
            {
                return balance_server;
            }
            else
                return flag_server;
        }
        else
        {
            double min_dense_cost = DBL_MAX;
            for (auto &purchase_server : purchase_servers)
            {
                // if(cpu_cores+memory_size > max(purchase_server->A_remain_core_num,purchase_server->B_remain_core_num) + max(purchase_server->A_remain_memory_size,purchase_server->B_remain_memory_size)){
                //     break;
                // }
                if (purchase_server->A_remain_core_num >= cpu_cores && purchase_server->A_remain_memory_size >= memory_size && purchase_server->B_remain_core_num >= cpu_cores && purchase_server->B_remain_memory_size >= memory_size && (purchase_server->A_vm_id.size() + purchase_server->B_vm_id.size() + purchase_server->AB_vm_id.size() == 0))
                {
                    double dense_cost;
                    if (true)
                    {
                        double _cpu_rate = 1.0 * cpu_cores / purchase_server->A_remain_core_num;
                        double _memory_rate = 1.0 * (memory_size) / purchase_server->A_remain_memory_size;
                        double use_rate = 1.0 * (_cpu_rate + _memory_rate) / 2;
                        // double use_rate = min (_cpu_rate , _memory_rate) ;
                        dense_cost = 1.0 * (server_name2info[purchase_server->server_name].daily_cost) * use_rate;
                    }
                    if (dense_cost < min_dense_cost)
                    {
                        min_dense_cost = dense_cost;
                        flag_server = purchase_server;
                    }
                }
            }
        }
    }

    return flag_server;
}

void DeployOnServer(PurchasedServer *flag_server, int deployment_way, int cpu_cores, int memory_size, int vm_id, string &vm_name)
{
    /**
     * @description: 向某服务器部署虚拟机
     * @param {flag_server: 目标服务器}
     * @return {*}
     */
    if (deployment_way == 1)
    {
        if (flag_server->A_vm_id.size() + flag_server->B_vm_id.size() + flag_server->AB_vm_id.size() == 0)
        {
            from_off_2_start.insert(flag_server->server_id);
        }
        flag_server->A_remain_core_num -= cpu_cores;
        flag_server->A_remain_memory_size -= memory_size;
        flag_server->B_remain_core_num -= cpu_cores;
        flag_server->B_remain_memory_size -= memory_size;
        flag_server->AB_vm_id.insert(vm_id);

        VmIdInfo vm_id_info;
        vm_id_info.purchase_server = flag_server;
        vm_id_info.vm_name = vm_name;
        vm_id_info.cpu_cores = cpu_cores;
        vm_id_info.memory_size = memory_size;
        vm_id_info.node = 'C';
        vm_id_info.vm_id = vm_id;
        vm_id2info[vm_id] = vm_id_info;
    }
}

void BuyAndDeployTwoVM(string vm_1_name, string vm_2_name, int vmID_1, int vmID_2, string serverName)
{
    total_server_cost += server_name2info[serverName].hardware_cost;
    PurchasedServer *purchase_server = new PurchasedServer;
    purchase_server->total_core_num = server_name2info[serverName].cpu_cores;
    purchase_server->total_memory_size = server_name2info[serverName].memory_size;
    purchase_server->A_remain_core_num = server_name2info[serverName].cpu_cores;
    purchase_server->A_remain_memory_size = server_name2info[serverName].memory_size;
    purchase_server->B_remain_core_num = server_name2info[serverName].cpu_cores;
    purchase_server->daily_cost = server_name2info[serverName].daily_cost;
    purchase_server->B_remain_memory_size = server_name2info[serverName].memory_size;
    purchase_server->server_name = server_name2info[serverName].server_name;
    purchase_servers.emplace_back(purchase_server);
    purchase_infos[serverName].emplace_back(purchase_server);

    //待改 from_off_2_start 有问题
    // from_off_2_start.insert(flag_server->server_id);

    purchase_server->A_remain_core_num -= vm_name2info[vm_1_name].cpu_cores;
    purchase_server->B_remain_core_num -= vm_name2info[vm_1_name].cpu_cores;
    purchase_server->A_remain_core_num -= vm_name2info[vm_2_name].cpu_cores;
    purchase_server->B_remain_core_num -= vm_name2info[vm_2_name].cpu_cores;
    purchase_server->A_remain_memory_size -= vm_name2info[vm_1_name].memory_size;
    purchase_server->B_remain_memory_size -= vm_name2info[vm_1_name].memory_size;
    purchase_server->A_remain_memory_size -= vm_name2info[vm_2_name].memory_size;
    purchase_server->B_remain_memory_size -= vm_name2info[vm_2_name].memory_size;
    purchase_server->AB_vm_id.insert(vmID_1);
    purchase_server->AB_vm_id.insert(vmID_2);

    vmIDs.erase(vmID_1);
    vmIDs.erase(vmID_2);

    VmIdInfo vm_id_info;
    vm_id_info.purchase_server = purchase_server;
    vm_id_info.vm_name = vm_1_name;
    vm_id_info.cpu_cores = vm_name2info[vm_1_name].cpu_cores;
    vm_id_info.memory_size = vm_name2info[vm_1_name].memory_size;
    vm_id_info.node = 'C';
    vm_id_info.vm_id = vmID_1;
    vm_id2info[vmID_1] = vm_id_info;
    vmIDs.insert(vmID_1);

    VmIdInfo vm_id_info_2;
    vm_id_info_2.purchase_server = purchase_server;
    vm_id_info_2.vm_name = vm_2_name;
    vm_id_info_2.cpu_cores = vm_name2info[vm_2_name].cpu_cores;
    vm_id_info_2.memory_size = vm_name2info[vm_2_name].memory_size;
    vm_id_info_2.node = 'C';
    vm_id_info_2.vm_id = vmID_2;
    vm_id2info[vmID_2] = vm_id_info_2;
    vmIDs.insert(vmID_2);
}

void BuyAndDeployTwoVM_2(string vm_1_name, string vm_2_name, int vmID_1, int vmID_2, string serverName)
{
    total_server_cost += server_name2info[serverName].hardware_cost;
    PurchasedServer *purchase_server = new PurchasedServer;
    purchase_server->total_core_num = server_name2info[serverName].cpu_cores;
    purchase_server->total_memory_size = server_name2info[serverName].memory_size;
    purchase_server->A_remain_core_num = server_name2info[serverName].cpu_cores;
    purchase_server->A_remain_memory_size = server_name2info[serverName].memory_size;
    purchase_server->B_remain_core_num = server_name2info[serverName].cpu_cores;
    purchase_server->daily_cost = server_name2info[serverName].daily_cost;
    purchase_server->B_remain_memory_size = server_name2info[serverName].memory_size;
    purchase_server->server_name = server_name2info[serverName].server_name;
    purchase_servers.emplace_back(purchase_server);
    purchase_infos[serverName].emplace_back(purchase_server);

    //待改 from_off_2_start 有问题
    // from_off_2_start.insert(flag_server->server_id);

    purchase_server->A_remain_core_num -= vm_name2info[vm_1_name].cpu_cores;
    purchase_server->B_remain_core_num -= vm_name2info[vm_2_name].cpu_cores;
    purchase_server->A_remain_memory_size -= vm_name2info[vm_1_name].memory_size;
    purchase_server->B_remain_memory_size -= vm_name2info[vm_2_name].memory_size;
    purchase_server->A_vm_id.insert(vmID_1);
    purchase_server->B_vm_id.insert(vmID_2);

    // vm_id2info.erase(vmID_1);
    // vm_id2info.erase(vmID_2);
    vmIDs.erase(vmID_1);
    vmIDs.erase(vmID_2);

    VmIdInfo vm_id_info;
    vm_id_info.purchase_server = purchase_server;
    vm_id_info.vm_name = vm_1_name;
    vm_id_info.cpu_cores = vm_name2info[vm_1_name].cpu_cores;
    vm_id_info.memory_size = vm_name2info[vm_1_name].memory_size;
    vm_id_info.node = 'A';
    vm_id_info.vm_id = vmID_1;
    vm_id2info[vmID_1] = vm_id_info;
    vmIDs.insert(vmID_1);

    VmIdInfo vm_id_info_2;
    vm_id_info_2.purchase_server = purchase_server;
    vm_id_info_2.vm_name = vm_2_name;
    vm_id_info_2.cpu_cores = vm_name2info[vm_2_name].cpu_cores;
    vm_id_info_2.memory_size = vm_name2info[vm_2_name].memory_size;
    vm_id_info_2.node = 'B';
    vm_id_info_2.vm_id = vmID_2;
    vm_id2info[vmID_2] = vm_id_info_2;
    vmIDs.insert(vmID_2);
}

PurchasedServer *BuyNewServer(int deployment_way, int cpu_cores, int memory_size)
{
    /**
     * @description: 购买新服务器并加入已购序列中
     * @param {*}
     * @return {刚刚购买的服务器PurchasedServer*}
     */
    SoldServer *flag_sold_server;
    double min_dense_cost = 99999999999999;

    if (deployment_way == 1)
    {
        for (auto &sold_server : sold_servers)
        {
            if (sold_server.cpu_cores >= cpu_cores && sold_server.memory_size >= memory_size)
            {
                double dense_cost;
                if (isDenseBuy == 1)
                {
                    // double use_rate = (1.0 *(cpu_cores) / sold_server.cpu_cores + 1.0 *(memory_size) / sold_server.memory_size) / 2;
                    // double use_rate = max(1.0 *(cpu_cores) / sold_server.cpu_cores , 1.0 *(memory_size) / sold_server.memory_size) ;
                    double _cpu_rate = 1.0 * cpu_cores / sold_server.cpu_cores;
                    double _memory_rate = 1.0 * (memory_size) / sold_server.memory_size;
                    // double use_rate = 1.0 * (_cpu_rate + _memory_rate) / 2;
                    double use_rate = _cpu_rate * r1 + _memory_rate * r2;
                    // dense_cost = 1.0 * sold_server.hardware_cost * use_rate;
                    // dense_cost = 1.0 * (sold_server.hardware_cost) + 1.0* sold_server.daily_cost * (total_days_num - now_day) * use_rate;
                    dense_cost = 1.0 * (sold_server.hardware_cost + sold_server.daily_cost * (total_days_num - now_day)) * use_rate;
                }
                else
                {
                    // dense_cost = sold_server.hardware_cost;
                    dense_cost = sold_server.hardware_cost + sold_server.daily_cost * (total_days_num - now_day);
                }
                if (dense_cost < min_dense_cost)
                {
                    min_dense_cost = dense_cost;
                    flag_sold_server = &sold_server;
                }
            }
        }
    }

    total_server_cost += flag_sold_server->hardware_cost;
    PurchasedServer *purchase_server = new PurchasedServer;
    purchase_server->total_core_num = flag_sold_server->cpu_cores;
    purchase_server->total_memory_size = flag_sold_server->memory_size;
    purchase_server->A_remain_core_num = flag_sold_server->cpu_cores;
    purchase_server->A_remain_memory_size = flag_sold_server->memory_size;
    purchase_server->B_remain_core_num = flag_sold_server->cpu_cores;
    purchase_server->daily_cost = flag_sold_server->daily_cost;
    purchase_server->B_remain_memory_size = flag_sold_server->memory_size;
    purchase_server->server_name = flag_sold_server->server_name;
    purchase_servers.emplace_back(purchase_server);
    purchase_infos[flag_sold_server->server_name].emplace_back(purchase_server);
    return purchase_server;
}

string AddVm(AddData &add_data)
{
    Cmp cmp;
    bool deployed = false;
    Evaluate evaluate; //创建一个评价类的实例
    int deployment_way = add_data.deployment_way;
    vmIDs.insert(add_data.vm_id);

    if (deployment_way == 1)
    { //双节点部署
        int cpu_cores = add_data.cpu_cores;
        int memory_size = add_data.memory_size;

        PurchasedServer *flag_server = 0;
        flag_server = SearchSuitPurchasedServer(1, cpu_cores, memory_size, true);

        if (flag_server != 0)
        {
            //从开机的服务器中选到了服务器
            deployed = true;
            DeployOnServer(flag_server, 1, cpu_cores, memory_size, add_data.vm_id, add_data.vm_name);
        }

        PurchasedServer *flag_off_server = 0;
        if (!deployed)
        {
            flag_off_server = SearchSuitPurchasedServer(1, cpu_cores, memory_size, false);
        }

        if (!deployed && flag_off_server != nullptr)
        {
            //从关机的机器里找
            deployed = true;
            DeployOnServer(flag_off_server, 1, cpu_cores, memory_size, add_data.vm_id, add_data.vm_name);
        }

        //购买新服务器
        PurchasedServer *newServer = 0;
        if (!deployed)
        {
            newServer = BuyNewServer(1, cpu_cores, memory_size);
            DeployOnServer(newServer, 1, cpu_cores, memory_size, add_data.vm_id, add_data.vm_name);
            deployed = true;
            return newServer->server_name;
        }
    }
    else
    { //单节点部署
        int cpu_cores = add_data.cpu_cores;
        int memory_size = add_data.memory_size;
        vector<PurchasedServer *> can_deploy_servers;

        //选出开机的合适的服务器
        for (auto &purchase_server : purchase_servers)
        { //先筛选能用的开机服务器
            if ((purchase_server->A_remain_core_num < cpu_cores || purchase_server->A_remain_memory_size < memory_size) && (purchase_server->B_remain_core_num < cpu_cores || purchase_server->B_remain_memory_size < memory_size) || (vm_nums(purchase_server) == 0))
            {
                continue;
            }
            else
            {
                can_deploy_servers.emplace_back(purchase_server);
                if (purchase_server->A_remain_core_num >= cpu_cores && purchase_server->A_remain_memory_size >= memory_size)
                {
                    purchase_server->can_deploy_A = true;
                }
                if (purchase_server->B_remain_core_num >= cpu_cores && purchase_server->B_remain_memory_size >= memory_size)
                {
                    purchase_server->can_deploy_B = true;
                }
            }
        }

        //sort(can_deploy_servers.begin(), can_deploy_servers.end(), cmp.CanDeploySingle);
        double min_remain_rate = 2.0;
        PurchasedServer *flag_server;
        char which_node = 'C';
        for (auto &purchase_server : can_deploy_servers)
        {
            if (deployed)
            {
                break;
            }
            bool evaluate_A = evaluate.PurchasedServerA(purchase_server, cpu_cores, memory_size);
            bool evaluate_B = evaluate.PurchasedServerB(purchase_server, cpu_cores, memory_size);

            if (evaluate_A && evaluate_B)
            {
                double _cpu_remain_rate = 1.0 * (purchase_server->A_remain_core_num - cpu_cores) / purchase_server->total_core_num;
                double _memory_remain_rate = 1.0 * (purchase_server->A_remain_memory_size - memory_size) / purchase_server->total_memory_size;

                if (2 * max(_cpu_remain_rate, _memory_remain_rate) < min_remain_rate)
                // if ((_cpu_remain_rate + _memory_remain_rate) < min_remain_rate)
                {
                    // min_remain_rate = _cpu_remain_rate + _memory_remain_rate;
                    min_remain_rate = 2 * max(_cpu_remain_rate, _memory_remain_rate) ;
                    flag_server = purchase_server;
                    which_node = 'A';
                }

                _cpu_remain_rate = 1.0 * (purchase_server->B_remain_core_num - cpu_cores) / purchase_server->total_core_num;
                _memory_remain_rate = 1.0 * (purchase_server->B_remain_memory_size - memory_size) / purchase_server->total_memory_size;
                if (2 * max(_cpu_remain_rate, _memory_remain_rate) < min_remain_rate)
                // if ((_cpu_remain_rate + _memory_remain_rate) < min_remain_rate)
                {
                    // min_remain_rate = _cpu_remain_rate + _memory_remain_rate;
                    min_remain_rate = 2 * max(_cpu_remain_rate, _memory_remain_rate) ;
                    flag_server = purchase_server;
                    which_node = 'B';
                }
            }
            else if (evaluate_A)
            {
                double _cpu_remain_rate = 1.0 * (purchase_server->A_remain_core_num - cpu_cores) / purchase_server->total_core_num;
                double _memory_remain_rate = 1.0 * (purchase_server->A_remain_memory_size - memory_size) / purchase_server->total_memory_size;
                if (2 * max(_cpu_remain_rate, _memory_remain_rate) < min_remain_rate)
                // if ((_cpu_remain_rate + _memory_remain_rate) < min_remain_rate)
                {
                    // min_remain_rate = _cpu_remain_rate + _memory_remain_rate;
                    min_remain_rate = 2 * max(_cpu_remain_rate, _memory_remain_rate) ;
                    flag_server = purchase_server;
                    which_node = 'A';
                }
            }
            else if (evaluate_B)
            {
                double _cpu_remain_rate = 1.0 * (purchase_server->B_remain_core_num - cpu_cores) / purchase_server->total_core_num;
                double _memory_remain_rate = 1.0 * (purchase_server->B_remain_memory_size - memory_size) / purchase_server->total_memory_size;
                if (2 * max(_cpu_remain_rate, _memory_remain_rate) < min_remain_rate)
                // if ((_cpu_remain_rate + _memory_remain_rate) < min_remain_rate)
                {
                    // min_remain_rate = _cpu_remain_rate + _memory_remain_rate;
                    min_remain_rate = 2 * max(_cpu_remain_rate, _memory_remain_rate) ;
                    flag_server = purchase_server;
                    which_node = 'B';
                }
            }
        }

        if (min_remain_rate != 2.0)
        {
            if (flag_server->A_vm_id.size() + flag_server->B_vm_id.size() + flag_server->AB_vm_id.size() == 0)
            {
                from_off_2_start.insert(flag_server->server_id);
            }
            if (which_node == 'A')
            {
                deployed = true;
                flag_server->A_remain_core_num -= cpu_cores;
                flag_server->A_remain_memory_size -= memory_size;
                flag_server->A_vm_id.insert(add_data.vm_id);

                VmIdInfo vm_id_info;
                vm_id_info.purchase_server = flag_server;
                vm_id_info.vm_name = add_data.vm_name;
                vm_id_info.cpu_cores = cpu_cores;
                vm_id_info.memory_size = memory_size;
                vm_id_info.node = 'A';
                vm_id_info.vm_id = add_data.vm_id;
                vm_id2info[add_data.vm_id] = vm_id_info;
            }
            else if (which_node == 'B')
            {
                deployed = true;
                flag_server->B_remain_core_num -= cpu_cores;
                flag_server->B_remain_memory_size -= memory_size;
                flag_server->B_vm_id.insert(add_data.vm_id);

                VmIdInfo vm_id_info;
                vm_id_info.purchase_server = flag_server;
                vm_id_info.vm_name = add_data.vm_name;
                vm_id_info.cpu_cores = cpu_cores;
                vm_id_info.memory_size = memory_size;
                vm_id_info.node = 'B';
                vm_id_info.vm_id = add_data.vm_id;
                vm_id2info[add_data.vm_id] = vm_id_info;
            }
        }
        else
        {
            can_deploy_servers.erase(can_deploy_servers.begin(), can_deploy_servers.end());
            for (auto &purchase_server : purchase_servers)
            { //先筛选能用的服务器
                if ((purchase_server->A_remain_core_num < cpu_cores || purchase_server->A_remain_memory_size < memory_size) && (purchase_server->B_remain_core_num < cpu_cores || purchase_server->B_remain_memory_size < memory_size) || (vm_nums(purchase_server) != 0))
                {
                    continue;
                }
                else
                {
                    can_deploy_servers.emplace_back(purchase_server);
                    if (purchase_server->A_remain_core_num >= cpu_cores && purchase_server->A_remain_memory_size >= memory_size)
                    {
                        purchase_server->can_deploy_A = true;
                    }
                    if (purchase_server->B_remain_core_num >= cpu_cores && purchase_server->B_remain_memory_size >= memory_size)
                    {
                        purchase_server->can_deploy_B = true;
                    }
                }
            }
            // sort(can_deploy_servers.begin(), can_deploy_servers.end(), cmp.CanDeploySingle);

            min_remain_rate = 2.0;
            double min_cost = DBL_MAX;
            flag_server = 0;
            which_node = 'C';
            for (auto &purchase_server : can_deploy_servers)
            {
                if (deployed)
                {
                    break;
                }
                double _cpu_rate = 1.0 * (cpu_cores / purchase_server->total_core_num);
                double _memory_rate = 1.0 * (memory_size / purchase_server->total_memory_size);
                double use_rate = (_cpu_rate + _memory_rate) / 2;
                double cost = purchase_server->daily_cost * use_rate;
                if (cost < min_cost)
                {
                    min_cost = cost;
                    flag_server = purchase_server;
                    which_node = 'A';
                }
            }

            if (min_cost != DBL_MAX)
            {
                //代表从关机的服务器中选取的
                if (flag_server->A_vm_id.size() + flag_server->B_vm_id.size() + flag_server->AB_vm_id.size() == 0)
                {
                    from_off_2_start.insert(flag_server->server_id);
                }
                deployed = true;
                flag_server->A_remain_core_num -= cpu_cores;
                flag_server->A_remain_memory_size -= memory_size;
                flag_server->A_vm_id.insert(add_data.vm_id);

                VmIdInfo vm_id_info;
                vm_id_info.purchase_server = flag_server;
                vm_id_info.vm_name = add_data.vm_name;
                vm_id_info.cpu_cores = cpu_cores;
                vm_id_info.memory_size = memory_size;
                vm_id_info.node = 'A';
                vm_id_info.vm_id = add_data.vm_id;
                vm_id2info[add_data.vm_id] = vm_id_info;
            }
        }

        double min_dense_cost = DBL_MAX;
        SoldServer *flag_sold_server;
        for (auto &sold_server : sold_servers)
        {
            if (deployed)
            {
                break;
            }
            if (sold_server.cpu_cores >= cpu_cores && sold_server.memory_size >= memory_size)
            {
                double dense_cost;
                if (true)
                {
                    double _cpu_rate = 1.0 * cpu_cores / sold_server.cpu_cores;
                    double _memory_rate = 1.0 * (memory_size) / sold_server.memory_size;
                    double use_rate = 0.5 * (_cpu_rate + _memory_rate);
                    dense_cost = 1.0 * (sold_server.hardware_cost + sold_server.daily_cost * (total_days_num - now_day)) * use_rate;
                }
                else
                {
                    dense_cost = 1.0 * (sold_server.hardware_cost + sold_server.daily_cost * (total_days_num - now_day));
                }
                if (dense_cost < min_dense_cost)
                {
                    min_dense_cost = dense_cost;
                    flag_sold_server = &sold_server;
                }
            }
        }

        if (!deployed)
        {
            //购买一台新服务器
            total_server_cost += flag_sold_server->hardware_cost;
            deployed = true;
            PurchasedServer *purchase_server = new PurchasedServer;
            purchase_server->total_core_num = flag_sold_server->cpu_cores;
            purchase_server->total_memory_size = flag_sold_server->memory_size;
            purchase_server->A_remain_core_num = flag_sold_server->cpu_cores - cpu_cores;
            purchase_server->A_remain_memory_size = flag_sold_server->memory_size - memory_size;
            purchase_server->B_remain_core_num = flag_sold_server->cpu_cores;
            purchase_server->daily_cost = flag_sold_server->daily_cost;
            purchase_server->B_remain_memory_size = flag_sold_server->memory_size;

            purchase_server->A_vm_id.insert(add_data.vm_id);
            purchase_server->server_name = flag_sold_server->server_name;
            purchase_servers.emplace_back(purchase_server);
            purchase_infos[flag_sold_server->server_name].emplace_back(purchase_server);

            VmIdInfo vm_id_info;
            vm_id_info.purchase_server = purchase_server;
            vm_id_info.vm_name = add_data.vm_name;
            vm_id_info.cpu_cores = cpu_cores;
            vm_id_info.memory_size = memory_size;
            vm_id_info.node = 'A';
            vm_id_info.vm_id = add_data.vm_id;
            vm_id2info[add_data.vm_id] = vm_id_info;
            return purchase_server->server_name;
        }
    }
    return "";
}

void DeleteVm(int vm_id)
{
    VmIdInfo vm_info = vm_id2info[vm_id];
    PurchasedServer *purchase_server = vm_info.purchase_server;
    char node = vm_info.node;
    int cpu_cores = vm_info.cpu_cores;
    int memory_size = vm_info.memory_size;
    if (node == 'A')
    {
        purchase_server->A_remain_core_num += cpu_cores;
        purchase_server->A_remain_memory_size += memory_size;
        purchase_server->A_vm_id.erase(vm_id);
    }
    else if (node == 'B')
    {
        purchase_server->B_remain_core_num += cpu_cores;
        purchase_server->B_remain_memory_size += memory_size;
        purchase_server->B_vm_id.erase(vm_id);
    }
    else if (node == 'C')
    {
        purchase_server->A_remain_core_num += cpu_cores;
        purchase_server->A_remain_memory_size += memory_size;
        purchase_server->B_remain_core_num += cpu_cores;
        purchase_server->B_remain_memory_size += memory_size;
        purchase_server->AB_vm_id.erase(vm_id);
    }
    vmIDs.erase(vm_info.vm_id);
}
void Print(vector<int> &vm_ids, vector<MigrationInfo> &migration_infos)
{
    int purchase_type_num = purchase_infos.size();
    cout << "(purchase, " << purchase_type_num << ')' << endl;
    for (auto &purchase_info : purchase_infos)
    {
        cout << '(' << purchase_info.first << ", " << purchase_info.second.size() << ')' << endl;
    }
    //cout << "(migration, 0)" << endl;
    cout << "(migration, " << migration_infos.size() << ")" << endl;
    for (auto &migration_info : migration_infos)
    {
        cout << "(" << migration_info.vm_id << ", " << migration_info.server->server_id;
        if (migration_info.node != 'C')
            cout << ", " << migration_info.node;
        cout << ")" << endl;
    }
    for (auto &vm_id : vm_ids)
    {
        VmIdInfo vm_info = vm_id2info[vm_id];
        char node = vm_info.node;
        if (node == 'C')
        {
            cout << '(' << vm_info.purchase_server->server_id << ')' << endl;
        }
        else
        {
            cout << '(' << vm_info.purchase_server->server_id << ", " << node << ')' << endl;
        }
    }
}
void Numbering()
{
    for (auto &purchase_info : purchase_infos)
    {
        for (auto &server : purchase_info.second)
        {
            server->server_id = number++;
            //当天购买的服务器都是从关机到开机的服务器
            from_off_2_start.insert(server->server_id);
        }
    }
}
void Compute_Power_Cost()
{
    for (auto &server : purchase_servers)
    {
        if (vm_nums(server) != 0)
        {
            //当天结束时候有虚拟机的服务器
            total_power_cost += server_name2info[server->server_name].daily_cost;
        }
        else
        {
            //当天结束时关机，但是当天开过机的服务器，算电费
            // if (from_off_2_start.find(server->server_id) != from_off_2_start.end())
            // {
            //     total_power_cost += server_name2info[server->server_name].daily_cost;
            // }
        }
    }
}

vector<int> GetAllResourceOfOwnServers(bool isRemained = false)
{
    /**
 * @description:获取当前所有已经购买的服务器的总资源 或 剩余资源
 * @param {isRemained 为false ：总资源，true：剩余资源}
 * @return {vector<int> 二维数组 [0]--cpu，[1]--memory}
 */
    int _total_cpu = 0;
    int _total_memory = 0;
    if (!isRemained)
    {
        for (auto &purchase_server : purchase_servers)
        {
            string _serverName = purchase_server->server_name;
            _total_cpu += server_name2info[_serverName].cpu_cores * 2;
            _total_memory += server_name2info[_serverName].memory_size * 2;
        }
    }
    else
    {
        for (auto &purchase_server : purchase_servers)
        {
            _total_cpu += purchase_server->A_remain_core_num + purchase_server->B_remain_core_num;
            _total_memory += purchase_server->A_remain_memory_size + purchase_server->B_remain_memory_size;
            // _total_cpu +=  max(purchase_server->A_remain_core_num , purchase_server->B_remain_core_num);
            // _total_memory += max(purchase_server->A_remain_memory_size , purchase_server->B_remain_memory_size);
        }
    }
    return {_total_cpu, _total_memory};
}

vector<int> GetAllResourceOfToday(vector<RequestData> &reqInfoOfToday)
{
    /**
 * @description: 获取当天请求所需的总资源,当天增加的减去删除的
 * @param {reqInfoOfToday ： 当天的请求信息}
 * @return {vector<int> 二维数组 [0]--cpu，[1]--memory}
 */
    int _total_cpu = 0;
    int _total_memory = 0;
    for (auto &req : reqInfoOfToday)
    {
        string vm_name = req.vm_name;
        int vm_isTwoNode = vm_name2info[vm_name].deployment_way;
        int vm_cpu, vm_memory;
        if (vm_isTwoNode == 1)
        {
            vm_cpu = vm_name2info[vm_name].cpu_cores * 2;
            vm_memory = vm_name2info[vm_name].memory_size * 2;
        }
        else
        {
            vm_cpu = vm_name2info[vm_name].cpu_cores;
            vm_memory = vm_name2info[vm_name].memory_size;
        }
        if (req.operation == "add")
        {
            _total_cpu += vm_cpu;
            _total_memory += vm_memory;
        }
        else if (req.operation == "del")
        {
            _total_cpu -= vm_cpu;
            _total_memory -= vm_memory;
        }
    }
    return {_total_cpu, _total_memory};
}

vector<long long> GetMaxResourceOfToday(vector<RequestData> &reqInfoOfToday)
{
    /**
 * @description: 获取当天请求巅峰时刻的Cpu值和Memory值
 * @param {reqInfoOfToday ： 当天的请求信息}
 * @return {vector<int> 二维数组 [0]--max_cpu，[1]--max_memory}
 */
    long long _total_cpu = 0;
    long long _total_memory = 0;
    long long _max_cpu = 0;
    long long _max_memory = 0;
    for (auto &req : reqInfoOfToday)
    {
        string vm_name = req.vm_name;
        int vm_isTwoNode = vm_name2info[vm_name].deployment_way;
        int vm_cpu, vm_memory;
        if (vm_isTwoNode == 1)
        {
            vm_cpu = vm_name2info[vm_name].cpu_cores * 2;
            vm_memory = vm_name2info[vm_name].memory_size * 2;
        }
        else
        {
            vm_cpu = vm_name2info[vm_name].cpu_cores;
            vm_memory = vm_name2info[vm_name].memory_size;
        }
        if (req.operation == "add")
        {
            _total_cpu += vm_cpu;
            _total_memory += vm_memory;
            if (_total_cpu > _max_cpu)
            {
                _max_cpu = _total_cpu;
            }
            if (_total_memory > _max_memory)
            {
                _max_memory = _total_memory;
            }
        }
        else if (req.operation == "del")
        {
            _total_cpu -= vm_cpu;
            _total_memory -= vm_memory;
        }
    }
    return {_max_cpu, _max_memory};
}

vector<int> GetAllResourceOfFutureNDays(int req_num)
{
    /**
 * @description: 获取未来N条请求所需要的所有资源,以及峰值时刻的CPU，峰值时刻的Memory
 * @param { int req_num}
 * @return {vector<int> 二维数组 [0]--cpu，[1]--memory,[2]--max_cpu,[3]--max_memory}
 */
    queue<vector<RequestData>> _temp(request_datas);
    int _cnt = 0;
    int _total_cpu = 0;
    int _total_memory = 0;
    int _max_cpu = 0;
    int _max_memory = 0;
    while (_temp.size() >= 1 && _cnt < req_num)
    {
        vector<RequestData> _tempReqs = _temp.front();
        _temp.pop();
        for (auto &_req : _tempReqs)
        {
            if (_cnt >= req_num)
            {
                break;
            }
            _cnt++;
            string vm_name = _req.vm_name;
            int vm_isTwoNode = vm_name2info[vm_name].deployment_way;
            int vm_cpu, vm_memory;
            if (vm_isTwoNode == 1)
            {
                vm_cpu = vm_name2info[vm_name].cpu_cores * 2;
                vm_memory = vm_name2info[vm_name].memory_size * 2;
            }
            else
            {
                vm_cpu = vm_name2info[vm_name].cpu_cores;
                vm_memory = vm_name2info[vm_name].memory_size;
            }
            if (_req.operation == "add")
            {
                _total_cpu += vm_cpu;
                _total_memory += vm_memory;
                if (_total_cpu > _max_cpu)
                {
                    _max_cpu = _total_cpu;
                }
                if (_total_memory > _max_memory)
                {
                    _max_memory = _total_memory;
                }
            }
            else if (_req.operation == "del")
            {
                _total_cpu -= vm_cpu;
                _total_memory -= vm_memory;
            }
        }
    }
    return {_total_cpu, _total_memory, _max_cpu, _max_memory};
}

SoldServer *SearchForContinueVM(int deployment_way, int cpu_cores, int memory_size)
{
    /**
     * '
     * @description: 为合并的机器找虚拟机
     * @param {*}
     * @return {SoldServer*}
     */
    SoldServer *flag_sold_server;
    double min_dense_cost = DBL_MAX;
    if (deployment_way == 1)
    {
        for (auto &sold_server : sold_servers)
        {
            if (sold_server.cpu_cores >= cpu_cores && sold_server.memory_size >= memory_size)
            {
                double dense_cost;
                if (isDenseBuy == 1)
                {
                    double _cpu_rate = 1.0 * cpu_cores / sold_server.cpu_cores;
                    double _memory_rate = 1.0 * (memory_size) / sold_server.memory_size;
                    double use_rate = 1.0 * (_cpu_rate + _memory_rate) / 2;

                    dense_cost = 1.0 * (sold_server.hardware_cost + sold_server.daily_cost * (total_days_num - now_day)) * use_rate;
                }
                else
                {
                    dense_cost = sold_server.hardware_cost + sold_server.daily_cost * (total_days_num - now_day);
                }
                if (dense_cost < min_dense_cost)
                {
                    min_dense_cost = dense_cost;
                    flag_sold_server = &sold_server;
                }
            }
        }
    }
    return flag_sold_server;
}

void revokeBuy(int vmID)
{
    string server_name = vm_id2info[vmID].purchase_server->server_name;
    total_server_cost -= server_name2info[server_name].hardware_cost;

    purchase_servers.erase(purchase_servers.end() - 1, purchase_servers.end());
    purchase_infos[server_name].erase(purchase_infos[server_name].end() - 1, purchase_infos[server_name].end());
    if (purchase_infos[server_name].size() == 0)
    {
        purchase_infos.erase(server_name);
    }
    // vm_id2info.erase(vmID);
    vmIDs.erase(vmID);
}

void SolveProblem()
{
    Cmp cmp;

    sort(sold_servers.begin(), sold_servers.end(), cmp.SoldServers);

    for (int i = 0; i < total_days_num; ++i)
    {
        now_day = i + 1;

#ifdef PRINTINFO
        if (now_day % 200 == 0)
            cout << now_day << endl;
#endif

        from_off_2_start.erase(from_off_2_start.begin(), from_off_2_start.end());

        // vector<MigrationInfo> migration_infos;
        vector<MigrationInfo> migration_infos = Migration();
        total_migration_num += migration_infos.size();
        //获取迁移之后的系统可以提供的总资源
        vector<int> allResouceAfterMigration = GetAllResourceOfOwnServers(true);

        //获取未来N条请求所需要的总资源
        allResourceOfNReqs = GetAllResourceOfFutureNDays(300);
        // cout<<allResourceOfNReqs[0] << "  "<<allResourceOfNReqs[1]<<endl;

        intraday_requests = request_datas.front();

        request_datas.pop();

        int request_num = intraday_requests.size();
        vector<int> vm_ids;
        for (int j = 0; j < request_num; ++j)
        { //把请求添加的虚拟机ID有序存起来   !!可能有bug
            if (intraday_requests[j].operation == "add")
            {
                vm_ids.emplace_back(intraday_requests[j].vm_id);
            }
        }

        //获取当天请求所需要的峰值时刻的cpu和memory
        vector<long long> max_resource_of_today_reqs = GetMaxResourceOfToday(intraday_requests);

        double _rate = 1.0;
        if (allResouceAfterMigration[0] * _rate >= max_resource_of_today_reqs[0] && allResouceAfterMigration[1] * _rate >= max_resource_of_today_reqs[1])
        {
            isDenseBuy = 1;
        }
        else
        {
            isDenseBuy = 0;
        }

        vector<int> add_del_count = print_req_num(intraday_requests);

        bool isContinueBuy = (allResouceAfterMigration[0] > add_del_count[0] && allResouceAfterMigration[1] > add_del_count[1]) && (1.1 * (add_del_count[2] + add_del_count[3]) < add_del_count[0] + add_del_count[1]);
        // bool isContinueBuy =( allResouceAfterMigration[0] + allResouceAfterMigration[1] > ( add_del_count[0]  + add_del_count[1])) && (1.1 *( add_del_count[2] + add_del_count[3]) < add_del_count[0]+ add_del_count[1]);
        // cout<<allResouceAfterMigration[0] << "  "<<add_del_count[0]<<"  "<<allResouceAfterMigration[1]<<"  "<<add_del_count[1]<< endl;

        // 一般的处理
        // if (true)
        // {
        //     int j = 0;
        //     allResouceAfterMigration = GetAllResourceOfOwnServers(true);
        //     allResourceOfNReqs = GetAllResourceOfFutureNDays(300);
        //     // cout<<allResourceOfNReqs[2]<<"  " << allResourceOfNReqs[3]<<endl;
        //     string operation = intraday_requests[j].operation;
        //     vector<AddData> continuous_add_datas;
        //     vector<DelData> continuous_del_datas;
        //     int cpu_add_need = 0;
        //     int memory_add_need = 0;

        //     while (j < request_num)
        //     {
        //         string operation = intraday_requests[j].operation;
        //         if (operation == "add")
        //         {
        //             string vm_name = intraday_requests[j].vm_name;
        //             SoldVm sold_vm = vm_name2info[vm_name];
        //             int deployment_way = sold_vm.deployment_way;
        //             int cpu_cores = sold_vm.cpu_cores;
        //             int memory_size = sold_vm.memory_size;

        //             int vm_id = intraday_requests[j].vm_id;
        //             cpu_add_need += (deployment_way + 1) * cpu_cores;
        //             memory_add_need += ((deployment_way + 1) * memory_size);
        //             continuous_add_datas.emplace_back(deployment_way, cpu_cores, memory_size, vm_id, vm_name);
        //             ++j;
        //             if (j == request_num)
        //             {
        //                 break;
        //             }
        //         }
        //         else
        //         {
        //             int vmID = intraday_requests[j].vm_id;
        //             // cout<<allResourceOfNReqs[2]<<"  "<<cpu_add_need<<endl;
        //             if (allResourceOfNReqs[2] >  cpu_add_need && allResourceOfNReqs[3] > memory_add_need)
        //             {
        //                 // cout<<"继续加"<<endl;
        //                 //继续加 把del保存
        //                 continuous_del_datas.emplace_back(vmID);
        //                 ++j;
        //                 if (j == request_num)
        //                 {
        //                     break;
        //                 }
        //             }
        //             else
        //             {
        //                 // 处理所有的Add 然后进行删除
        //                 // cout<<1<< " "<<allResouceAfterMigration[0]<<"  "<<allResouceAfterMigration[1]<<endl;
        //                 // cout<<cpu_add_need<<"  "<<memory_add_need<<endl;
        //                 sort(continuous_add_datas.begin(), continuous_add_datas.end(), cmp.ContinuousADD);
        //                 string last_buy_server_name = "";
        //                 AddData last_add_data;
        //                 for (auto &add_data : continuous_add_datas)
        //                 {
        //                     string buy_server_name = AddVm(add_data);

        //                     bool isSuccess = false;

        //                     if (last_buy_server_name != "" && buy_server_name != "")
        //                     {
        //                         //连续两次购买

        //                         if (last_add_data.deployment_way == 1 && add_data.deployment_way == 1)
        //                         {
        //                             int cpu_cores = last_add_data.cpu_cores + add_data.cpu_cores;
        //                             int memory_cores = last_add_data.memory_size + add_data.memory_size;

        //                             SoldServer *suitServer = 0;
        //                             if (memory_cores > cpu_cores)
        //                             {
        //                                 suitServer = SearchForContinueVM(1, cpu_cores, memory_cores);
        //                             }

        //                             if (suitServer != 0)
        //                             {
        //                                 // int new_cost = suitServer->hardware_cost+ (total_days_num - now_day) * suitServer->daily_cost;
        //                                 // int old_cost = server_name2info[last_buy_server_name].hardware_cost + (total_days_num - now_day) * server_name2info[last_buy_server_name].daily_cost + server_name2info[buy_server_name].hardware_cost + (total_days_num - now_day) * server_name2info[buy_server_name].daily_cost;
        //                                 int new_cost = suitServer->hardware_cost + (total_days_num - now_day) * suitServer->daily_cost;
        //                                 int old_cost = server_name2info[last_buy_server_name].hardware_cost + (total_days_num - now_day) * server_name2info[last_buy_server_name].daily_cost + server_name2info[buy_server_name].hardware_cost + (total_days_num - now_day) * server_name2info[buy_server_name].daily_cost;
        //                                 // cout<<new_cost<<"   "<<old_cost<<endl;
        //                                 int num1 = vm_id2info.size();
        //                                 if (new_cost < old_cost)
        //                                 {
        //                                     count_continue_buy++;
        //                                     revokeBuy(add_data.vm_id);
        //                                     revokeBuy(last_add_data.vm_id);

        //                                     // last_buy_server_name = AddVm(last_add_data);
        //                                     // buy_server_name = AddVm(add_data);
        //                                     BuyAndDeployTwoVM(last_add_data.vm_name, add_data.vm_name, last_add_data.vm_id, add_data.vm_id, suitServer->server_name);
        //                                     isSuccess = true;
        //                                 }
        //                             }
        //                         }

        //                         else if (last_add_data.deployment_way == 0 && add_data.deployment_way == 0)
        //                         {
        //                             int cpu_cores = max(last_add_data.cpu_cores, add_data.cpu_cores);
        //                             int memory_cores = max(last_add_data.memory_size, add_data.memory_size);
        //                             SoldServer *suitServer = SearchForContinueVM(1, cpu_cores, memory_cores);
        //                             if (suitServer != 0)
        //                             {
        //                                 int new_cost = suitServer->hardware_cost + (total_days_num - now_day) * suitServer->daily_cost;
        //                                 int old_cost = server_name2info[last_buy_server_name].hardware_cost + (total_days_num - now_day) * server_name2info[last_buy_server_name].daily_cost + server_name2info[buy_server_name].hardware_cost + (total_days_num - now_day) * server_name2info[buy_server_name].daily_cost;
        //                                 // int new_cost = suitServer->hardware_cost;
        //                                 // int old_cost = server_name2info[last_buy_server_name].hardware_cost + server_name2info[buy_server_name].hardware_cost ;
        //                                 // cout<<new_cost<<"   "<<old_cost<<endl;
        //                                 if (new_cost < old_cost)
        //                                 {
        //                                     count_continue_buy++;
        //                                     revokeBuy(add_data.vm_id);
        //                                     revokeBuy(last_add_data.vm_id);
        //                                     // last_buy_server_name = AddVm(last_add_data);
        //                                     // buy_server_name = AddVm(add_data);
        //                                     BuyAndDeployTwoVM_2(last_add_data.vm_name, add_data.vm_name, last_add_data.vm_id, add_data.vm_id, suitServer->server_name);
        //                                     isSuccess = true;
        //                                 }
        //                             }
        //                         }
        //                     }

        //                     if (isSuccess)
        //                     {
        //                         last_buy_server_name = "";
        //                     }
        //                     else
        //                     {
        //                         last_buy_server_name = buy_server_name;
        //                         last_add_data = add_data;
        //                     }
        //                 }
        //                 // allResouceAfterMigration = GetAllResourceOfOwnServers(true);
        //                 // cout<<2<< " "<<allResouceAfterMigration[0]<<"  "<<allResouceAfterMigration[1]<<endl;
        //                 //处理删除
        //                 continuous_del_datas.emplace_back(vmID);
        //                 for (auto &del : continuous_del_datas)
        //                 {
        //                     DeleteVm(del.vm_id);
        //                 }
        //                 continuous_add_datas.erase(continuous_add_datas.begin(), continuous_add_datas.end());
        //                 continuous_del_datas.erase(continuous_del_datas.begin(), continuous_del_datas.end());
        //                 cpu_add_need = 0;
        //                 memory_add_need = 0;
        //                 allResourceOfNReqs = GetAllResourceOfFutureNDays(300);
        //                 // cout<<3<< " "<<allResouceAfterMigration[0]<<"  "<<allResouceAfterMigration[1]<<endl;
        //                 ++j;
        //                 if (j == request_num)
        //                 {
        //                     break;
        //                 }
        //             }
        //         }
        //     }
        //     if (continuous_add_datas.size() != 0)
        //     {
        //         sort(continuous_add_datas.begin(), continuous_add_datas.end(), cmp.ContinuousADD);
        //         string last_buy_server_name = "";
        //         AddData last_add_data;
        //         for (auto &add_data : continuous_add_datas)
        //         {
        //             string buy_server_name = AddVm(add_data);

        //             bool isSuccess = false;

        //             if (last_buy_server_name != "" && buy_server_name != "")
        //             {
        //                 //连续两次购买

        //                 if (last_add_data.deployment_way == 1 && add_data.deployment_way == 1)
        //                 {
        //                     int cpu_cores = last_add_data.cpu_cores + add_data.cpu_cores;
        //                     int memory_cores = last_add_data.memory_size + add_data.memory_size;

        //                     SoldServer *suitServer = 0;
        //                     if (memory_cores > cpu_cores)
        //                     {
        //                         suitServer = SearchForContinueVM(1, cpu_cores, memory_cores);
        //                     }

        //                     if (suitServer != 0)
        //                     {
        //                         // int new_cost = suitServer->hardware_cost+ (total_days_num - now_day) * suitServer->daily_cost;
        //                         // int old_cost = server_name2info[last_buy_server_name].hardware_cost + (total_days_num - now_day) * server_name2info[last_buy_server_name].daily_cost + server_name2info[buy_server_name].hardware_cost + (total_days_num - now_day) * server_name2info[buy_server_name].daily_cost;
        //                         int new_cost = suitServer->hardware_cost + (total_days_num - now_day) * suitServer->daily_cost;
        //                         int old_cost = server_name2info[last_buy_server_name].hardware_cost + (total_days_num - now_day) * server_name2info[last_buy_server_name].daily_cost + server_name2info[buy_server_name].hardware_cost + (total_days_num - now_day) * server_name2info[buy_server_name].daily_cost;
        //                         // cout<<new_cost<<"   "<<old_cost<<endl;
        //                         int num1 = vm_id2info.size();
        //                         if (new_cost < old_cost)
        //                         {
        //                             count_continue_buy++;
        //                             revokeBuy(add_data.vm_id);
        //                             revokeBuy(last_add_data.vm_id);

        //                             // last_buy_server_name = AddVm(last_add_data);
        //                             // buy_server_name = AddVm(add_data);
        //                             BuyAndDeployTwoVM(last_add_data.vm_name, add_data.vm_name, last_add_data.vm_id, add_data.vm_id, suitServer->server_name);
        //                             isSuccess = true;
        //                         }
        //                     }
        //                 }

        //                 else if (last_add_data.deployment_way == 0 && add_data.deployment_way == 0)
        //                 {
        //                     int cpu_cores = max(last_add_data.cpu_cores, add_data.cpu_cores);
        //                     int memory_cores = max(last_add_data.memory_size, add_data.memory_size);
        //                     SoldServer *suitServer = SearchForContinueVM(1, cpu_cores, memory_cores);
        //                     if (suitServer != 0)
        //                     {
        //                         int new_cost = suitServer->hardware_cost + (total_days_num - now_day) * suitServer->daily_cost;
        //                         int old_cost = server_name2info[last_buy_server_name].hardware_cost + (total_days_num - now_day) * server_name2info[last_buy_server_name].daily_cost + server_name2info[buy_server_name].hardware_cost + (total_days_num - now_day) * server_name2info[buy_server_name].daily_cost;
        //                         // int new_cost = suitServer->hardware_cost;
        //                         // int old_cost = server_name2info[last_buy_server_name].hardware_cost + server_name2info[buy_server_name].hardware_cost ;
        //                         // cout<<new_cost<<"   "<<old_cost<<endl;
        //                         if (new_cost < old_cost)
        //                         {
        //                             count_continue_buy++;
        //                             revokeBuy(add_data.vm_id);
        //                             revokeBuy(last_add_data.vm_id);
        //                             // last_buy_server_name = AddVm(last_add_data);
        //                             // buy_server_name = AddVm(add_data);
        //                             BuyAndDeployTwoVM_2(last_add_data.vm_name, add_data.vm_name, last_add_data.vm_id, add_data.vm_id, suitServer->server_name);
        //                             isSuccess = true;
        //                         }
        //                     }
        //                 }
        //             }

        //             if (isSuccess)
        //             {
        //                 last_buy_server_name = "";
        //             }
        //             else
        //             {
        //                 last_buy_server_name = buy_server_name;
        //                 last_add_data = add_data;
        //             }
        //         }
        //     }

        //     if (continuous_del_datas.size() != 0)
        //     {
        //         for (auto &del : continuous_del_datas)
        //         {
        //             DeleteVm(del.vm_id);
        //         }
        //         continuous_add_datas.erase(continuous_add_datas.begin(), continuous_add_datas.end());
        //         continuous_del_datas.erase(continuous_del_datas.begin(), continuous_del_datas.end());
        //     }

        if (!isContinueBuy)
        {
            for (int j = 0; j < request_num; ++j)
            {
                string operation = intraday_requests[j].operation;
                vector<AddData> continuous_add_datas;
                while (operation == "add" && j < request_num)
                {
                    string vm_name = intraday_requests[j].vm_name;
                    SoldVm sold_vm = vm_name2info[vm_name];
                    int deployment_way = sold_vm.deployment_way;
                    int cpu_cores = sold_vm.cpu_cores;
                    int memory_size = sold_vm.memory_size;
                    int vm_id = intraday_requests[j].vm_id;
                    continuous_add_datas.emplace_back(deployment_way, cpu_cores, memory_size, vm_id, vm_name);
                    ++j;
                    if (j == request_num)
                    {
                        break;
                    }
                    operation = intraday_requests[j].operation;
                }
                sort(continuous_add_datas.begin(), continuous_add_datas.end(), cmp.ContinuousADD);

                string last_buy_server_name = "";
                AddData last_add_data;
                for (auto &add_data : continuous_add_datas)
                {
                    string buy_server_name = AddVm(add_data);

                    bool isSuccess = false;

                    if (last_buy_server_name != "" && buy_server_name != "")
                    {
                        //连续两次购买

                        if (last_add_data.deployment_way == 1 && add_data.deployment_way == 1)
                        {
                            int cpu_cores = last_add_data.cpu_cores + add_data.cpu_cores;
                            int memory_cores = last_add_data.memory_size + add_data.memory_size;

                            SoldServer *suitServer = 0;
                            if (memory_cores > cpu_cores)
                            {
                                suitServer = SearchForContinueVM(1, cpu_cores, memory_cores);
                            }

                            if (suitServer != 0)
                            {
                                // int new_cost = suitServer->hardware_cost+ (total_days_num - now_day) * suitServer->daily_cost;
                                // int old_cost = server_name2info[last_buy_server_name].hardware_cost + (total_days_num - now_day) * server_name2info[last_buy_server_name].daily_cost + server_name2info[buy_server_name].hardware_cost + (total_days_num - now_day) * server_name2info[buy_server_name].daily_cost;
                                int new_cost = suitServer->hardware_cost + (total_days_num - now_day) * suitServer->daily_cost;
                                int old_cost = server_name2info[last_buy_server_name].hardware_cost + (total_days_num - now_day) * server_name2info[last_buy_server_name].daily_cost + server_name2info[buy_server_name].hardware_cost + (total_days_num - now_day) * server_name2info[buy_server_name].daily_cost;
                                // cout<<new_cost<<"   "<<old_cost<<endl;
                                int num1 = vm_id2info.size();
                                if (new_cost < old_cost)
                                {
                                    count_continue_buy++;
                                    revokeBuy(add_data.vm_id);
                                    revokeBuy(last_add_data.vm_id);

                                    // last_buy_server_name = AddVm(last_add_data);
                                    // buy_server_name = AddVm(add_data);
                                    BuyAndDeployTwoVM(last_add_data.vm_name, add_data.vm_name, last_add_data.vm_id, add_data.vm_id, suitServer->server_name);
                                    isSuccess = true;
                                }
                            }
                        }

                        else if (last_add_data.deployment_way == 0 && add_data.deployment_way == 0)
                        {
                            int cpu_cores = max(last_add_data.cpu_cores, add_data.cpu_cores);
                            int memory_cores = max(last_add_data.memory_size, add_data.memory_size);
                            SoldServer *suitServer = SearchForContinueVM(1, cpu_cores, memory_cores);
                            if (suitServer != 0)
                            {
                                int new_cost = suitServer->hardware_cost + (total_days_num - now_day) * suitServer->daily_cost;
                                int old_cost = server_name2info[last_buy_server_name].hardware_cost + (total_days_num - now_day) * server_name2info[last_buy_server_name].daily_cost + server_name2info[buy_server_name].hardware_cost + (total_days_num - now_day) * server_name2info[buy_server_name].daily_cost;
                                // int new_cost = suitServer->hardware_cost;
                                // int old_cost = server_name2info[last_buy_server_name].hardware_cost + server_name2info[buy_server_name].hardware_cost ;
                                // cout<<new_cost<<"   "<<old_cost<<endl;
                                if (new_cost <  old_cost)
                                {
                                    count_continue_buy++;
                                    revokeBuy(add_data.vm_id);
                                    revokeBuy(last_add_data.vm_id);
                                    // last_buy_server_name = AddVm(last_add_data);
                                    // buy_server_name = AddVm(add_data);
                                    BuyAndDeployTwoVM_2(last_add_data.vm_name, add_data.vm_name, last_add_data.vm_id, add_data.vm_id, suitServer->server_name);
                                    isSuccess = true;
                                }
                            }
                        }
                    }

                    if (isSuccess)
                    {
                        last_buy_server_name = "";
                    }
                    else
                    {
                        last_buy_server_name = buy_server_name;
                        last_add_data = add_data;
                    }
                }
                if (j == request_num)
                {
                    break;
                }
                if (operation == "del")
                {
                    int vm_id = intraday_requests[j].vm_id;
                    DeleteVm(vm_id);
                    // vm_id2info.erase(vm_id);
                }
            }
        }
        else
        {

            //收集所有的add操作
            vector<AddData> continuous_add_datas;
            for (int j = 0; j < request_num; ++j)
            {
                string operation = intraday_requests[j].operation;
                if (operation == "add")
                {
                    string vm_name = intraday_requests[j].vm_name;
                    SoldVm sold_vm = vm_name2info[vm_name];
                    int deployment_way = sold_vm.deployment_way;
                    int cpu_cores = sold_vm.cpu_cores;
                    int memory_size = sold_vm.memory_size;
                    int vm_id = intraday_requests[j].vm_id;
                    continuous_add_datas.emplace_back(deployment_way, cpu_cores, memory_size, vm_id, vm_name);
                    operation = intraday_requests[j].operation;
                }
            }

            //对连续的add排序
            sort(continuous_add_datas.begin(), continuous_add_datas.end(), cmp.ContinuousADD);

            string last_buy_server_name = "";
            AddData last_add_data;
            for (auto &add_data : continuous_add_datas)
            {
                string buy_server_name = AddVm(add_data);
                // if(buy_server_name!=""){
                //     revokeBuy(add_data.vm_id);
                //     buy_server_name = AddVm(add_data);
                // }
                bool isSuccess = false;
                if (last_buy_server_name != "" && buy_server_name != "")
                {
                    //连续两次购买

                    if (last_add_data.deployment_way == 1 && add_data.deployment_way == 1)
                    {
                        int cpu_cores = last_add_data.cpu_cores + add_data.cpu_cores;
                        int memory_cores = last_add_data.memory_size + add_data.memory_size;

                        SoldServer *suitServer = 0;
                        if (memory_cores > cpu_cores)
                        {
                            suitServer = SearchForContinueVM(1, cpu_cores, memory_cores);
                        }

                        if (suitServer != 0)
                        {
                            int new_cost = suitServer->hardware_cost;
                            int old_cost = server_name2info[last_buy_server_name].hardware_cost + server_name2info[buy_server_name].hardware_cost;
                            // int new_cost = suitServer->hardware_cost + (total_days_num - now_day) * suitServer->daily_cost;
                            // int old_cost = server_name2info[last_buy_server_name].hardware_cost + (total_days_num - now_day) * server_name2info[last_buy_server_name].daily_cost + server_name2info[buy_server_name].hardware_cost + (total_days_num - now_day) * server_name2info[buy_server_name].daily_cost;
                            // cout<<new_cost<<"   "<<old_cost<<endl;
                            if (new_cost < 2.0 / 3 * old_cost)
                            {
                                count_continue_buy++;
                                revokeBuy(add_data.vm_id);
                                revokeBuy(last_add_data.vm_id);

                                // last_buy_server_name = AddVm(last_add_data);
                                // buy_server_name = AddVm(add_data);
                                BuyAndDeployTwoVM(last_add_data.vm_name, add_data.vm_name, last_add_data.vm_id, add_data.vm_id, suitServer->server_name);
                                isSuccess = true;
                            }
                        }
                    }
                    // if(last_add_data.deployment_way == 1 && add_data.deployment_way == 0){
                    //     int cpu_cores = last_add_data.cpu_cores+add_data.cpu_cores;
                    //     int memory_cores = last_add_data.memory_size + add_data.memory_size;
                    //     SoldServer* suitServer = 0;
                    //     if(memory_cores > 1.5 * cpu_cores){
                    //         suitServer = SearchForContinueVM(1,cpu_cores,memory_cores);
                    //     }
                    //     if(suitServer!=0){
                    //         // int new_cost = suitServer->hardware_cost+ (total_days_num - now_day) * suitServer->daily_cost;
                    //         // int old_cost = server_name2info[last_buy_server_name].hardware_cost + (total_days_num - now_day) * server_name2info[last_buy_server_name].daily_cost + server_name2info[buy_server_name].hardware_cost + (total_days_num - now_day) * server_name2info[buy_server_name].daily_cost;
                    //         int new_cost = suitServer->hardware_cost;
                    //         int old_cost = server_name2info[last_buy_server_name].hardware_cost + server_name2info[buy_server_name].hardware_cost ;
                    //         // cout<<new_cost<<"   "<<old_cost<<endl;
                    //         if(new_cost <2.0 / 3 * old_cost){
                    //             save_cost+=old_cost - new_cost;
                    //             count_continue_buy ++;
                    //             revokeBuy(add_data.vm_id);
                    //             revokeBuy(last_add_data.vm_id);

                    //             // last_buy_server_name = AddVm(last_add_data);
                    //             // buy_server_name = AddVm(add_data);
                    //             BuyAndDeployTwoVM(last_add_data.vm_name,add_data.vm_name,last_add_data.vm_id,add_data.vm_id,suitServer->server_name);
                    //             isSuccess = true;
                    //         }
                    //     }
                    // }
                    // if(last_add_data.deployment_way == 0 && add_data.deployment_way == 1){
                    //     int cpu_cores = last_add_data.cpu_cores+add_data.cpu_cores;
                    //     int memory_cores = last_add_data.memory_size + add_data.memory_size;
                    //     SoldServer* suitServer = 0;
                    //     if(memory_cores > cpu_cores){
                    //         suitServer = SearchForContinueVM(1,cpu_cores,memory_cores);
                    //     }
                    //     if(suitServer!=0){
                    //         // int new_cost = suitServer->hardware_cost+ (total_days_num - now_day) * suitServer->daily_cost;
                    //         // int old_cost = server_name2info[last_buy_server_name].hardware_cost + (total_days_num - now_day) * server_name2info[last_buy_server_name].daily_cost + server_name2info[buy_server_name].hardware_cost + (total_days_num - now_day) * server_name2info[buy_server_name].daily_cost;
                    //         int new_cost = suitServer->hardware_cost;
                    //         int old_cost = server_name2info[last_buy_server_name].hardware_cost + server_name2info[buy_server_name].hardware_cost ;
                    //         // cout<<new_cost<<"   "<<old_cost<<endl;
                    //         if(new_cost <2.0 / 3 * old_cost){
                    //             save_cost+=old_cost - new_cost;
                    //             count_continue_buy ++;
                    //             revokeBuy(add_data.vm_id);
                    //             revokeBuy(last_add_data.vm_id);
                    //             // last_buy_server_name = AddVm(last_add_data);
                    //             // buy_server_name = AddVm(add_data);
                    //             BuyAndDeployTwoVM(last_add_data.vm_name,add_data.vm_name,last_add_data.vm_id,add_data.vm_id,suitServer->server_name);
                    //             isSuccess = true;
                    //         }
                    //     }
                    // }
                    if (last_add_data.deployment_way == 0 && add_data.deployment_way == 0)
                    {
                        int cpu_cores = max(last_add_data.cpu_cores, add_data.cpu_cores);
                        int memory_cores = max(last_add_data.memory_size, add_data.memory_size);
                        SoldServer *suitServer = SearchForContinueVM(1, cpu_cores, memory_cores);
                        if (suitServer != 0)
                        {
                            int new_cost = suitServer->hardware_cost + (total_days_num - now_day) * suitServer->daily_cost;
                            int old_cost = server_name2info[last_buy_server_name].hardware_cost + (total_days_num - now_day) * server_name2info[last_buy_server_name].daily_cost + server_name2info[buy_server_name].hardware_cost + (total_days_num - now_day) * server_name2info[buy_server_name].daily_cost;
                            // int new_cost = suitServer->hardware_cost;
                            // int old_cost = server_name2info[last_buy_server_name].hardware_cost + server_name2info[buy_server_name].hardware_cost ;
                            // cout<<new_cost<<"   "<<old_cost<<endl;
                            if (new_cost < old_cost)
                            {
                                count_continue_buy++;
                                revokeBuy(add_data.vm_id);
                                revokeBuy(last_add_data.vm_id);
                                // last_buy_server_name = AddVm(last_add_data);
                                // buy_server_name = AddVm(add_data);
                                BuyAndDeployTwoVM_2(last_add_data.vm_name, add_data.vm_name, last_add_data.vm_id, add_data.vm_id, suitServer->server_name);
                                isSuccess = true;
                            }
                        }
                    }
                }
                if (isSuccess)
                {
                    last_buy_server_name = "";
                }
                else
                {
                    last_buy_server_name = buy_server_name;
                    last_add_data = add_data;
                }
            }

            //处理所有的del
            for (int j = 0; j < request_num; ++j)
            {
                string operation = intraday_requests[j].operation;
                if (operation == "del")
                {
                    int vm_id = intraday_requests[j].vm_id;
                    DeleteVm(vm_id);
                    if (vm_id2info.count(vm_id) != 0)
                    {
                        // vm_id2info.erase(vm_id);
                    }
                }
            }
        }
        Numbering(); //给购买了的服务器编号
#ifndef PRINTINFO
        Print(vm_ids, migration_infos);
#endif
        fflush(stdout);
        purchase_infos.clear();
        from_off_2_start.clear();
        if (i < total_days_num - foreseen_days_num)
            ParseRequest(1);
        Compute_Power_Cost();
    }
    // cout<<count_continue_buy<<endl;
    // cout<<save_cost<<endl;
    // cout<<count_add_more_del<< "  "<<total_days_num - count_add_more_del<<endl;
    // cout<<total_migration_num<<endl;
}

vector<double> solve_functions(int x1, int y1, int z1, int x2, int y2, int z2)
{
    if (x2 * y1 == x1 * y2)
        return {};
    double a, b;
    a = 1.0 * (y2 * z1 - y1 * z2) / (x1 * y2 - x2 * y1);
    b = 1.0 * (x2 * z1 - x1 * z2) / (x2 * y1 - x1 * y2);
    return {a / (a + b), b / (a + b)};
}

void PrintCostInfo()
{
    cout << "Server Num : " << purchase_servers.size() << endl;
    cout << "Total Migration Num : " << total_migration_num << endl;
#ifdef PRINTINFO
    cout << "Time: " << double(_end - _start) / CLOCKS_PER_SEC << " s" << endl;
#endif
    cout << "Total Cost : " << to_string(total_server_cost) << " + " << to_string(total_power_cost) << " = " << total_power_cost + total_server_cost << endl;
}

int main(int argc, char *argv[])
{

    Statistics sta;
    vector<double> ans;
#ifdef REDIRECT
    // freopen("training-1.txt", "r", stdin);
    freopen("/Users/wangtongling/Desktop/training-data/training-1.txt", "r", stdin);
    // freopen("out1.txt", "w", stdout);
#endif
#ifdef PRINTINFO
    _start = clock();
#endif

    ParseInput();
    // ans = sta.linear_regression(sold_servers);
    // cout<<ans[0]<<" " <<ans[1]<<endl;
    // r1 = ans[0]; r2 = ans[1];
    // k1 = ans[0]; k2 = ans[1];
    SolveProblem();
#ifdef PRINTINFO
    _end = clock();
#endif
#ifdef PRINTINFO
    PrintCostInfo();
#endif

#ifdef MULTIPROCESS
    init();
#ifdef REDIRECT
    // freopen("training-1.txt", "r", stdin);
    freopen("/Users/wangtongling/Desktop/training-data/training-2.txt", "r", stdin);
    // freopen("out1.txt", "w", stdout);
#endif
#ifdef PRINTINFO
    _start = clock();
#endif
    ParseInput();
    // ans = sta.linear_regression(sold_servers);
    // cout<<ans[0]<<" " <<ans[1]<<endl;
    // r1 = ans[0]; r2 = ans[1];
    // k1 = ans[0]; k2 = ans[1];
    SolveProblem();
#ifdef PRINTINFO
    _end = clock();
#endif
#ifdef PRINTINFO
    PrintCostInfo();
#endif

    init();

#ifdef REDIRECT
    // freopen("training-1.txt", "r", stdin);
    freopen("/Users/wangtongling/Desktop/training-data/training-3.txt", "r", stdin);
    // freopen("out1.txt", "w", stdout);
#endif
#ifdef PRINTINFO
    _start = clock();
#endif
    ParseInput();
    // ans = sta.linear_regression(sold_servers);
    // cout<<ans[0]<<" " <<ans[1]<<endl;
    // r1 = ans[0]; r2 = ans[1];
    // k1 = ans[0]; k2 = ans[1];
    SolveProblem();
#ifdef PRINTINFO
    _end = clock();
#endif
#ifdef PRINTINFO
    PrintCostInfo();
#endif

    init();

#ifdef REDIRECT
    // freopen("training-1.txt", "r", stdin);
    freopen("/Users/wangtongling/Desktop/training-data/training-4.txt", "r", stdin);
    // freopen("out1.txt", "w", stdout);
#endif
#ifdef PRINTINFO
    _start = clock();
#endif
    ParseInput();
    // ans = sta.linear_regression(sold_servers);
    // cout<<ans[0]<<" " <<ans[1]<<endl;
    // r1 = ans[0]; r2 = ans[1];
    // k1 = ans[0]; k2 = ans[1];
    SolveProblem();
#ifdef PRINTINFO
    _end = clock();
#endif
#ifdef PRINTINFO
    PrintCostInfo();
#endif

#endif

#ifdef REDIRECT
    fclose(stdin);
    fclose(stdout);
#endif
    return 0;
}