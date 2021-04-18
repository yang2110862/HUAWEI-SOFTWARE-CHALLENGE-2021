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

unordered_map<string, vector<int>> my_offers; //保存自己对于某种虚拟机的报价。
unordered_map<string, vector<pair<int, int>>> rival_and_user_offers; //保存对手对于某种虚拟机的报价，第一项为对手报价，第二项为用户报价。
unordered_map<string, pair<double, double>> statistics; //保存对手对于某种虚拟机报价的平均值和方差。（此数据不在调用时计算，而在增加信息时更新。）
unordered_map<string, int> valid_data_sizes; //保存对手对于某种虚拟机报价数据的有效数据量（即报价不为-1的）。
unordered_map<string, int> rival_lower_bounds; //保存对手对于某种虚拟机的成本下界。


int count_continue_buy = 0;

int count_add_more_del = 0;

double Start_rate = 1.0 / 2;
double End_rate = 4.0 / 4;

int total_days_num, foreseen_days_num; //总天数和可预知的天数
int now_day;                           //现在是第几天
int total_req_num = 0;

int number = 0; //给服务器编号
long long total_server_cost = 0;
long long total_power_cost = 0;
int total_migration_num = 0;

bool isUsed = false;

double hardware_cost_per_day_per_resource = 0;
double power_cost_per_day_per_resource = 0;

#ifdef PRINTINFO
clock_t _start, _end;
#endif

//状态值
int isDenseBuy = 0; // 0--非密度购买  1--密度购买
double _future_N_reqs_cpu_rate = 0;
double _future_N_reqs_memory_rate = 0;
double _migration_threshold = 0.024; //减小能增加迁移数量。

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

void update_statistics(string vm_name) {
    pair<int, int> new_offers = rival_and_user_offers[vm_name].back();
    if (new_offers.first != -1) {
        int new_data = new_offers.first;
        pair<double, double>* avg_and_var = &statistics[vm_name];
        double old_avg = avg_and_var->first, old_var = avg_and_var->second;
        int valid_data_size = valid_data_sizes[vm_name]++;
        avg_and_var->first = (old_avg * valid_data_size + new_data) / (valid_data_size + 1);
        avg_and_var->second = ((old_var + old_avg * old_avg) * valid_data_size + new_data * new_data) / (valid_data_size + 1) - avg_and_var->first * avg_and_var->first;
    } else {
        int temp = rival_lower_bounds[vm_name];
        rival_lower_bounds[vm_name] = temp == 0 ? new_offers.second : min(temp, new_offers.second);
    }
}

int FindMaxDel(){
    vector<int> delNums;
    for(int i = now_day;i < now_day + foreseen_days_num ;i++){
        if(i<total_days_num+1){
            vector<RequestData> temp = request_datas.front();
            int cnt_del = 0;
            for(auto & data : temp){
                if(data.operation == "del"){
                    cnt_del++;

                }
            }
            delNums.emplace_back(cnt_del);
        }
        request_datas.push(request_datas.front());
        request_datas.pop();
    }

    int max_del_num = -1;
    int flag_day =  0;
    int sum = 0;

    for(int i = 0;i<delNums.size();i++){
        if(delNums[i]>max_del_num){
            max_del_num = delNums[i];
            flag_day = now_day+i;
        }
        sum += delNums[i];
    }

    if(max_del_num > 2.0 * (1.0 * sum / delNums.size())){
        return flag_day;
    }else{
        return -1;
    }
    return -1;
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
        return r1 * server->A_remain_cpu / server->total_cpu + r2 * server->A_remain_memory / server->total_memory;
    }
    else
    {
        return r1 * server->B_remain_cpu / server->total_cpu + r2 * server->B_remain_memory / server->total_memory;
    }
}

void ParseServerInfo()
{
    int server_num;
    cin >> server_num;
    string server_name, cpu_core, memory, hardware_cost, daily_cost;
    for (int i = 0; i < server_num; ++i)
    {
        SoldServer sold_server;
        cin >> server_name >> cpu_core >> memory >> hardware_cost >> daily_cost;
        sold_server.server_name = server_name.substr(1, server_name.size() - 2);
        sold_server.cpu = stoi(cpu_core.substr(0, cpu_core.size() - 1)) / 2;
        sold_server.memory = stoi(memory.substr(0, memory.size() - 1)) / 2;
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
    string vm_name, cpu_core, memory, deployment_way;
    for (int i = 0; i < vm_num; ++i)
    {
        SoldVm sold_vm;
        cin >> vm_name >> cpu_core >> memory >> deployment_way;
        vm_name = vm_name.substr(1, vm_name.size() - 2);
        sold_vm.cpu = stoi(cpu_core.substr(0, cpu_core.size() - 1));
        sold_vm.memory = stoi(memory.substr(0, memory.size() - 1));
        sold_vm.deployment_way = stoi(deployment_way.substr(0, deployment_way.size() - 1));
        if (sold_vm.deployment_way == 1)
        {
            sold_vm.cpu /= 2;
            sold_vm.memory /= 2;
        }
        vm_name2info[vm_name] = sold_vm;
    }
}

void ParseRequest(int days_num)
{
    int operation_num;
    string operation, vm_name, vm_id, duration, user_offer;
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
                cin >> vm_name >> vm_id >> duration >> user_offer;
                data.vm_name = vm_name.substr(0, vm_name.size() - 1);
                data.vm_id = stoi(vm_id.substr(0, vm_id.size() - 1));
                data.duration = stoi(duration.substr(0, duration.size() - 1));
                data.user_offer = stoi(user_offer.substr(0, user_offer.size() - 1));
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

/**
 * @brief 读取与对手的竞争信息。
 * @param {int} num 当天add请求的数量。
 * @return {*} pair数组（是否得到虚拟机，对手报价）。
 */
vector<pair<int, int>> ParseCompeteInfo(int num) {
    vector<pair<int, int>> compete_infos;
    string have_got, rival_offer;
    for (int i = 0; i < num; ++i) {
        cin >> have_got >> rival_offer;
        compete_infos.emplace_back(make_pair(stoi(have_got.substr(1, 1)), stoi(rival_offer.substr(0, rival_offer.size() - 1))));
    }
    return compete_infos;
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
        cout << sold_server.server_name << " " << sold_server.cpu << " " << sold_server.memory
             << " " << sold_server.hardware_cost << " " << sold_server.daily_cost << endl;
    }
    cout << vm_name2info.size() << endl;
    for (auto &vm : vm_name2info)
    {
        cout << vm.first << " " << vm.second.cpu << " " << vm.second.memory << " "
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

vector<RequestData> ParseBidingRes(vector<pair<int,int>>& bidingRes,vector<RequestData>& allReq){
/**
 * @description: 处理报价竞争的结果。1、转化为真实的请求信息vector<RequestData>，便于后续部署处理
 *                                2、保存对手对于某种虚拟机的历史报价信息
 * @param {*}
 * @return {*}
 */
    int index = 0;
    unordered_set<int> today_add_today_del_vmid;

    vector<RequestData> res;
    for(auto& req:allReq){
        if(req.operation == "add"){
            int biding_res_flag = bidingRes[index].first;
            int competitor_price = bidingRes[index].second;
            if(biding_res_flag == 1){
                //获得了订单
                res.emplace_back(req);
            }
            //保存对手报价
            rival_and_user_offers[req.vm_name].emplace_back(make_pair(competitor_price, req.user_offer));
            // 每次记录报价时进行统计量更新。
            update_statistics(req.vm_name);
            //保存当天创建当天删除虚拟机信息
            if(req.duration == 0 && biding_res_flag == 1) today_add_today_del_vmid.insert(req.vm_id);
            ++index;
        }else if(req.operation == "del"){
            if (vmIDs.find(req.vm_id) != vmIDs.end()) {
                res.emplace_back(req);
            } else {
                if (today_add_today_del_vmid.find(req.vm_id) != today_add_today_del_vmid.end()) {
                    res.emplace_back(req);
                }
            }
            // if(today_add_today_del_vmid.find(req.vm_id) == today_add_today_del_vmid.end()){
            //     if(vmIDs.find(req.vm_id) != vmIDs.end()){
            //         //是己方虚拟机
            //         res.emplace_back(req);
            //     }
            // }else{
            //     res.emplace_back(req);
            // }
        }
    }
    return res;
}
double total_hardware_cost_per_day = 0;
long long total_daily_power_cost = 0;
int total_left_day_nums = 0;
double total_resource = 0;
void UpdateHardwareCost(string serverName ,bool isRevoke = false){
    if(!isRevoke){
        total_hardware_cost_per_day += 1.0 * (server_name2info[serverName].hardware_cost) / (total_days_num - now_day);
        total_daily_power_cost += server_name2info[serverName].daily_cost;
        total_resource  += 1.0 * 2 * ( server_name2info[serverName].cpu) + 1.0 * 2 * ( server_name2info[serverName].memory);
        if(total_hardware_cost_per_day == 0) hardware_cost_per_day_per_resource = 0;
        else hardware_cost_per_day_per_resource = 1.0 * total_hardware_cost_per_day / (total_resource);
        if(total_daily_power_cost == 0) power_cost_per_day_per_resource = 0;
        else power_cost_per_day_per_resource = 1.0 * total_daily_power_cost / total_resource;
    }else{
        total_hardware_cost_per_day -= 1.0 * (server_name2info[serverName].hardware_cost) / (total_days_num - now_day);
        total_daily_power_cost -= server_name2info[serverName].daily_cost;
        total_resource  -= 1.0 * 2 * ( server_name2info[serverName].cpu) + 1.0 * 2 * ( server_name2info[serverName].memory);
        if(total_hardware_cost_per_day == 0) hardware_cost_per_day_per_resource = 0;
        else hardware_cost_per_day_per_resource = 1.0 * total_hardware_cost_per_day / (total_resource);
        if(total_daily_power_cost == 0) power_cost_per_day_per_resource = 0;
        else power_cost_per_day_per_resource = 1.0 * total_daily_power_cost / total_resource;
    }
    
}




// 筛选出利用率较低的服务器，迁移走其中的虚拟机。
bool NeedMigration(PurchasedServer *server)
{
    if (vm_nums(server) == 0)
        return false;
    double _A_cpu_remain_rate = 1.0 * server->A_remain_cpu / server->total_cpu;
    double _B_cpu_remain_rate = 1.0 * server->B_remain_cpu / server->total_cpu;
    double _A_memory_remain_rate = 1.0 * server->A_remain_memory / server->total_memory;
    double _B_memory_remain_rate = 1.0 * server->B_remain_memory / server->total_memory;
    return (_A_cpu_remain_rate > _migration_threshold) + (_A_memory_remain_rate > _migration_threshold) + (_B_cpu_remain_rate > _migration_threshold) + (_B_memory_remain_rate > _migration_threshold) >= 1;
    // return ( (_A_cpu_remain_rate > _migration_threshold && _A_memory_remain_rate > _migration_threshold) + (_B_cpu_remain_rate > _migration_threshold && _B_memory_remain_rate > _migration_threshold) ) >=1;
}

bool NeedMigration_2(PurchasedServer *server)
{
    if (vm_nums(server) == 0)
        return false;
    double _A_cpu_remain_rate = 1.0 * server->A_remain_cpu / server->total_cpu;
    double _B_cpu_remain_rate = 1.0 * server->B_remain_cpu / server->total_cpu;
    double _A_memory_remain_rate = 1.0 * server->A_remain_memory / server->total_memory;
    double _B_memory_remain_rate = 1.0 * server->B_remain_memory / server->total_memory;
    return (_A_cpu_remain_rate > _migration_threshold) + (_A_memory_remain_rate > _migration_threshold) + (_B_cpu_remain_rate > _migration_threshold) + (_B_memory_remain_rate > _migration_threshold) >= 1;
}

int _HowManyCondionSuit(PurchasedServer *server)
{
    /**
 * @description: 接NeedMigration，返回服务器满足的条件个数
 * @param {*}
 * @return {个数}
 */
    double _A_cpu_remain_rate = 1.0 * server->A_remain_cpu / server->total_cpu;
    double _B_cpu_remain_rate = 1.0 * server->B_remain_cpu / server->total_cpu;
    double _A_memory_remain_rate = 1.0 * server->A_remain_memory / server->total_memory;
    double _B_memory_remain_rate = 1.0 * server->B_remain_memory / server->total_memory;
    return (_A_cpu_remain_rate > _migration_threshold) + (_A_memory_remain_rate > _migration_threshold) + (_B_cpu_remain_rate > _migration_threshold) + (_B_memory_remain_rate > _migration_threshold);
}

// 筛选出几乎满了的服务器，不作为目标服务器。
bool NearlyFull(PurchasedServer *server)
{
    return (1.0 * server->A_remain_cpu / server->total_cpu < _near_full_threshold || 1.0 * server->A_remain_memory / server->total_memory < _near_full_threshold) && (1.0 * server->B_remain_cpu / server->total_cpu < _near_full_threshold || 1.0 * server->B_remain_memory / server->total_memory < _near_full_threshold);
}

bool NearlyFull_2(PurchasedServer *server)
{
    return (1.0 * server->A_remain_cpu / server->total_cpu < _near_full_threshold_2 || 1.0 * server->A_remain_memory / server->total_memory < _near_full_threshold_2) && (1.0 * server->B_remain_cpu / server->total_cpu < _near_full_threshold_2 || 1.0 * server->B_remain_memory / server->total_memory < _near_full_threshold_2);
}

// 将某台虚拟机迁移至指定位置，调用前请确保能装入。
void migrate_to(VmIdInfo *vm_info, PurchasedServer *target_server, char target_node, vector<MigrationInfo> &migration_infos)
{
    int cpu = vm_info->cpu, memory = vm_info->memory, vm_id = vm_info->vm_id;
    char original_node = vm_info->node;
    PurchasedServer *original_server = vm_info->purchase_server;
    if (original_node == 'A')
    {
        original_server->A_remain_cpu += cpu;
        original_server->A_remain_memory += memory;
        original_server->A_vm_id.erase(vm_id);
    }
    else if (original_node == 'B')
    {
        original_server->B_remain_cpu += cpu;
        original_server->B_remain_memory += memory;
        original_server->B_vm_id.erase(vm_id);
    }
    else
    {
        original_server->A_remain_cpu += cpu;
        original_server->A_remain_memory += memory;
        original_server->B_remain_cpu += cpu;
        original_server->B_remain_memory += memory;
        original_server->AB_vm_id.erase(vm_id);
    }

    if (target_node == 'A')
    {
        target_server->A_remain_cpu -= cpu;
        target_server->A_remain_memory -= memory;
        target_server->A_vm_id.emplace(vm_id);
        vm_info->purchase_server = target_server;
        vm_info->node = 'A';
        migration_infos.emplace_back(vm_id, target_server, 'A');
    }
    else if (target_node == 'B')
    {
        target_server->B_remain_cpu -= cpu;
        target_server->B_remain_memory -= memory;
        target_server->B_vm_id.emplace(vm_id);
        vm_info->purchase_server = target_server;
        vm_info->node = 'B';
        migration_infos.emplace_back(vm_id, target_server, 'B');
    }
    else
    {
        target_server->A_remain_cpu -= cpu;
        target_server->A_remain_memory -= memory;
        target_server->B_remain_cpu -= cpu;
        target_server->B_remain_memory -= memory;
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
        sum_cpu += vm_info->cpu;
        sum_memory += vm_info->memory;
        vm_info->purchase_server = target_server;
        vm_info->node = target_node;
        migration_infos.emplace_back(vm_id, target_server, target_node);
    }

    if (original_node == 'A' || original_node == 'C')
    {
        original_server->A_remain_cpu += sum_cpu;
        original_server->A_remain_memory += sum_memory;
    }
    if (original_node == 'B' || original_node == 'C')
    {
        original_server->B_remain_cpu += sum_cpu;
        original_server->B_remain_memory += sum_memory;
    }

    if (target_node == 'A')
    {
        target_server->A_remain_cpu -= sum_cpu;
        target_server->A_remain_memory -= sum_memory;
        target_server->A_vm_id.insert(vm_ids.begin(), vm_ids.end());
    }
    else if (target_node == 'B')
    {
        target_server->B_remain_cpu -= sum_cpu;
        target_server->B_remain_memory -= sum_memory;
        target_server->B_vm_id.insert(vm_ids.begin(), vm_ids.end());
    }
    else
    {
        target_server->A_remain_cpu -= sum_cpu;
        target_server->A_remain_memory -= sum_memory;
        target_server->B_remain_cpu -= sum_cpu;
        target_server->B_remain_memory -= sum_memory;
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
            cpu_add += (vm_name2info[vm_name].cpu) * (vm_name2info[vm_name].deployment_way + 1);
            memory_add += (vm_name2info[vm_name].memory) * (vm_name2info[vm_name].deployment_way + 1);
        }
        else if (operation == "del")
        {

            int vm_id = req.vm_id;
            string vm_name = vm_id2info[vm_id].vm_name;
            cpu_del += (vm_name2info[vm_name].cpu) * (vm_name2info[vm_name].deployment_way + 1);
            memory_del += (vm_name2info[vm_name].memory) * (vm_name2info[vm_name].deployment_way + 1);
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
                return 1.0 * (vm1->cpu + vm1->memory) * (vm1->node == 'C' ? 1 : 1) > 1.0 * (vm2->cpu + vm2->memory) * (vm2->node == 'C' ? 1 : 1);
            }
            else
            {
                return false;
            }
        });

        // sort(migrating_vms.begin(), migrating_vms.end(), [&](VmIdInfo *vm1,VmIdInfo *vm2) {
        //     PurchasedServer *server1 = vm1->purchase_server, *server2 = vm2->purchase_server;
        //     if(1.0*(vm1->cpu  + vm1->memory ) * (vm1->node == 'C' ? 1 : 1)  > 1.0* (vm2->cpu  + vm2->memory ) * (vm2->node == 'C' ? 1 : 1) ) return true;
        //     else if( 1.0*(vm1->cpu  + vm1->memory ) * (vm1->node == 'C' ? 1 : 1)  == 1.0* (vm2->cpu  + vm2->memory ) * (vm2->node == 'C' ? 1 : 1)){
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
                int cpu = vm_info->cpu;
                int memory = vm_info->memory;
                double _original_rate = remain_rate(original_server, vm_info->node) * original_server->daily_cost;
                double min_rate = _original_rate;
                PurchasedServer *best_server;
                char which_node = '!';
                for (auto &target_server : target_servers)
                { //找最合适的服务器。
                    if (!(target_server == original_server && vm_info->node == 'A') && (target_server->A_remain_cpu >= cpu && target_server->A_remain_memory >= memory))
                    {
                        double rate = r1 * (target_server->A_remain_cpu - cpu) / target_server->total_cpu * target_server->daily_cost + r2 * (target_server->A_remain_memory - memory) / target_server->total_memory * target_server->daily_cost;

                        if (rate < min_rate)
                        {
                            min_rate = rate;
                            best_server = target_server;
                            which_node = 'A';
                        }
                    }
                    if (!(target_server == original_server && vm_info->node == 'B') && (target_server->B_remain_cpu >= cpu && target_server->B_remain_memory >= memory))
                    {
                        double rate = r1 * (target_server->B_remain_cpu - cpu) / target_server->total_cpu * target_server->daily_cost + r2 * (target_server->B_remain_memory - memory) / target_server->total_memory * target_server->daily_cost;

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
                int cpu = vm_info->cpu;
                int memory = vm_info->memory;
                double _original_rate = (remain_rate(original_server, 'A') + remain_rate(original_server, 'B')) / 2 * original_server->daily_cost;
                double min_rate = _original_rate;
                PurchasedServer *best_server = NULL;
                for (auto &target_server : target_servers)
                { //找最合适的服务器。
                    if (target_server == original_server)
                        continue;
                    if (target_server->A_remain_cpu >= cpu && target_server->A_remain_memory >= memory && target_server->B_remain_cpu >= cpu && target_server->B_remain_memory >= memory)
                    {

                        double rate = r1 * (target_server->A_remain_cpu - cpu + target_server->B_remain_cpu - cpu) / target_server->total_cpu / 2 * target_server->daily_cost + r2 * (target_server->A_remain_memory - memory + target_server->B_remain_memory - memory) / target_server->total_memory / 2 * target_server->daily_cost;
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
            if (migration_infos.size() + original_server->A_vm_id.size() > max_migration_num && migration_infos.size() + original_server->B_vm_id.size() > max_migration_num)
                continue;
            double rate1a = remain_rate(original_server, 'A'), rate1b = remain_rate(original_server, 'B');
            if ((rate1a < _near_full_threshold && rate1b > 1 - _near_full_threshold) || (rate1b < _near_full_threshold && rate1a > 1 - _near_full_threshold))
            {
                int sum_cpu[] = {0, 0, 0}, sum_memory[] = {0, 0, 0}; //分别代表A，B，C结点的已有大小。
                for (int vm_id : original_server->AB_vm_id)
                {
                    VmIdInfo *vm_info = &vm_id2info[vm_id];
                    sum_cpu[2] += vm_info->cpu;
                    sum_memory[2] += vm_info->memory;
                }
                sum_cpu[0] = original_server->total_cpu - original_server->A_remain_cpu - sum_cpu[2];
                sum_cpu[1] = original_server->total_cpu - original_server->B_remain_cpu - sum_cpu[2];
                sum_memory[0] = original_server->total_memory - original_server->A_remain_memory - sum_memory[2];
                sum_memory[1] = original_server->total_memory - original_server->B_remain_memory - sum_memory[2];
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
                        if (target_server->B_remain_cpu >= sum_cpu[original_node - 'A'] && target_server->B_remain_memory >= sum_memory[original_node - 'A'])
                        {
                            double diff = k1 * (target_server->B_remain_cpu - sum_cpu[original_node - 'A']) + k2 * (target_server->B_remain_memory - sum_memory[original_node - 'A']);
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
                        if (target_server->A_remain_cpu >= sum_cpu[original_node - 'A'] && target_server->A_remain_memory >= sum_memory[original_node - 'A'])
                        {
                            double diff = k1 * (target_server->A_remain_cpu - sum_cpu[original_node - 'A']) + k2 * (target_server->A_remain_memory - sum_memory[original_node - 'A']);
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
                        if (migration_infos.size() + original_server->A_vm_id.size() <= max_migration_num)
                        {
                            migrate_to(original_server, 'A', best_server[index], which_node[index], migration_infos);
                            if (migration_infos.size() == max_migration_num)
                                return migration_infos;
                        }
                        // 如果能，顺带迁B。
                        if (best_server[index]->A_remain_cpu >= sum_cpu[1] && best_server[index]->A_remain_memory >= sum_memory[1] && which_node[index] == 'B')
                        { //B迁A。
                            if (migration_infos.size() + original_server->B_vm_id.size() <= max_migration_num)
                            {
                                migrate_to(original_server, 'B', best_server[index], 'A', migration_infos);
                                if (migration_infos.size() == max_migration_num)
                                    return migration_infos;
                            }
                        }
                        if (best_server[index]->B_remain_cpu >= sum_cpu[1] && best_server[index]->B_remain_memory >= sum_memory[1] && which_node[index] == 'A')
                        { //B迁B。
                            if (migration_infos.size() + original_server->B_vm_id.size() <= max_migration_num)
                            {
                                migrate_to(original_server, 'B', best_server[index], 'B', migration_infos);
                                if (migration_infos.size() == max_migration_num)
                                    return migration_infos;
                            }
                        }
                        if (best_server[index]->A_remain_cpu >= sum_cpu[2] && best_server[index]->A_remain_memory >= sum_memory[2] && best_server[index]->B_remain_cpu >= sum_cpu[2] && best_server[index]->B_remain_memory >= sum_memory[2])
                        { //双节点虚拟机迁移。
                            if (migration_infos.size() + original_server->AB_vm_id.size() <= max_migration_num)
                            {
                                migrate_to(original_server, 'C', best_server[index], 'C', migration_infos);
                                if (migration_infos.size() == max_migration_num)
                                    return migration_infos;
                            }
                        }
                    }
                    else
                    {
                        if (migration_infos.size() + original_server->B_vm_id.size() <= max_migration_num)
                        {
                            migrate_to(original_server, 'B', best_server[index], which_node[index], migration_infos);
                            if (migration_infos.size() == max_migration_num)
                                return migration_infos;
                        }
                        // 如果能，顺带迁A。
                        if (best_server[index]->A_remain_cpu >= sum_cpu[0] && best_server[index]->A_remain_memory >= sum_memory[0] && which_node[index] == 'B')
                        { //A迁A。
                            if (migration_infos.size() + original_server->A_vm_id.size() <= max_migration_num)
                            {
                                migrate_to(original_server, 'A', best_server[index], 'A', migration_infos);
                                if (migration_infos.size() == max_migration_num)
                                    return migration_infos;
                            }
                        }
                        if (best_server[index]->B_remain_cpu >= sum_cpu[0] && best_server[index]->B_remain_memory >= sum_memory[0] && which_node[index] == 'A')
                        { //A迁B。
                            if (migration_infos.size() + original_server->A_vm_id.size() <= max_migration_num)
                            {
                                migrate_to(original_server, 'A', best_server[index], 'B', migration_infos);
                                if (migration_infos.size() == max_migration_num)
                                    return migration_infos;
                            }
                        }
                        if (best_server[index]->A_remain_cpu >= sum_cpu[2] && best_server[index]->A_remain_memory >= sum_memory[2] && best_server[index]->B_remain_cpu >= sum_cpu[2] && best_server[index]->B_remain_memory >= sum_memory[2])
                        { //双节点虚拟机迁移。
                            if (migration_infos.size() + original_server->AB_vm_id.size() <= max_migration_num)
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
        //     double balance_rate = log(1.0 * (server->A_remain_cpu + server->A_remain_memory) / (server->B_remain_cpu + server->B_remain_memory)) ;
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
        //         return vm_id2info[vmA].cpu + vm_id2info[vmA].memory  < vm_id2info[vmB].cpu + vm_id2info[vmB].memory ;
        //     });
        //     double _balanceRateBefore = fabs( log(1.0 * (server->A_remain_cpu + server->A_remain_memory) / (server->B_remain_cpu + server->B_remain_memory))) ;
        //     for(auto&vmID:_tempVMID){
        //         if(server->B_remain_cpu < vm_id2info[vmID].cpu || server->B_remain_memory < vm_id2info[vmID].memory) continue;
        //         double _balanceRateAfter = fabs( log(1.0 * (server->A_remain_cpu + vm_id2info[vmID].cpu + server->A_remain_memory+ vm_id2info[vmID].memory) / (server->B_remain_cpu + server->B_remain_memory - vm_id2info[vmID].cpu- vm_id2info[vmID].memory)))  ;
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
        //         return vm_id2info[vmA].cpu + vm_id2info[vmA].memory  < vm_id2info[vmB].cpu + vm_id2info[vmB].memory ;
        //     });
        //     double _balanceRateBefore = fabs( log(1.0 * (server->A_remain_cpu + server->A_remain_memory) / (server->B_remain_cpu + server->B_remain_memory))) ;
        //     for(auto&vmID:_tempVMID){
        //         if(server->A_remain_cpu < vm_id2info[vmID].cpu || server->A_remain_memory < vm_id2info[vmID].memory) continue;
        //         double _balanceRateAfter = fabs( log(1.0 * (server->A_remain_cpu - vm_id2info[vmID].cpu + server->A_remain_memory- vm_id2info[vmID].memory) / (server->B_remain_cpu + server->B_remain_memory + vm_id2info[vmID].cpu+ vm_id2info[vmID].memory)))  ;
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
                return min(server1->A_remain_cpu, server1->A_remain_memory) < min(server2->A_remain_cpu, server2->A_remain_memory);
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
                // return  server1->total_cpu - server1->A_remain_cpu - server1->B_remain_cpu + 2 * server1->total_memory - server1->A_remain_memory - server1->B_remain_memory < 2 * server2->total_cpu - server2->A_remain_cpu - server2->B_remain_cpu + 2 * server2->total_memory - server2->A_remain_memory - server2->B_remain_memory;
            }
            else
            {
                return false;
            }
        });
        for (auto original_server : original_servers)
        {
            if (migration_infos.size() + vm_nums(original_server) > max_migration_num) continue;
            for (auto target_server : target_servers)
            {
                if (target_server != original_server && (vm_nums(target_server) == 0 ? target_server->daily_cost : 0) <= 0.9 * original_server->daily_cost && migration_infos.size() + vm_nums(original_server) <= max_migration_num && target_server->A_remain_cpu >= original_server->total_cpu - original_server->A_remain_cpu && target_server->A_remain_memory >= original_server->total_memory - original_server->A_remain_memory && target_server->B_remain_cpu >= original_server->total_cpu - original_server->B_remain_cpu && target_server->B_remain_memory >= original_server->total_memory - original_server->B_remain_memory)
                {
                    migrate_to(original_server, 'C', target_server, 'C', migration_infos);
                    migrate_to(original_server, 'A', target_server, 'A', migration_infos);
                    migrate_to(original_server, 'B', target_server, 'B', migration_infos);
                    if (migration_infos.size() == max_migration_num){
                        if( Start_rate * total_days_num <=now_day&& End_rate * total_days_num >=now_day && !isUsed){
                            isUsed = true;
                            // cout<<"Used!!!!"<<endl;
                            max_migration_num = vmIDs.size();
                        }else{
                            return migration_infos;
                        }
                    }
                }
                else if (target_server != original_server && (vm_nums(target_server) == 0 ? target_server->daily_cost : 0) <= 0.9 * original_server->daily_cost && migration_infos.size() + vm_nums(original_server) <= max_migration_num && target_server->A_remain_cpu >= original_server->total_cpu - original_server->B_remain_cpu && target_server->A_remain_memory >= original_server->total_memory - original_server->B_remain_memory && target_server->B_remain_cpu >= original_server->total_cpu - original_server->A_remain_cpu && target_server->B_remain_memory >= original_server->total_memory - original_server->A_remain_memory)
                {
                    migrate_to(original_server, 'C', target_server, 'C', migration_infos);
                    migrate_to(original_server, 'A', target_server, 'B', migration_infos);
                    migrate_to(original_server, 'B', target_server, 'A', migration_infos);
                    if (migration_infos.size() == max_migration_num ){
                        if(Start_rate * total_days_num <=now_day&& End_rate * total_days_num >=now_day && !isUsed){
                            isUsed = true;
                            // cout<<"Used!!!!"<<endl;
                            max_migration_num = vmIDs.size();
                        }else{
                            return migration_infos;
                        }
                    }
                        
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
                return 1.0 * (vm1->cpu + vm1->memory) * (vm1->node == 'C' ? 1 : 1) > 1.0 * (vm2->cpu + vm2->memory) * (vm2->node == 'C' ? 1 : 1);
            }
            else
            {
                return false;
            }
        });

        // sort(migrating_vms.begin(), migrating_vms.end(), [&](VmIdInfo *vm1,VmIdInfo *vm2) {
        //     PurchasedServer *server1 = vm1->purchase_server, *server2 = vm2->purchase_server;
        //     if(1.0*(vm1->cpu  + vm1->memory ) * (vm1->node == 'C' ? 1 : 1)  > 1.0* (vm2->cpu  + vm2->memory ) * (vm2->node == 'C' ? 1 : 1) ) return true;
        //     else if( 1.0*(vm1->cpu  + vm1->memory ) * (vm1->node == 'C' ? 1 : 1)  == 1.0* (vm2->cpu  + vm2->memory ) * (vm2->node == 'C' ? 1 : 1)){
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
                int cpu = vm_info->cpu;
                int memory = vm_info->memory;
                double _original_rate = remain_rate(original_server, vm_info->node) * original_server->daily_cost;
                double min_rate = _original_rate;
                PurchasedServer *best_server;
                char which_node = '!';
                for (auto &target_server : target_servers)
                { //找最合适的服务器。
                    if (!(target_server == original_server && vm_info->node == 'A') && (target_server->A_remain_cpu >= cpu && target_server->A_remain_memory >= memory))
                    {
                        double rate = r1 * (target_server->A_remain_cpu - cpu) / target_server->total_cpu * target_server->daily_cost + r2 * (target_server->A_remain_memory - memory) / target_server->total_memory * target_server->daily_cost;

                        if (rate < min_rate)
                        {
                            min_rate = rate;
                            best_server = target_server;
                            which_node = 'A';
                        }
                    }
                    if (!(target_server == original_server && vm_info->node == 'B') && (target_server->B_remain_cpu >= cpu && target_server->B_remain_memory >= memory))
                    {
                        double rate = r1 * (target_server->B_remain_cpu - cpu) / target_server->total_cpu * target_server->daily_cost + r2 * (target_server->B_remain_memory - memory) / target_server->total_memory * target_server->daily_cost;

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
                    if (migration_infos.size() == max_migration_num ){
                        if(Start_rate * total_days_num <=now_day&& End_rate * total_days_num >=now_day && !isUsed){
                            isUsed = true;
                            // cout<<"Used!!!!"<<endl;
                            max_migration_num = vmIDs.size();
                        }else{
                            return migration_infos;
                        }
                    }
                }
            }
            else
            {
                PurchasedServer *original_server = vm_info->purchase_server;
                int vm_id = vm_info->vm_id;
                int cpu = vm_info->cpu;
                int memory = vm_info->memory;
                double _original_rate = (remain_rate(original_server, 'A') + remain_rate(original_server, 'B')) / 2 * original_server->daily_cost;
                double min_rate = _original_rate;
                PurchasedServer *best_server = NULL;
                for (auto &target_server : target_servers)
                { //找最合适的服务器。
                    if (target_server == original_server)
                        continue;
                    if (target_server->A_remain_cpu >= cpu && target_server->A_remain_memory >= memory && target_server->B_remain_cpu >= cpu && target_server->B_remain_memory >= memory)
                    {

                        double rate = r1 * (target_server->A_remain_cpu - cpu + target_server->B_remain_cpu - cpu) / target_server->total_cpu / 2 * target_server->daily_cost + r2 * (target_server->A_remain_memory - memory + target_server->B_remain_memory - memory) / target_server->total_memory / 2 * target_server->daily_cost;
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
                    if (migration_infos.size() == max_migration_num ){
                        if(Start_rate * total_days_num <=now_day&& End_rate * total_days_num >=now_day && !isUsed){
                            isUsed = true;
                            // cout<<"Used!!!!"<<endl;
                            max_migration_num = vmIDs.size();
                        }else{
                            return migration_infos;
                        }
                    }
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
                return min(server1->A_remain_cpu, server1->A_remain_memory) < min(server2->A_remain_cpu, server2->A_remain_memory);
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
                // return  server1->total_cpu - server1->A_remain_cpu - server1->B_remain_cpu + 2 * server1->total_memory - server1->A_remain_memory - server1->B_remain_memory < 2 * server2->total_cpu - server2->A_remain_cpu - server2->B_remain_cpu + 2 * server2->total_memory - server2->A_remain_memory - server2->B_remain_memory;
            }
            else
            {
                return false;
            }
        });
        for (auto original_server : original_servers)
        {
            if (migration_infos.size() + vm_nums(original_server) > max_migration_num) continue;
            for (auto target_server : target_servers)
            {
                if (target_server != original_server && (vm_nums(target_server) == 0 ? target_server->daily_cost : 0) <= 1.0 * original_server->daily_cost && migration_infos.size() + vm_nums(original_server) <= max_migration_num && target_server->A_remain_cpu >= original_server->total_cpu - original_server->A_remain_cpu && target_server->A_remain_memory >= original_server->total_memory - original_server->A_remain_memory && target_server->B_remain_cpu >= original_server->total_cpu - original_server->B_remain_cpu && target_server->B_remain_memory >= original_server->total_memory - original_server->B_remain_memory)
                {
                    migrate_to(original_server, 'C', target_server, 'C', migration_infos);
                    migrate_to(original_server, 'A', target_server, 'A', migration_infos);
                    migrate_to(original_server, 'B', target_server, 'B', migration_infos);
                    if (migration_infos.size() == max_migration_num ){
                        if(Start_rate * total_days_num <=now_day&& End_rate * total_days_num >=now_day && !isUsed){
                            isUsed = true;
                            // cout<<"Used!!!!"<<endl;
                            max_migration_num = vmIDs.size();
                        }else{
                            return migration_infos;
                        }
                    }
                }
                else if (target_server != original_server && (vm_nums(target_server) == 0 ? target_server->daily_cost : 0) <= 1.0 * original_server->daily_cost && migration_infos.size() + vm_nums(original_server) <= max_migration_num && target_server->A_remain_cpu >= original_server->total_cpu - original_server->B_remain_cpu && target_server->A_remain_memory >= original_server->total_memory - original_server->B_remain_memory && target_server->B_remain_cpu >= original_server->total_cpu - original_server->A_remain_cpu && target_server->B_remain_memory >= original_server->total_memory - original_server->A_remain_memory)
                {
                    migrate_to(original_server, 'C', target_server, 'C', migration_infos);
                    migrate_to(original_server, 'A', target_server, 'B', migration_infos);
                    migrate_to(original_server, 'B', target_server, 'A', migration_infos);
                    if (migration_infos.size() == max_migration_num ){
                        if(Start_rate * total_days_num <=now_day&& End_rate * total_days_num >=now_day && !isUsed){
                            isUsed = true;
                            // cout<<"Used!!!!"<<endl;
                            max_migration_num = vmIDs.size();
                        }else{
                            return migration_infos;
                        }
                    }
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
        return max(a->A_remain_cpu, a->B_remain_cpu) + max(a->A_remain_memory, a->B_remain_memory) > max(b->A_remain_cpu, b->B_remain_cpu) + max(b->A_remain_memory, b->B_remain_memory);
    }
};

pair< PurchasedServer *, char> SearchSuitPurchasedServer(int deployed_way, int cpu, int memory, bool from_open)
{
    /**
     * @description: 从已有服务器中搜寻合适服务器
     * @param {deployed_way:部署方式)(1:双节点,0:单节点);from_open: 从开机/关机的服务器中搜索 }
     * @return {合适的服务器PurchasedServer* ，没有则返回nullptr}
     */
    Cmp cmp;
    Evaluate evaluate; //创建一个评价类的实例
    PurchasedServer *flag_server = 0;
    PurchasedServer *balance_server = 0;
    bool use_Balance = true;

    if (deployed_way == 1)
    {
        if (from_open)
        {
            //从开机的服务器中搜索双节点虚拟机可以部署的服务器
            double min_remain_rate = 2.0;
            double min_balance_rate = 2.5;

            for (auto &purchase_server : purchase_servers)
            {
                if (purchase_server->A_remain_cpu >= cpu && purchase_server->A_remain_memory >= memory && purchase_server->B_remain_cpu >= cpu && purchase_server->B_remain_memory >= memory && (purchase_server->A_vm_id.size() + purchase_server->B_vm_id.size() + purchase_server->AB_vm_id.size() != 0))
                {
                    double _cpu_remain_rate = max(1.0 * (purchase_server->A_remain_cpu - cpu) / purchase_server->total_cpu, 1.0 * (purchase_server->B_remain_cpu - cpu) / purchase_server->total_cpu);
                    double _memory_remain_rate = max(1.0 * (purchase_server->A_remain_memory - memory) / purchase_server->total_memory, 1.0 * (purchase_server->B_remain_memory - memory) / purchase_server->total_memory);

                    if (use_Balance)
                    {
                        double _cpu_remain = purchase_server->A_remain_cpu - cpu + purchase_server->B_remain_cpu - cpu;
                        double _memory_remain = purchase_server->A_remain_memory - memory + purchase_server->B_remain_memory - memory;
                        double _cpu_remain_A = purchase_server->A_remain_cpu - cpu;
                        double _cpu_remain_B = purchase_server->B_remain_cpu - cpu;
                        double _memory_remain_A = purchase_server->A_remain_memory - memory;
                        double _memory_remain_B = purchase_server->B_remain_memory - memory;
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

                    if (_cpu_remain_rate + _memory_remain_rate < min_remain_rate)
                    {
                        min_remain_rate = _cpu_remain_rate + _memory_remain_rate;
                        flag_server = purchase_server;
                    }
                }
            }
            if (use_Balance)
            {
                return  make_pair (balance_server,'C');
            }
            else
                return make_pair(flag_server,'C');
        }
        else
        {
            // 从关机的服务器中搜索双节点虚拟机可以部署的服务器，按照分摊电费最低搜索
            double min_dense_cost = DBL_MAX;
            for (auto &purchase_server : purchase_servers)
            {
                if (purchase_server->A_remain_cpu >= cpu && purchase_server->A_remain_memory >= memory && purchase_server->B_remain_cpu >= cpu && purchase_server->B_remain_memory >= memory && (purchase_server->A_vm_id.size() + purchase_server->B_vm_id.size() + purchase_server->AB_vm_id.size() == 0))
                {
                    double dense_cost;
                    
                    double _cpu_rate = 1.0 * cpu / purchase_server->A_remain_cpu;
                    double _memory_rate = 1.0 * (memory) / purchase_server->A_remain_memory;
                    double use_rate = 1.0 * (_cpu_rate + _memory_rate) / 2;
                    dense_cost = 1.0 * (server_name2info[purchase_server->server_name].daily_cost) * use_rate;
                    
                    if (dense_cost < min_dense_cost)
                    {
                        min_dense_cost = dense_cost;
                        flag_server = purchase_server;
                    }
                }
            }
            return make_pair(flag_server,'C');
        }
    }else{
        vector<PurchasedServer *> can_deploy_servers;
        if(from_open){
            //选出开机的合适的服务器
            for (auto &purchase_server : purchase_servers)
            { //先筛选能用的开机服务器
                if ((purchase_server->A_remain_cpu < cpu || purchase_server->A_remain_memory < memory) && (purchase_server->B_remain_cpu < cpu || purchase_server->B_remain_memory < memory) || (vm_nums(purchase_server) == 0))
                {
                    continue;
                }
                else
                {
                    can_deploy_servers.emplace_back(purchase_server);
                    if (purchase_server->A_remain_cpu >= cpu && purchase_server->A_remain_memory >= memory)
                    {
                        purchase_server->can_deploy_A = true;
                    }
                    if (purchase_server->B_remain_cpu >= cpu && purchase_server->B_remain_memory >= memory)
                    {
                        purchase_server->can_deploy_B = true;
                    }
                }
            }
            double min_remain_rate = DBL_MAX;
            PurchasedServer *flag_server = 0;
            char which_node = 'C';
            for (auto &purchase_server : can_deploy_servers)
            {
                bool evaluate_A = evaluate.PurchasedServerA(purchase_server, cpu, memory);
                bool evaluate_B = evaluate.PurchasedServerB(purchase_server, cpu, memory);

                if (evaluate_A && evaluate_B)
                {
                    double _cpu_remain_rate = 1.0 * (purchase_server->A_remain_cpu - cpu) / purchase_server->total_cpu;
                    double _memory_remain_rate = 1.0 * (purchase_server->A_remain_memory - memory) / purchase_server->total_memory;

                    if (2 * max(_cpu_remain_rate, _memory_remain_rate) < min_remain_rate)
                    // if ((_cpu_remain_rate + _memory_remain_rate) < min_remain_rate)
                    {
                        // min_remain_rate = _cpu_remain_rate + _memory_remain_rate;
                        min_remain_rate = 2 * max(_cpu_remain_rate, _memory_remain_rate) ;
                        flag_server = purchase_server;
                        which_node = 'A';
                    }

                    _cpu_remain_rate = 1.0 * (purchase_server->B_remain_cpu - cpu) / purchase_server->total_cpu;
                    _memory_remain_rate = 1.0 * (purchase_server->B_remain_memory - memory) / purchase_server->total_memory;
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
                    double _cpu_remain_rate = 1.0 * (purchase_server->A_remain_cpu - cpu) / purchase_server->total_cpu;
                    double _memory_remain_rate = 1.0 * (purchase_server->A_remain_memory - memory) / purchase_server->total_memory;
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
                    double _cpu_remain_rate = 1.0 * (purchase_server->B_remain_cpu - cpu) / purchase_server->total_cpu;
                    double _memory_remain_rate = 1.0 * (purchase_server->B_remain_memory - memory) / purchase_server->total_memory;
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
            if (min_remain_rate != DBL_MAX)
            {
                return make_pair(flag_server,which_node);
            }
        }else{
            for (auto &purchase_server : purchase_servers)
            { //先筛选能用的服务器
                if ((purchase_server->A_remain_cpu < cpu || purchase_server->A_remain_memory < memory) && (purchase_server->B_remain_cpu < cpu || purchase_server->B_remain_memory < memory) || (vm_nums(purchase_server) != 0))
                {
                    continue;
                }
                else
                {
                    can_deploy_servers.emplace_back(purchase_server);
                    if (purchase_server->A_remain_cpu >= cpu && purchase_server->A_remain_memory >= memory)
                    {
                        purchase_server->can_deploy_A = true;
                    }
                    if (purchase_server->B_remain_cpu >= cpu && purchase_server->B_remain_memory >= memory)
                    {
                        purchase_server->can_deploy_B = true;
                    }
                }
            }
            double min_remain_rate = DBL_MAX;
            double min_cost = DBL_MAX;
            PurchasedServer* flag_server = 0;
            char which_node = 'C';
            for (auto &purchase_server : can_deploy_servers)
            {
                double _cpu_rate = 1.0 * (cpu / purchase_server->total_cpu);
                double _memory_rate = 1.0 * (memory / purchase_server->total_memory);
                double use_rate = (_cpu_rate + _memory_rate) / 2;
                double cost = purchase_server->daily_cost * use_rate;
                if (cost < min_cost)
                {
                    min_cost = cost;
                    flag_server = purchase_server;
                    which_node = 'A';
                }
            }
            if(min_cost!=DBL_MAX){
                return make_pair(flag_server,which_node);
            }
        }
    }
    
    
    return make_pair( flag_server,'C');
}

void DeployOnServer(PurchasedServer *flag_server, int deployment_way,char whichNode, int cpu, int memory, int vm_id, string &vm_name)
{
    /**
     * @description: 向某服务器部署虚拟机
     * @param {flag_server: 目标服务器}
     * @return {*}
     */
    if (deployment_way == 1)
    {
        // 部署双节点虚拟机到指定服务器
        if (flag_server->A_vm_id.size() + flag_server->B_vm_id.size() + flag_server->AB_vm_id.size() == 0)
        {
            from_off_2_start.insert(flag_server->server_id);
        }
        flag_server->A_remain_cpu -= cpu;
        flag_server->A_remain_memory -= memory;
        flag_server->B_remain_cpu -= cpu;
        flag_server->B_remain_memory -= memory;
        flag_server->AB_vm_id.insert(vm_id);

        VmIdInfo vm_id_info;
        vm_id_info.purchase_server = flag_server;
        vm_id_info.vm_name = vm_name;
        vm_id_info.cpu = cpu;
        vm_id_info.memory = memory;
        vm_id_info.node = 'C';
        vm_id_info.vm_id = vm_id;
        
        vm_id2info[vm_id] = vm_id_info;
    }else{
        if(whichNode == 'A'){
            flag_server->A_remain_cpu -= cpu;
            flag_server->A_remain_memory -= memory;
            flag_server->A_vm_id.insert(vm_id);

            VmIdInfo vm_id_info;
            vm_id_info.purchase_server = flag_server;
            vm_id_info.vm_name = vm_name;
            vm_id_info.cpu = cpu;
            vm_id_info.memory = memory;
            vm_id_info.node = 'A';
            vm_id_info.vm_id = vm_id;
            vm_id2info[vm_id] = vm_id_info;
        }else if(whichNode == 'B'){
            flag_server->B_remain_cpu -= cpu;
            flag_server->B_remain_memory -= memory;
            flag_server->B_vm_id.insert(vm_id);

            VmIdInfo vm_id_info;
            vm_id_info.purchase_server = flag_server;
            vm_id_info.vm_name = vm_name;
            vm_id_info.cpu = cpu;
            vm_id_info.memory = memory;
            vm_id_info.node = 'B';
            vm_id_info.vm_id = vm_id;
            vm_id2info[vm_id] = vm_id_info;

        }
    }
}

void SimulateDeployOnServer(PurchasedServer *flag_server, int deployment_way,char whichNode, int cpu, int memory, int vm_id, string &vm_name)
{
    /**
     * @description: 向某服务器部署虚拟机
     * @param {flag_server: 目标服务器}
     * @return {*}
     */
    if (deployment_way == 1)
    {
        // 部署双节点虚拟机到指定服务器
        if (flag_server->A_vm_id.size() + flag_server->B_vm_id.size() + flag_server->AB_vm_id.size() == 0)
        {
            from_off_2_start.insert(flag_server->server_id);
        }
        flag_server->A_remain_cpu -= cpu;
        flag_server->A_remain_memory -= memory;
        flag_server->B_remain_cpu -= cpu;
        flag_server->B_remain_memory -= memory;
        // flag_server->AB_vm_id.insert(vm_id);


    }else{
        if(whichNode == 'A'){
            flag_server->A_remain_cpu -= cpu;
            flag_server->A_remain_memory -= memory;
            // flag_server->A_vm_id.insert(vm_id);

          
        }else if(whichNode == 'B'){
            flag_server->B_remain_cpu -= cpu;
            flag_server->B_remain_memory -= memory;
            // flag_server->B_vm_id.insert(vm_id);

          
        }
    }
}


void BuyAndDeployTwoVM(string vm_1_name, string vm_2_name, int vmID_1, int vmID_2, string serverName)
{
/**
 * @description: (连续Add绑定购买中使用)购买指定服务器，并且将两台虚拟机部署在刚刚购买的服务器上
 * @param {deployedway : 1 -- 前后两台服务器均为双节点部署}
 * @return {*}
 */
    total_server_cost += server_name2info[serverName].hardware_cost;

    // 购买服务器
    PurchasedServer *purchase_server = new PurchasedServer;
    purchase_server->total_cpu = server_name2info[serverName].cpu;
    purchase_server->total_memory = server_name2info[serverName].memory;
    purchase_server->A_remain_cpu = server_name2info[serverName].cpu;
    purchase_server->A_remain_memory = server_name2info[serverName].memory;
    purchase_server->B_remain_cpu = server_name2info[serverName].cpu;
    purchase_server->daily_cost = server_name2info[serverName].daily_cost;
    purchase_server->B_remain_memory = server_name2info[serverName].memory;
    purchase_server->server_name = server_name2info[serverName].server_name;
    int left_day = total_days_num - now_day +1;
    purchase_server->hardware_avg_cost = 1.0*  server_name2info[serverName].hardware_cost / left_day / (2.0 * server_name2info[serverName].cpu * 2.3 +  2.0* server_name2info[serverName].memory);
    purchase_servers.emplace_back(purchase_server);
    purchase_infos[serverName].emplace_back(purchase_server);
    UpdateHardwareCost(purchase_server->server_name);

    if(vm_name2info[vm_1_name].deployment_way == 1 && vm_name2info[vm_2_name].deployment_way == 1){
        //前后两台虚拟机均为双节点部署的虚拟机
        // 修改部署后目的服务器信息
        purchase_server->A_remain_cpu -= vm_name2info[vm_1_name].cpu;
        purchase_server->B_remain_cpu -= vm_name2info[vm_1_name].cpu;
        purchase_server->A_remain_cpu -= vm_name2info[vm_2_name].cpu;
        purchase_server->B_remain_cpu -= vm_name2info[vm_2_name].cpu;
        purchase_server->A_remain_memory -= vm_name2info[vm_1_name].memory;
        purchase_server->B_remain_memory -= vm_name2info[vm_1_name].memory;
        purchase_server->A_remain_memory -= vm_name2info[vm_2_name].memory;
        purchase_server->B_remain_memory -= vm_name2info[vm_2_name].memory;
        purchase_server->AB_vm_id.insert(vmID_1);
        purchase_server->AB_vm_id.insert(vmID_2);

        //修改部署后虚拟机信息
        vmIDs.erase(vmID_1);
        vmIDs.erase(vmID_2);

        VmIdInfo vm_id_info;
        vm_id_info.purchase_server = purchase_server;
        vm_id_info.vm_name = vm_1_name;
        vm_id_info.cpu = vm_name2info[vm_1_name].cpu;
        vm_id_info.memory = vm_name2info[vm_1_name].memory;
        vm_id_info.node = 'C';
        vm_id_info.vm_id = vmID_1;
        vm_id2info[vmID_1] = vm_id_info;
        vmIDs.insert(vmID_1);

        VmIdInfo vm_id_info_2;
        vm_id_info_2.purchase_server = purchase_server;
        vm_id_info_2.vm_name = vm_2_name;
        vm_id_info_2.cpu = vm_name2info[vm_2_name].cpu;
        vm_id_info_2.memory = vm_name2info[vm_2_name].memory;
        vm_id_info_2.node = 'C';
        vm_id_info_2.vm_id = vmID_2;
        vm_id2info[vmID_2] = vm_id_info_2;
        vmIDs.insert(vmID_2);
    }
    else if(vm_name2info[vm_1_name].deployment_way == 0 && vm_name2info[vm_2_name].deployment_way == 0 ){
        //前后两台虚拟机均为单节点的虚拟机部署

        purchase_server->A_remain_cpu -= vm_name2info[vm_1_name].cpu;
        purchase_server->B_remain_cpu -= vm_name2info[vm_2_name].cpu;
        purchase_server->A_remain_memory -= vm_name2info[vm_1_name].memory;
        purchase_server->B_remain_memory -= vm_name2info[vm_2_name].memory;
        purchase_server->A_vm_id.insert(vmID_1);
        purchase_server->B_vm_id.insert(vmID_2);

        vmIDs.erase(vmID_1);
        vmIDs.erase(vmID_2);

        VmIdInfo vm_id_info;
        vm_id_info.purchase_server = purchase_server;
        vm_id_info.vm_name = vm_1_name;
        vm_id_info.cpu = vm_name2info[vm_1_name].cpu;
        vm_id_info.memory = vm_name2info[vm_1_name].memory;
        vm_id_info.node = 'A';
        vm_id_info.vm_id = vmID_1;
        vm_id2info[vmID_1] = vm_id_info;
        vmIDs.insert(vmID_1);

        VmIdInfo vm_id_info_2;
        vm_id_info_2.purchase_server = purchase_server;
        vm_id_info_2.vm_name = vm_2_name;
        vm_id_info_2.cpu = vm_name2info[vm_2_name].cpu;
        vm_id_info_2.memory = vm_name2info[vm_2_name].memory;
        vm_id_info_2.node = 'B';
        vm_id_info_2.vm_id = vmID_2;
        vm_id2info[vmID_2] = vm_id_info_2;
        vmIDs.insert(vmID_2);
    }
}

PurchasedServer *BuyNewServer(int deployment_way, int cpu, int memory)
{
    /**
     * @description: 购买新服务器并加入已购序列中
     * @param {*}
     * @return {刚刚购买的服务器PurchasedServer*}
     */
    SoldServer *flag_sold_server;
    double min_dense_cost = DBL_MAX;

    if (deployment_way == 1)
    {
        for (auto &sold_server : sold_servers)
        {
            if (sold_server.cpu >= cpu && sold_server.memory >= memory)
            {
                double dense_cost;
                if (isDenseBuy == 1)
                {
                    // double use_rate = (1.0 *(cpu) / sold_server.cpu + 1.0 *(memory) / sold_server.memory) / 2;
                    // double use_rate = max(1.0 *(cpu) / sold_server.cpu , 1.0 *(memory) / sold_server.memory) ;
                    double _cpu_rate = 1.0 * cpu / sold_server.cpu;
                    double _memory_rate = 1.0 * (memory) / sold_server.memory;
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
    }else{
        for (auto &sold_server : sold_servers)
        {
            if (sold_server.cpu >= cpu && sold_server.memory >= memory)
            {
                double dense_cost;
                if (true)
                {
                    double _cpu_rate = 1.0 * cpu / sold_server.cpu;
                    double _memory_rate = 1.0 * (memory) / sold_server.memory;
                    double use_rate = 0.5 * (_cpu_rate + _memory_rate);
                    dense_cost = 1.0 * (sold_server.hardware_cost + sold_server.daily_cost * (total_days_num - now_day)) * use_rate;
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
    purchase_server->total_cpu = flag_sold_server->cpu;
    purchase_server->total_memory = flag_sold_server->memory;
    purchase_server->A_remain_cpu = flag_sold_server->cpu;
    purchase_server->A_remain_memory = flag_sold_server->memory;
    purchase_server->B_remain_cpu = flag_sold_server->cpu;
    purchase_server->daily_cost = flag_sold_server->daily_cost;
    purchase_server->B_remain_memory = flag_sold_server->memory;
    purchase_server->server_name = flag_sold_server->server_name;
    int left_day = total_days_num - now_day+1;
    purchase_server->hardware_avg_cost = 1.0*  flag_sold_server->hardware_cost / left_day / (2.0 * flag_sold_server->cpu*2.3 +  2.0* flag_sold_server->memory);

    purchase_servers.emplace_back(purchase_server);
    purchase_infos[flag_sold_server->server_name].emplace_back(purchase_server);
    UpdateHardwareCost(purchase_server->server_name);
    return purchase_server;
}

SoldServer *SearchNewServer(int deployment_way, int cpu, int memory)
{
    /**
     * @description: 购买新服务器并加入已购序列中
     * @param {*}
     * @return {刚刚购买的服务器PurchasedServer*}
     */
    SoldServer *flag_sold_server;
    double min_dense_cost = DBL_MAX;

    if (deployment_way == 1)
    {
        for (auto &sold_server : sold_servers)
        {
            if (sold_server.cpu >= cpu && sold_server.memory >= memory)
            {
                double dense_cost;
                if (isDenseBuy == 1)
                {
                    // double use_rate = (1.0 *(cpu) / sold_server.cpu + 1.0 *(memory) / sold_server.memory) / 2;
                    // double use_rate = max(1.0 *(cpu) / sold_server.cpu , 1.0 *(memory) / sold_server.memory) ;
                    double _cpu_rate = 1.0 * cpu / sold_server.cpu;
                    double _memory_rate = 1.0 * (memory) / sold_server.memory;
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
    }else{
        for (auto &sold_server : sold_servers)
        {
            if (sold_server.cpu >= cpu && sold_server.memory >= memory)
            {
                double dense_cost;
                if (true)
                {
                    double _cpu_rate = 1.0 * cpu / sold_server.cpu;
                    double _memory_rate = 1.0 * (memory) / sold_server.memory;
                    double use_rate = 0.5 * (_cpu_rate + _memory_rate);
                    dense_cost = 1.0 * (sold_server.hardware_cost + sold_server.daily_cost * (total_days_num - now_day)) * use_rate;
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

string AddVm(AddData &add_data)
{
    Cmp cmp;
    bool deployed = false;
    Evaluate evaluate; //创建一个评价类的实例
    int deployment_way = add_data.deployment_way;
    vmIDs.insert(add_data.vm_id);

    if (deployment_way == 1)
    {   //双节点部署
        int cpu = add_data.cpu;
        int memory = add_data.memory;

        PurchasedServer *flag_server = 0;
        flag_server = SearchSuitPurchasedServer(1, cpu, memory, true).first;

        if (flag_server != 0)
        {
            //从开机的服务器中选到了服务器
            deployed = true;
            DeployOnServer(flag_server, 1, 'C',cpu, memory, add_data.vm_id, add_data.vm_name);
        }

        PurchasedServer *flag_off_server = 0;
        if (!deployed)
        {
            flag_off_server = SearchSuitPurchasedServer(1, cpu, memory, false).first;
        }

        if (!deployed && flag_off_server != nullptr)
        {
            //从关机的机器里找
            deployed = true;
            DeployOnServer(flag_off_server, 1, 'C',cpu, memory, add_data.vm_id, add_data.vm_name);
        }

        //购买新服务器
        PurchasedServer *newServer = 0;
        if (!deployed)
        {
            newServer = BuyNewServer(1, cpu, memory);
            DeployOnServer(newServer, 1,'C', cpu, memory, add_data.vm_id, add_data.vm_name);
            deployed = true;
            return newServer->server_name;
        }
    }
    else
    { //单节点部署
        int cpu = add_data.cpu;
        int memory = add_data.memory;
        PurchasedServer* flag_server = 0;
        char which_node = 'C';

        pair<PurchasedServer*,char> res = SearchSuitPurchasedServer(0,cpu,memory,true);
        flag_server = res.first;
        which_node = res.second;

        if (flag_server != 0)
        {
            //在开机的服务器中找到了合适的放置位置
            if (flag_server->A_vm_id.size() + flag_server->B_vm_id.size() + flag_server->AB_vm_id.size() == 0)
            {
                from_off_2_start.insert(flag_server->server_id);
            }
            if (which_node == 'A')
            {
                deployed = true;
                DeployOnServer(flag_server,0,'A',cpu,memory,add_data.vm_id,add_data.vm_name);
            }
            else if (which_node == 'B')
            {
                deployed = true;
                DeployOnServer(flag_server,0,'B',cpu,memory,add_data.vm_id,add_data.vm_name);
            }
        }
        else
        {
            //在开机的服务器中未找到，在关机的服务器中找
            pair<PurchasedServer*,char> res = SearchSuitPurchasedServer(0,cpu,memory,false);
            flag_server = res.first;
            which_node = res.second;

            if (flag_server != 0)
            {
                //代表从关机的服务器中选取的
                if (flag_server->A_vm_id.size() + flag_server->B_vm_id.size() + flag_server->AB_vm_id.size() == 0)
                {
                    from_off_2_start.insert(flag_server->server_id);
                }
                deployed = true;
                DeployOnServer(flag_server,0,'A',cpu,memory,add_data.vm_id,add_data.vm_name);
            }
        }

        //购买新服务器并部署
        if(!deployed){
            PurchasedServer* new_buy_server = BuyNewServer(0,cpu,memory);
            deployed = true;
            DeployOnServer(new_buy_server,0,'A',cpu,memory,add_data.vm_id,add_data.vm_name);
            return new_buy_server->server_name;
        }
    }
    return "";
}

void DeleteVm(int vm_id)
{
    /**
     * @description: 按id删除指定虚拟机
     * @param {*}
     * @return {*}
     */
    VmIdInfo vm_info = vm_id2info[vm_id];
    PurchasedServer *purchase_server = vm_info.purchase_server;
    char node = vm_info.node;
    int cpu = vm_info.cpu;
    int memory = vm_info.memory;
    if (node == 'A')
    {
        purchase_server->A_remain_cpu += cpu;
        purchase_server->A_remain_memory += memory;
        purchase_server->A_vm_id.erase(vm_id);
    }
    else if (node == 'B')
    {
        purchase_server->B_remain_cpu += cpu;
        purchase_server->B_remain_memory += memory;
        purchase_server->B_vm_id.erase(vm_id);
    }
    else if (node == 'C')
    {
        purchase_server->A_remain_cpu += cpu;
        purchase_server->A_remain_memory += memory;
        purchase_server->B_remain_cpu += cpu;
        purchase_server->B_remain_memory += memory;
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
            _total_cpu += server_name2info[_serverName].cpu * 2;
            _total_memory += server_name2info[_serverName].memory * 2;
        }
    }
    else
    {
        for (auto &purchase_server : purchase_servers)
        {
            _total_cpu += purchase_server->A_remain_cpu + purchase_server->B_remain_cpu;
            _total_memory += purchase_server->A_remain_memory + purchase_server->B_remain_memory;
            // _total_cpu +=  max(purchase_server->A_remain_cpu , purchase_server->B_remain_cpu);
            // _total_memory += max(purchase_server->A_remain_memory , purchase_server->B_remain_memory);
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
            vm_cpu = vm_name2info[vm_name].cpu * 2;
            vm_memory = vm_name2info[vm_name].memory * 2;
        }
        else
        {
            vm_cpu = vm_name2info[vm_name].cpu;
            vm_memory = vm_name2info[vm_name].memory;
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
            vm_cpu = vm_name2info[vm_name].cpu * 2;
            vm_memory = vm_name2info[vm_name].memory * 2;
        }
        else
        {
            vm_cpu = vm_name2info[vm_name].cpu;
            vm_memory = vm_name2info[vm_name].memory;
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
                vm_cpu = vm_name2info[vm_name].cpu * 2;
                vm_memory = vm_name2info[vm_name].memory * 2;
            }
            else
            {
                vm_cpu = vm_name2info[vm_name].cpu;
                vm_memory = vm_name2info[vm_name].memory;
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


SoldServer *SearchForContinueVM(int deployment_way, int cpu, int memory)
{
    /**
     * '
     * @description: 为合并的机器找虚拟机
     * @param {*}
     * @return {SoldServer*}
     */
    SoldServer *flag_sold_server = 0;
    double min_dense_cost = DBL_MAX;
    if (deployment_way == 1)
    {
        for (auto &sold_server : sold_servers)
        {
            if (sold_server.cpu >= cpu && sold_server.memory >= memory)
            {
                double dense_cost;
                if (isDenseBuy == 1)
                {
                    double _cpu_rate = 1.0 * cpu / sold_server.cpu;
                    double _memory_rate = 1.0 * (memory) / sold_server.memory;
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
    /**
     * @description: （连续购买时使用，回退某台虚拟机的放置）
     * @param {*}
     * @return {*}
     */
    string server_name = vm_id2info[vmID].purchase_server->server_name;
    total_server_cost -= server_name2info[server_name].hardware_cost;

    UpdateHardwareCost(server_name,true);
    purchase_servers.erase(purchase_servers.end() - 1, purchase_servers.end());
    purchase_infos[server_name].erase(purchase_infos[server_name].end() - 1, purchase_infos[server_name].end());
    if (purchase_infos[server_name].size() == 0)
    {
        purchase_infos.erase(server_name);
    }
    // vm_id2info.erase(vmID);
    vmIDs.erase(vmID);
}



double CaculateTotalCost(string vmName,int day){
    int deployedment_way = vm_name2info[vmName].deployment_way;
    int cpu = vm_name2info[vmName].cpu;
    int memory = vm_name2info[vmName].memory;


    //从开机的机器里找有无可以容纳该请求的地方
    pair<PurchasedServer*,char> searchRes =  SearchSuitPurchasedServer(deployedment_way,cpu,memory,true);
    if(searchRes.first == 0){
        searchRes = SearchSuitPurchasedServer(deployedment_way,cpu,memory,false);
        if(searchRes.first == 0 ){
            //系统内无合适的服务器，需要购买新的服务器
            SoldServer* newServer = SearchNewServer(deployedment_way,cpu,memory);

            double new_server_hardware_cost_per_day_per_resource = 1.0 * newServer->hardware_cost / (total_days_num - now_day) /(2* newServer->cpu + 2* newServer->memory);
            double new_server_power_cost_per_day_per_resource = 1.0 * newServer->daily_cost / (2* newServer->cpu + 2* newServer->memory);
            if(now_day < 2.0 / 3 * total_days_num){
                return (new_server_hardware_cost_per_day_per_resource+new_server_power_cost_per_day_per_resource) * (day == 0 ?1:day) * (cpu + memory) *(deployedment_way +1);
            }else{
                return (new_server_hardware_cost_per_day_per_resource+new_server_power_cost_per_day_per_resource) * (day == 0 ?1:day) * (cpu + memory) *(deployedment_way +1) * 100;
            }
        }else{
            return (power_cost_per_day_per_resource+hardware_cost_per_day_per_resource) * (day == 0 ?1:day) * (cpu + memory) *(deployedment_way +1) *1;
        }
    }else{
        vector<int> allResouceAfterMigration = GetAllResourceOfOwnServers(true);
        vector<int> allResourceNeed = GetAllResourceOfToday(intraday_requests);
        if(allResouceAfterMigration[0] >2.0* allResourceNeed[0] &&allResouceAfterMigration[1] >2.0*allResourceNeed[1] ){
            return (power_cost_per_day_per_resource+hardware_cost_per_day_per_resource) * (day == 0 ?1:day) * (cpu + memory) *(deployedment_way +1);
        }
        else{
            searchRes = SearchSuitPurchasedServer(deployedment_way,cpu,memory,false);
            if(searchRes.first == 0 ){
                //系统内无合适的服务器，需要购买新的服务器
                SoldServer* newServer = SearchNewServer(deployedment_way,cpu,memory);
                
                double new_server_hardware_cost_per_day_per_resource = 1.0 * newServer->hardware_cost / (total_days_num - now_day) /(2* newServer->cpu + 2* newServer->memory);
                double new_server_power_cost_per_day_per_resource = 1.0 * newServer->daily_cost / (2* newServer->cpu + 2* newServer->memory);
                if(now_day < 2.0 / 3 * total_days_num){
                    return (new_server_hardware_cost_per_day_per_resource+new_server_power_cost_per_day_per_resource) * (day == 0 ?1:day) * (cpu + memory) *(deployedment_way +1);
                }else{
                    return (new_server_hardware_cost_per_day_per_resource+new_server_power_cost_per_day_per_resource) * (day == 0 ?1:day) * (cpu + memory) *(deployedment_way +1) * 100;
                }
                
            }else{
                return (power_cost_per_day_per_resource+hardware_cost_per_day_per_resource) * (day == 0 ?1:day) * (cpu + memory) *(deployedment_way +1) ;
            }
        }
    }
    return (power_cost_per_day_per_resource+hardware_cost_per_day_per_resource) * (day == 0 ?1:day) * (cpu + memory) *(deployedment_way +1) ;
}

/**
 * @brief 此函数用于计算我方对虚拟机的报价。
 * @param {RequestData} request 某台虚拟机的请求数据。
 * @return {int} 报价。
 */
int CalculateMyOffer(RequestData& request) {
    double global_cost = CaculateTotalCost(request.vm_name, request.duration);

    if (global_cost < request.user_offer) {
        //可以接
        int myOffer = int( (global_cost + request.user_offer) / 2);
        //分析对手
        if(myOffer < statistics[request.vm_name].first){
            return myOffer;
        }else{
            if(request.user_offer > rival_lower_bounds[request.vm_name]){
                return request.user_offer;
            }else{
                return int(statistics[request.vm_name].first * (0.95));
            }
        }
    } else {
        return -1;
    }
}

struct ReqCmp
{
    bool operator()(RequestData a, RequestData b) 
    {
        return a.vm_id < b.vm_id;
    }
};


int SimulateDeploy(RequestData& req){
    double a = 1.0;
    double b = 1.0;
    double over_rate =1.0;
    string vm_name = req.vm_name;
    int deployment_way = vm_name2info[vm_name].deployment_way;
    int cpu = vm_name2info[vm_name].cpu;
    int memory = vm_name2info[vm_name].memory;
    

    pair< PurchasedServer *, char> res = SearchSuitPurchasedServer(deployment_way,cpu,memory,true);
    if(res.first != 0){
        //开机的里面找到了合适的
        double total_used_resource = req.duration * (deployment_way+1) * (cpu * 2.3+ memory);
        double power_cost_perresource = 1.0 * res.first->daily_cost /  (2.0 * res.first->total_cpu * 2.3 + 2.0 * res.first->total_memory);

        int total_cost = total_used_resource * (a* power_cost_perresource + b* res.first->hardware_avg_cost);
        // int total_cost = total_used_resource * (power_cost_perresource + res.first->hardware_avg_cost);
        if(total_cost  <= over_rate* req.user_offer ){
            SimulateDeployOnServer(res.first,deployment_way,res.second,cpu,memory,req.vm_id,vm_name);
            return (int)(total_cost);
        }else{
            return -1;
        }
        
    }
    else{
        //开机的里面没找到
        res = SearchSuitPurchasedServer(deployment_way,cpu,memory,false);
        if(res.first !=0 ){
            double total_used_resource = req.duration * (deployment_way+1) * (cpu*2.3+ memory);
            double power_cost_perresource = 1.0 * res.first->daily_cost /  (2.0 * res.first->total_cpu * 2.3 + 2.0 * res.first->total_memory);
            int total_cost = total_used_resource * (a*power_cost_perresource +b* res.first->hardware_avg_cost) ;
            if(total_cost <= over_rate* req.user_offer){
                SimulateDeployOnServer(res.first,deployment_way,res.second,cpu,memory,req.vm_id,vm_name);
                return (int)(total_cost);
            }else{
                return -1;
            }
        }else{
            //新买服务器
            // if(now_day > 2.5 / 5 * total_days_num) return -1;
            SoldServer* suitServer = SearchNewServer(deployment_way,cpu,memory);
            int left_day = total_days_num - now_day +1;
            double hardware_cost_perday_perresource = 1.0 * suitServer->hardware_cost / left_day / (2.0 * suitServer->cpu * 2.3 + 2.0 * suitServer->memory);
            double power_cost_perresource = 1.0 * suitServer->daily_cost /  (2.0 * suitServer->cpu * 2.3 + 2.0 * suitServer->memory);
            double total_used_resource = req.duration * (deployment_way + 1) * (cpu * 2.3 + memory) ; 
            int total_cost = total_used_resource * (b*hardware_cost_perday_perresource +a* power_cost_perresource);
            if(total_cost   > req.user_offer )
            // if(false )
            {
                return -1;
            }else{
                // PurchasedServer *purchase_server = new PurchasedServer;
                // purchase_server->total_cpu = suitServer->cpu;
                // purchase_server->total_memory = suitServer->memory;
                // purchase_server->A_remain_cpu = suitServer->cpu;
                // purchase_server->A_remain_memory = suitServer->memory;
                // purchase_server->B_remain_cpu = suitServer->cpu;
                // purchase_server->daily_cost = suitServer->daily_cost;
                // purchase_server->B_remain_memory = suitServer->memory;
                // purchase_server->server_name = suitServer->server_name;
                // purchase_server->hardware_avg_cost = hardware_cost_perday_perresource;

                // purchase_servers.emplace_back(purchase_server);
                // purchase_infos[suitServer->server_name].emplace_back(purchase_server);
                // SimulateDeployOnServer(purchase_server,deployment_way, deployment_way == 0? 'A':'C' ,cpu,memory,req.vm_id,req.vm_name  );
                return ( int)(total_cost);
            }
        }
    }

}

/**
 * @brief 输出自己的报价，返回add请求数量。
 * @param {vector<RequestData>} intraday_requests 当天所有的请求数据。
 * @return {int} add的请求数量，用于下一步读取竞争信息。
 */
int GiveMyOffers(vector<RequestData>& intraday_requests) {

    vector<PurchasedServer*> temp_purchase_servers;
    for(auto purchase_server:purchase_servers){
        PurchasedServer *temp = new PurchasedServer(*purchase_server);
        temp_purchase_servers.emplace_back(temp);
    }



    vector<RequestData> addRequests;
    for(auto& intraday_request : intraday_requests){

        if(intraday_request.operation== "add"){
            addRequests.emplace_back(intraday_request);
        }
    }

    //按照请求的单位利润率排序
    sort(addRequests.begin(),addRequests.end(),[](RequestData& a, RequestData& b){
        return 1.0 * a.user_offer / (a.duration) / ( (vm_name2info[a.vm_name].deployment_way+1) * (vm_name2info[a.vm_name].cpu + vm_name2info[a.vm_name].memory) ) > 1.0 * b.user_offer / (b.duration) / ( (vm_name2info[b.vm_name].deployment_way+1) * (vm_name2info[b.vm_name].cpu + vm_name2info[b.vm_name].memory) );
    });
    
    // set<RequestData,ReqCmp> suitReqs;
    // suitReqs.insert(addRequests.begin(),addRequests.begin() + addRequests.size()* (0.6)  );
    // suitReqs.insert(addRequests.begin(),addRequests.end() );

    for(auto& request : intraday_requests){
        long long my_offer = -1;
        if(request.operation == "add"){
            int cal_cost = SimulateDeploy(request);
            if(cal_cost == -1){
                my_offer = -1;
            }else{
                my_offer = (0.05 + 0.3 *(now_day - 1) / total_days_num)   * (request.user_offer - cal_cost)+cal_cost;
            }
            cout << my_offer << endl;
    
            my_offers[request.vm_name].emplace_back(my_offer);
        }
        
    }

    for(int i = 0;i<temp_purchase_servers.size();i++){
        *(purchase_servers[i]) = *(temp_purchase_servers[i]);
    }

    return addRequests.size();

   
}

void SolveProblem()
{
    Cmp cmp;

    // sort(sold_servers.begin(), sold_servers.end(), [](SoldServer& server1,SoldServer& server2){
    //         return server1.hardware_cost+(total_days_num - now_day) * server1.daily_cost < server2.hardware_cost+(total_days_num - now_day) * server2.daily_cost;
    //     });
    for (int i = 0; i < total_days_num; ++i)
    {
        now_day = i + 1;

#ifdef PRINTINFO
        // if (now_day % 200 == 0)
            cout << now_day << endl;
#endif

        from_off_2_start.erase(from_off_2_start.begin(), from_off_2_start.end());
        intraday_requests = request_datas.front();
        
        int add_nums = GiveMyOffers(intraday_requests);
        vector<pair<int, int>> compete_infos = ParseCompeteInfo(add_nums);
        intraday_requests = ParseBidingRes(compete_infos, intraday_requests);

        request_datas.pop();

        // vector<MigrationInfo> migration_infos;
        vector<MigrationInfo> migration_infos = Migration();
        total_migration_num += migration_infos.size();
        //获取迁移之后的系统可以提供的总资源
        vector<int> allResouceAfterMigration = GetAllResourceOfOwnServers(true);

        //获取未来N条请求所需要的总资源
        // allResourceOfNReqs = GetAllResourceOfFutureNDays(300);
        // cout<<allResourceOfNReqs[0] << "  "<<allResourceOfNReqs[1]<<endl;

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
        double _rate = 1.2;
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
                    int cpu = sold_vm.cpu;
                    int memory = sold_vm.memory;
                    int vm_id = intraday_requests[j].vm_id;
                    continuous_add_datas.emplace_back(deployment_way, cpu, memory, vm_id, vm_name);
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
                            int cpu = last_add_data.cpu + add_data.cpu;
                            int memory_cores = last_add_data.memory + add_data.memory;

                            SoldServer *suitServer = 0;
                            if (memory_cores > cpu)
                            {
                                suitServer = SearchForContinueVM(1, cpu, memory_cores);
                            }

                            if (suitServer != 0)
                            {
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
                            int cpu = max(last_add_data.cpu, add_data.cpu);
                            int memory_cores = max(last_add_data.memory, add_data.memory);
                            SoldServer *suitServer = SearchForContinueVM(1, cpu, memory_cores);
                            if (suitServer != 0)
                            {
                                int new_cost = suitServer->hardware_cost + (total_days_num - now_day) * suitServer->daily_cost;
                                int old_cost = server_name2info[last_buy_server_name].hardware_cost + (total_days_num - now_day) * server_name2info[last_buy_server_name].daily_cost + server_name2info[buy_server_name].hardware_cost + (total_days_num - now_day) * server_name2info[buy_server_name].daily_cost;
                                if (new_cost <  old_cost)
                                {
                                    count_continue_buy++;
                                    revokeBuy(add_data.vm_id);
                                    revokeBuy(last_add_data.vm_id);
                                    BuyAndDeployTwoVM(last_add_data.vm_name, add_data.vm_name, last_add_data.vm_id, add_data.vm_id, suitServer->server_name);
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
                    int cpu = sold_vm.cpu;
                    int memory = sold_vm.memory;
                    int vm_id = intraday_requests[j].vm_id;
                    continuous_add_datas.emplace_back(deployment_way, cpu, memory, vm_id, vm_name);
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
                bool isSuccess = false;
                if (last_buy_server_name != "" && buy_server_name != "")
                {
                    //连续两次购买
                    if (last_add_data.deployment_way == 1 && add_data.deployment_way == 1)
                    {
                        int cpu = last_add_data.cpu + add_data.cpu;
                        int memory_cores = last_add_data.memory + add_data.memory;

                        SoldServer *suitServer = 0;
                        if (memory_cores > cpu)
                        {
                            suitServer = SearchForContinueVM(1, cpu, memory_cores);
                        }

                        if (suitServer != 0)
                        {
                            int new_cost = suitServer->hardware_cost;
                            int old_cost = server_name2info[last_buy_server_name].hardware_cost + server_name2info[buy_server_name].hardware_cost;
                            if (new_cost < 2.0 / 3 * old_cost)
                            {
                                count_continue_buy++;
                                revokeBuy(add_data.vm_id);
                                revokeBuy(last_add_data.vm_id);

                                BuyAndDeployTwoVM(last_add_data.vm_name, add_data.vm_name, last_add_data.vm_id, add_data.vm_id, suitServer->server_name);
                                isSuccess = true;
                            }
                        }
                    }

                    if (last_add_data.deployment_way == 0 && add_data.deployment_way == 0)
                    {
                        int cpu = max(last_add_data.cpu, add_data.cpu);
                        int memory_cores = max(last_add_data.memory, add_data.memory);
                        SoldServer *suitServer = SearchForContinueVM(1, cpu, memory_cores);
                        if (suitServer != 0)
                        {
                            int new_cost = suitServer->hardware_cost + (total_days_num - now_day) * suitServer->daily_cost;
                            int old_cost = server_name2info[last_buy_server_name].hardware_cost + (total_days_num - now_day) * server_name2info[last_buy_server_name].daily_cost + server_name2info[buy_server_name].hardware_cost + (total_days_num - now_day) * server_name2info[buy_server_name].daily_cost;
                            if (new_cost < old_cost)
                            {
                                count_continue_buy++;
                                revokeBuy(add_data.vm_id);
                                revokeBuy(last_add_data.vm_id);
                                BuyAndDeployTwoVM(last_add_data.vm_name, add_data.vm_name, last_add_data.vm_id, add_data.vm_id, suitServer->server_name);
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
                    if (vm_id2info.find(vm_id) != vm_id2info.end())
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

    // Statistics sta;
    // vector<double> ans;
#ifdef REDIRECT
    // freopen("test.txt", "r", stdin);
    freopen("/Users/wangtongling/Desktop/training-data/test.txt", "r", stdin);
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
    freopen("training-2.txt", "r", stdin);
    // freopen("/Users/wangtongling/Desktop/training-data/training-4.txt", "r", stdin);
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
    freopen("training-3.txt", "r", stdin);
    // freopen("/Users/wangtongling/Desktop/training-data/training-5.txt", "r", stdin);
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
    freopen("training-4.txt", "r", stdin);
    // freopen("/Users/wangtongling/Desktop/training-data/training-6.txt", "r", stdin);
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