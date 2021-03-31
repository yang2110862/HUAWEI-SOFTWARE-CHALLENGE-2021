#include "H.h"

vector<SoldServer> sold_servers;
unordered_map<string, SoldVm> vm_name2info;
unordered_map<string,SoldServer> server_name2info;
unordered_map<int, VmIdInfo> vm_id2info;
queue<vector<RequestData>> request_datas;
vector<PurchasedServer*> purchase_servers;
unordered_map<string, vector<PurchasedServer*>> purchase_infos;

unordered_set<int> from_off_2_start;

int total_days_num, foreseen_days_num; //总天数和可预知的天数
int now_day;    //现在是第几天
int total_req_num  = 0;
int now_req_num = 0;

long long total_server_cost = 0;
long long total_power_cost = 0;
int total_migration_num = 0;

#ifdef PRINTINFO
clock_t _start,_end;
#endif

int number = 0; //给服务器编号

//状态值
int isDenseBuy = 0; // 0--非密度购买  1--密度购买
double _future_N_reqs_cpu_rate = 0;
double _future_N_reqs_memory_rate = 0;
double _migration_threahold = 0.4;
double _near_full_threhold = 0.07;

void ParseServerInfo() {
    int server_num;
    cin >> server_num;
    string server_name, cpu_core, memory_size, hardware_cost, daily_energy_cost;
    for (int i = 0; i < server_num; ++i) {
        SoldServer sold_server;
        cin >> server_name >> cpu_core >> memory_size >> hardware_cost >> daily_energy_cost;
        sold_server.server_name = server_name.substr(1, server_name.size() - 2);
        sold_server.cpu_cores = stoi(cpu_core.substr(0, cpu_core.size() - 1)) / 2;
        sold_server.memory_size = stoi(memory_size.substr(0, memory_size.size() - 1)) / 2;
        sold_server.hardware_cost = stoi(hardware_cost.substr(0, hardware_cost.size() - 1));
        sold_server.daily_energy_cost = stoi(daily_energy_cost.substr(0, daily_energy_cost.size() - 1));
        server_name2info[ sold_server.server_name] = sold_server;
        sold_servers.emplace_back(sold_server);   //可优化，不需要sold_server
    }
}
void ParseVmInfo() {
    int vm_num;
    cin >> vm_num;
    string vm_name, cpu_core, memory_size, deployment_way;
    for (int i = 0; i < vm_num; ++i) {
        SoldVm sold_vm;
        cin >> vm_name >> cpu_core >> memory_size >> deployment_way;
        vm_name = vm_name.substr(1, vm_name.size() - 2);
        sold_vm.cpu_cores = stoi(cpu_core.substr(0, cpu_core.size() - 1));
        sold_vm.memory_size = stoi(memory_size.substr(0, memory_size.size() - 1));
        sold_vm.deployment_way = stoi(deployment_way.substr(0, deployment_way.size() - 1));
        if (sold_vm.deployment_way == 1) {
            sold_vm.cpu_cores /= 2;
            sold_vm.memory_size /= 2;
        }
        vm_name2info[vm_name] = sold_vm;
    }
}
void ParseRequest(int days_num) {
    int operation_num;
    string operation, vm_name, vm_id;
    for (int i = 0; i < days_num; ++i) {
        cin >> operation_num;
        vector<RequestData> request_data;
        total_req_num += operation_num;
        for (int j = 0; j < operation_num; ++j) {
            cin >> operation;
            operation = operation.substr(1, operation.size() - 2);
            if (operation == "add") {
                RequestData data;
                data.operation = "add";
                cin >> vm_name >> vm_id;
                data.vm_name = vm_name.substr(0, vm_name.size() - 1);
                data.vm_id = stoi(vm_id.substr(0, vm_id.size() - 1));
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
void ParseInput() {
    ParseServerInfo();
    ParseVmInfo();
    cin >> total_days_num >> foreseen_days_num;
    ParseRequest(foreseen_days_num);
#ifdef TEST_PARSEINPUT
    cout << sold_servers.size() << endl;
    for (auto&& sold_server : sold_servers) {
        cout << sold_server.server_name << " " << sold_server.cpu_cores << " " << sold_server.memory_size
        << " " << sold_server.hardware_cost << " " << sold_server.daily_energy_cost << endl;
    }
    cout << vm_name2info.size() << endl;
    for (auto& vm : vm_name2info) {
        cout << vm.first << " " << vm.second.cpu_cores << " " << vm.second.memory_size << " "
        << vm.second.deployment_way << endl;
    }
    cout << request_datas.size() << endl;
    for (auto& request : request_datas) {
        cout << request.size() << endl;
        for (auto& data : request) {
            if (data.operation == "add") {
                cout << data.operation << " " << data.vm_name << " " << data.vm_id << endl;
            } else if (data.operation == "del") {
                cout << data.operation << " " << data.vm_id << endl;
            } else {
                cerr << "operation error!";
            }
        }
    }
#endif
}
// 筛选出利用率较低的服务器，迁移走其中的虚拟机。
bool NeedMigration(PurchasedServer *server) {
    if(server->A_vm_id.size() + server->B_vm_id.size() + server->AB_vm_id.size() == 0) return false;
    double _A_cpu_remain_rate = 1.0 * server->A_remain_core_num / server->total_core_num;
    double _B_cpu_remain_rate = 1.0 * server->B_remain_core_num / server->total_core_num;
    double _A_memory_remain_rate = 1.0 * server->A_remain_memory_size / server->total_memory_size;
    double _B_memory_remain_rate = 1.0 * server->B_remain_memory_size / server->total_memory_size;
    return (_A_cpu_remain_rate > _migration_threahold) + (_A_memory_remain_rate > _migration_threahold) + (_B_cpu_remain_rate > _migration_threahold)
                    + (_B_memory_remain_rate > _migration_threahold) >= 1;
}

int _HowManyCondionSuit(PurchasedServer *server){
/**
 * @description: 接NeedMigration，返回服务器满足的条件个数
 * @param {*}
 * @return {个数}
 */
    double _A_cpu_remain_rate = 1.0 * server->A_remain_core_num / server->total_core_num;
    double _B_cpu_remain_rate = 1.0 * server->B_remain_core_num / server->total_core_num;
    double _A_memory_remain_rate = 1.0 * server->A_remain_memory_size / server->total_memory_size;
    double _B_memory_remain_rate = 1.0 * server->B_remain_memory_size / server->total_memory_size;
    return (_A_cpu_remain_rate > _migration_threahold) + (_A_memory_remain_rate > _migration_threahold) + (_B_cpu_remain_rate > _migration_threahold)
                    + (_B_memory_remain_rate > _migration_threahold) ;
}

// 筛选出几乎满了的服务器，不作为目标服务器。
bool NearlyFull(PurchasedServer *server) {
    return (1.0 * server->A_remain_core_num / server->total_core_num < _near_full_threhold || 1.0 * server->A_remain_memory_size / server->total_memory_size < _near_full_threhold)
                    && (1.0 * server->B_remain_core_num / server->total_core_num < _near_full_threhold || 1.0 * server->B_remain_memory_size / server->total_memory_size < _near_full_threhold);
}
vector<MigrationInfo> Migration() {
    int max_migration_num = vm_id2info.size() * 30 / 1000;
    vector<MigrationInfo> migration_infos;
    // unordered_set<int> migrated_vms;
    unordered_set<int> happened_migtation_serverID;
    vector<PurchasedServer *> original_servers, target_servers;
    for (auto server : purchase_servers) {
        if (NeedMigration(server)) original_servers.emplace_back(server);
        if (!NearlyFull(server)) target_servers.emplace_back(server);
        // else
        //     target_servers.emplace_back(server);

        // else if ((server->A_remain_core_num + server->B_remain_core_num) / 2.0 / server->total_core_num < 0.8 ||
        //            (server->A_remain_memory_size + server->B_remain_memory_size) / 2.0 / server->total_memory_size < 0.8)
        //     target_servers.emplace_back(server);
    }

    //直接按照需要迁移的虚拟机内部数量排序
    sort(original_servers.begin(),original_servers.end(),[](PurchasedServer* a,PurchasedServer*b) {
        return (a->A_vm_id.size() + a->B_vm_id.size() + a->AB_vm_id.size() < b->A_vm_id.size() + b->B_vm_id.size() + b->AB_vm_id.size()) ;
    });
    //在按照需要迁移的虚拟机内部数量排序基础上，按照符合的条件数排序
    // sort(original_servers.begin(),original_servers.end(),[](PurchasedServer* a,PurchasedServer*b) {
    //     if(a->A_vm_id.size() + a->B_vm_id.size() + a->AB_vm_id.size() < b->A_vm_id.size() + b->B_vm_id.size() + b->AB_vm_id.size()){
    //         return true;
    //     }else if(a->A_vm_id.size() + a->B_vm_id.size() + a->AB_vm_id.size() == b->A_vm_id.size() + b->B_vm_id.size() + b->AB_vm_id.size()){
    //         if(_HowManyCondionSuit(a)>_HowManyCondionSuit(b)) return true;
    //         else return false;
    //     }else{
    //         return false;
    //     }
    // });

    for (auto &original_server : original_servers) {
        if (migration_infos.size() == max_migration_num) return migration_infos;
        Evaluate evaluate;
        unordered_set<int> a_vms = original_server->A_vm_id;
        for (auto &vm_id : a_vms) {
            if (migration_infos.size() == max_migration_num) return migration_infos;
            // if (!NeedMigration(original_server)) break;
            // if (migrated_vms.find(vm_id) != migrated_vms.end()) continue;
            VmIdInfo *vm_info = &vm_id2info[vm_id];
            double min_rate = 2.0;
            PurchasedServer* flag_server;
            char which_node ;
            for (auto &target_server : target_servers) {
                if(target_server->server_id == original_server->server_id) continue;
                if(happened_migtation_serverID.count(target_server->server_id)!=0) continue;
                if(target_server->A_remain_core_num < vm_info->cpu_cores || target_server->A_remain_memory_size < vm_info->memory_size) {
                    ;
                }
                else{
                    double _cpu_remain_rate = 1.0 * (target_server->A_remain_core_num - vm_info->cpu_cores) / target_server->total_core_num ;
                    double _memory_remain_rate = 1.0 * (target_server->A_remain_memory_size - vm_info->memory_size) / target_server->total_memory_size ;
                    if(_cpu_remain_rate + _memory_remain_rate < min_rate) {
                        min_rate = _cpu_remain_rate + _memory_remain_rate;
                        flag_server = target_server;
                        which_node = 'A';
                    }
                }
                if(target_server->B_remain_core_num < vm_info->cpu_cores || target_server->B_remain_memory_size < vm_info->memory_size) {
                    ;
                }
                else{
                    double _cpu_remain_rate = 1.0 * (target_server->B_remain_core_num- vm_info->cpu_cores) / target_server->total_core_num;
                    double _memory_remain_rate =  1.0 * (target_server->B_remain_memory_size - vm_info->memory_size) / target_server->total_memory_size;
                    if(_cpu_remain_rate + _memory_remain_rate < min_rate) {
                        min_rate = _cpu_remain_rate + _memory_remain_rate;
                        flag_server = target_server;
                        which_node = 'B';
                    }
                }
            }
            if(min_rate!=2.0) {
                total_migration_num++;
                happened_migtation_serverID.emplace(original_server->server_id);
                int cpu_cores = vm_info->cpu_cores;
                int memory_size = vm_info->memory_size;
                original_server->A_remain_core_num += cpu_cores;
                original_server->A_remain_memory_size += memory_size;
                original_server->A_vm_id.erase(vm_id);
                
                if(which_node == 'A') {
                    flag_server->A_remain_core_num -= cpu_cores;
                    flag_server->A_remain_memory_size -= memory_size;
                    flag_server->A_vm_id.insert(vm_id);
                    vm_info->purchase_server = flag_server;
                    vm_info->node = 'A';
                    // migrated_vms.insert(vm_id);
                    migration_infos.emplace_back(MigrationInfo(vm_id, flag_server, 'A'));
                }else if(which_node == 'B') {
                    flag_server->B_remain_core_num -= cpu_cores;
                    flag_server->B_remain_memory_size -= memory_size;
                    flag_server->B_vm_id.insert(vm_id);
                    vm_info->purchase_server = flag_server;
                    vm_info->node = 'B';
                    // migrated_vms.insert(vm_id);
                    migration_infos.emplace_back(MigrationInfo(vm_id, flag_server, 'B'));
                }
                
            }
        }
        unordered_set<int> b_vms = original_server->B_vm_id;
        for (auto &vm_id : b_vms) {
            if (migration_infos.size() == max_migration_num) return migration_infos;
            //if (!NeedMigration(original_server)) break;
            // if (migrated_vms.find(vm_id) != migrated_vms.end()) continue;
            VmIdInfo *vm_info = &vm_id2info[vm_id];
            double min_rate = 2.0;
            PurchasedServer* flag_server;
            char which_node ;
            for (auto &target_server : target_servers) {
                if(target_server->server_id == original_server->server_id) continue;
                if(happened_migtation_serverID.count(target_server->server_id)!=0) continue;
                if(target_server->A_remain_core_num < vm_info->cpu_cores || target_server->A_remain_memory_size < vm_info->memory_size) {
                    ;
                }
                else{
                    double _cpu_remain_rate = 1.0 * (target_server->A_remain_core_num - vm_info->cpu_cores) / target_server->total_core_num ;
                    double _memory_remain_rate = 1.0 * (target_server->A_remain_memory_size - vm_info->memory_size) / target_server->total_memory_size ;
                    if(_cpu_remain_rate + _memory_remain_rate < min_rate) {
                        min_rate = _cpu_remain_rate + _memory_remain_rate;
                        flag_server = target_server;
                        which_node = 'A';
                    }
                }
                if(target_server->B_remain_core_num < vm_info->cpu_cores || target_server->B_remain_memory_size < vm_info->memory_size) {
                    ;
                }
                else{
                    double _cpu_remain_rate =  1.0 * (target_server->B_remain_core_num- vm_info->cpu_cores) / target_server->total_core_num;
                    double _memory_remain_rate =  1.0 * (target_server->B_remain_memory_size - vm_info->memory_size) / target_server->total_memory_size;
                    if(_cpu_remain_rate + _memory_remain_rate < min_rate) {
                        min_rate = _cpu_remain_rate + _memory_remain_rate;
                        flag_server = target_server;
                        which_node = 'B';
                    }
                }

                
            }

            if(min_rate!=2.0) {
                total_migration_num++;
                happened_migtation_serverID.emplace(original_server->server_id);
                int cpu_cores = vm_info->cpu_cores;
                int memory_size = vm_info->memory_size;
                original_server->B_remain_core_num += cpu_cores;
                original_server->B_remain_memory_size += memory_size;
                original_server->B_vm_id.erase(vm_id);
                
                if(which_node == 'A') {
                    flag_server->A_remain_core_num -= cpu_cores;
                    flag_server->A_remain_memory_size -= memory_size;
                    flag_server->A_vm_id.insert(vm_id);
                    vm_info->purchase_server = flag_server;
                    vm_info->node = 'A';
                    // migrated_vms.insert(vm_id);
                    migration_infos.emplace_back(MigrationInfo(vm_id, flag_server, 'A'));
                }else if(which_node == 'B') {
                    flag_server->B_remain_core_num -= cpu_cores;
                    flag_server->B_remain_memory_size -= memory_size;
                    flag_server->B_vm_id.insert(vm_id);
                    vm_info->purchase_server = flag_server;
                    vm_info->node = 'B';
                    // migrated_vms.insert(vm_id);
                    migration_infos.emplace_back(MigrationInfo(vm_id, flag_server, 'B'));
                }
                
            }
        }
        unordered_set<int> ab_vms = original_server->AB_vm_id;
        for (auto &vm_id : ab_vms) {
            if (migration_infos.size() == max_migration_num) return migration_infos;
            //if (!NeedMigration(original_server)) break;
            // if (migrated_vms.find(vm_id) != migrated_vms.end()) continue;
            VmIdInfo *vm_info = &vm_id2info[vm_id];
            double min_rate = 2.0;
            PurchasedServer* flag_server;
            char which_node ;
            for (auto &target_server : target_servers) {
                if(target_server->server_id == original_server->server_id) continue;
                if(happened_migtation_serverID.count(target_server->server_id)!=0) continue;
                if(target_server->A_remain_core_num < vm_info->cpu_cores || target_server->A_remain_memory_size < vm_info->memory_size|| target_server->B_remain_core_num < vm_info->cpu_cores || target_server->B_remain_memory_size < vm_info->memory_size) {
                    ;
                }
                else{
                    double _cpu_remain_rate = (1.0 * (target_server->A_remain_core_num - vm_info->cpu_cores) / target_server->total_core_num + 1.0 * (target_server->B_remain_core_num - vm_info->cpu_cores) / target_server->total_core_num) / 2;
                    double _memory_remain_rate = (1.0 * (target_server->A_remain_memory_size - vm_info->memory_size) / target_server->total_memory_size + 1.0 * (target_server->B_remain_memory_size - vm_info->memory_size) / target_server->total_memory_size) / 2;
                    if(_cpu_remain_rate + _memory_remain_rate < min_rate) {
                        min_rate = _cpu_remain_rate + _memory_remain_rate;
                        flag_server = target_server;
                        which_node = 'C';
                    }
                }
            }

            if(min_rate!=2.0) {
                total_migration_num++;
                happened_migtation_serverID.emplace(original_server->server_id);
                int cpu_cores = vm_info->cpu_cores;
                int memory_size = vm_info->memory_size;
                original_server->A_remain_core_num += cpu_cores;
                original_server->A_remain_memory_size += memory_size;
                original_server->B_remain_core_num += cpu_cores;
                original_server->B_remain_memory_size += memory_size;
                original_server->AB_vm_id.erase(vm_id);
                
             
                flag_server->A_remain_core_num -= cpu_cores;
                flag_server->A_remain_memory_size -= memory_size;
                flag_server->B_remain_core_num -= cpu_cores;
                flag_server->B_remain_memory_size -= memory_size;
                flag_server->AB_vm_id.insert(vm_id);
                vm_info->purchase_server = flag_server;
                vm_info->node = 'C';
                // migrated_vms.insert(vm_id);
                migration_infos.emplace_back(MigrationInfo(vm_id, flag_server, 'C'));
            
                
            }
        }
    }
    return migration_infos;
}
void AddVm(AddData& add_data) {
    Cmp cmp;
    bool deployed = false;
    Evaluate evaluate;  //创建一个评价类的实例
    int deployment_way = add_data.deployment_way;
    if (deployment_way == 1) { //双节点部署
        int cpu_cores = add_data.cpu_cores;
        int memory_size = add_data.memory_size;
        double min_remain_rate = 2.0;
        PurchasedServer* flag_server;
        for (auto& purchase_server : purchase_servers) {
            if (deployed) {
                break;
            }   //先筛选能用的服务器
            if (purchase_server->A_remain_core_num >= cpu_cores && purchase_server->A_remain_memory_size >= memory_size
            && purchase_server->B_remain_core_num >= cpu_cores && purchase_server->B_remain_memory_size >= memory_size) {
                // double _cpu_remain_rate = (1.0*(purchase_server->A_remain_core_num - cpu_cores)/purchase_server->total_core_num + 1.0*(purchase_server->B_remain_core_num - cpu_cores)/ purchase_server->total_core_num) / 2;
                // double _memory_remain_rate = (1.0*(purchase_server->A_remain_memory_size - memory_size)/purchase_server->total_memory_size + 1.0*(purchase_server->B_remain_memory_size - memory_size) / purchase_server->total_memory_size) / 2;
                // if(_cpu_remain_rate + _memory_remain_rate < min_remain_rate) {
                //     min_remain_rate = _cpu_remain_rate + _memory_remain_rate;
                //     flag_server = purchase_server;
                // }
                double _cpu_remain_rate = min(1.0*(purchase_server->A_remain_core_num - cpu_cores)/purchase_server->total_core_num , 1.0*(purchase_server->B_remain_core_num - cpu_cores)/ purchase_server->total_core_num) ;
                double _memory_remain_rate = min(1.0*(purchase_server->A_remain_memory_size - memory_size)/purchase_server->total_memory_size , 1.0*(purchase_server->B_remain_memory_size - memory_size) / purchase_server->total_memory_size) ;
                // if((_cpu_remain_rate + _memory_remain_rate)  < min_remain_rate) {
                //     min_remain_rate = _cpu_remain_rate + _memory_remain_rate;
                //     flag_server = purchase_server;
                // }
                if( 2* max(_cpu_remain_rate , _memory_remain_rate)  < min_remain_rate) {
                    min_remain_rate = _cpu_remain_rate + _memory_remain_rate;
                    flag_server = purchase_server;
                }
            }
        }

        if(min_remain_rate != 2.0) {
            //代表从关机的服务器中选取的
            if(flag_server->A_vm_id.size()+flag_server->B_vm_id.size()+flag_server->AB_vm_id.size() == 0){
                from_off_2_start.insert(flag_server->server_id);
            }
            deployed = true;
            flag_server->A_remain_core_num -= cpu_cores;
            flag_server->A_remain_memory_size -= memory_size;
            flag_server->B_remain_core_num -= cpu_cores;
            flag_server->B_remain_memory_size -= memory_size;
            flag_server->AB_vm_id.insert(add_data.vm_id);

            VmIdInfo vm_id_info;
            vm_id_info.purchase_server = flag_server;
            vm_id_info.vm_name = add_data.vm_name;
            vm_id_info.cpu_cores = cpu_cores;
            vm_id_info.memory_size = memory_size;
            vm_id_info.node = 'C';
            vm_id2info[add_data.vm_id] = vm_id_info;
        }


        double min_dense_cost = 99999999999999;
        SoldServer* flag_sold_server;
        for(auto& sold_server : sold_servers) {
            if(deployed)
                break;
            if (sold_server.cpu_cores >= cpu_cores && sold_server.memory_size >= memory_size) {
                double dense_cost;
                if(isDenseBuy == 1) {
                    // double use_rate = (1.0 *(cpu_cores) / sold_server.cpu_cores + 1.0 *(memory_size) / sold_server.memory_size) / 2;
                    // double use_rate = max(1.0 *(cpu_cores) / sold_server.cpu_cores , 1.0 *(memory_size) / sold_server.memory_size) ;
                    double _cpu_rate = 1.0 * cpu_cores / sold_server.cpu_cores;
                    double _memory_rate = 1.0 *(memory_size) / sold_server.memory_size ;
                    double T = 0.9;
                    double _temp_sum = pow(_cpu_rate,1.0 / T) + pow(_memory_rate,1.0 / T);
                    double use_cpu_rate = pow(_cpu_rate,1.0 / T) / _temp_sum;
                    double use_memory_rate = pow(_memory_rate,1.0 / T) / _temp_sum;
                    double use_rate = max  (use_cpu_rate , use_memory_rate);
                    // dense_cost = 1.0 * sold_server.hardware_cost * use_rate;
                    dense_cost = 1.0 * (sold_server.hardware_cost + sold_server.daily_energy_cost * (total_days_num - now_day)) * use_rate;
                }else{
                    // dense_cost = sold_server.hardware_cost;
                    dense_cost = sold_server.hardware_cost + sold_server.daily_energy_cost * (total_days_num - now_day);
                }
                if(dense_cost < min_dense_cost) {
                    min_dense_cost = dense_cost;
                    flag_sold_server = &sold_server;
                }
            }
        }
        if(!deployed) {
            //购买新的服务器并部署
            total_server_cost+=flag_sold_server->hardware_cost;
            deployed = true;
            PurchasedServer* purchase_server = new PurchasedServer;
            purchase_server->total_core_num = flag_sold_server->cpu_cores;
            purchase_server->total_memory_size = flag_sold_server->memory_size;
            purchase_server->A_remain_core_num = flag_sold_server->cpu_cores - cpu_cores;
            purchase_server->A_remain_memory_size = flag_sold_server->memory_size - memory_size;
            purchase_server->B_remain_core_num = flag_sold_server->cpu_cores - cpu_cores;
            purchase_server->B_remain_memory_size = flag_sold_server->memory_size - memory_size;
            purchase_server->AB_vm_id.insert(add_data.vm_id);
            purchase_server->server_name = flag_sold_server->server_name;
            purchase_servers.emplace_back(purchase_server);
            purchase_infos[flag_sold_server->server_name].emplace_back(purchase_server);

            VmIdInfo vm_id_info;
            vm_id_info.purchase_server = purchase_server;
            vm_id_info.vm_name = add_data.vm_name;
            vm_id_info.cpu_cores = cpu_cores;
            vm_id_info.memory_size = memory_size;
            vm_id_info.node = 'C';
            vm_id2info[add_data.vm_id] = vm_id_info;
        }

       
    } else {       //单节点部署
        int cpu_cores = add_data.cpu_cores;
        int memory_size = add_data.memory_size;
        vector<PurchasedServer*> can_deploy_servers;
        for (auto& purchase_server : purchase_servers) {    //先筛选能用的服务器
            if ((purchase_server->A_remain_core_num < cpu_cores || purchase_server->A_remain_memory_size < memory_size)
            && (purchase_server->B_remain_core_num < cpu_cores || purchase_server->B_remain_memory_size < memory_size)) {
                continue;
            } else {
                can_deploy_servers.emplace_back(purchase_server);
                if (purchase_server->A_remain_core_num >= cpu_cores && purchase_server->A_remain_memory_size >= memory_size) {
                    purchase_server->can_deploy_A = true;
                }
                if (purchase_server->B_remain_core_num >= cpu_cores && purchase_server->B_remain_memory_size >= memory_size) {
                    purchase_server->can_deploy_B = true;
                }
            }
        }
        //sort(can_deploy_servers.begin(), can_deploy_servers.end(), cmp.CanDeploySingle);
        double min_remain_rate = 2.0;
        PurchasedServer* flag_server;
        char which_node = 'C';
        for (auto& purchase_server : can_deploy_servers) {
            if (deployed) {
                break;
            }
            bool evaluate_A = evaluate.PurchasedServerA(purchase_server, cpu_cores, memory_size);
            bool evaluate_B = evaluate.PurchasedServerB(purchase_server, cpu_cores, memory_size);

            if(evaluate_A&&evaluate_B) {
                double _cpu_remain_rate = 1.0*(purchase_server->A_remain_core_num - cpu_cores)/purchase_server->total_core_num ;
                double _memory_remain_rate = 1.0*(purchase_server->A_remain_memory_size - memory_size)/purchase_server->total_memory_size ;



                // if(_cpu_remain_rate + _memory_remain_rate < min_remain_rate) {
                //     min_remain_rate = _cpu_remain_rate + _memory_remain_rate;
                //     flag_server = purchase_server;
                //     which_node = 'A';
                // }
                if( 2 * max(_cpu_remain_rate , _memory_remain_rate )< min_remain_rate) {
                    min_remain_rate = _cpu_remain_rate + _memory_remain_rate;
                    flag_server = purchase_server;
                    which_node = 'A';
                }



                _cpu_remain_rate =  1.0*(purchase_server->B_remain_core_num- cpu_cores) / purchase_server->total_core_num;
                _memory_remain_rate =  1.0*(purchase_server->B_remain_memory_size- memory_size) / purchase_server->total_memory_size;
                // _cpu_remain_rate = (1.0*(purchase_server->A_remain_core_num )/purchase_server->total_core_num + 1.0*(purchase_server->B_remain_core_num- cpu_cores) / purchase_server->total_core_num) / 2;
                // _memory_remain_rate = (1.0*(purchase_server->A_remain_memory_size)/purchase_server->total_memory_size + 1.0*(purchase_server->B_remain_memory_size- memory_size) / purchase_server->total_memory_size) / 2;
                // if(_cpu_remain_rate + _memory_remain_rate < min_remain_rate) {
                //     min_remain_rate = _cpu_remain_rate + _memory_remain_rate;
                //     flag_server = purchase_server;
                //     which_node = 'B';
                // }
                if( 2 * max(_cpu_remain_rate , _memory_remain_rate )< min_remain_rate) {
                    min_remain_rate = _cpu_remain_rate + _memory_remain_rate;
                    flag_server = purchase_server;
                    which_node = 'B';
                }



            }else if (evaluate_A) {
                double _cpu_remain_rate = 1.0*(purchase_server->A_remain_core_num - cpu_cores)/purchase_server->total_core_num ;
                double _memory_remain_rate = 1.0*(purchase_server->A_remain_memory_size - memory_size)/purchase_server->total_memory_size;
                // double _cpu_remain_rate = (1.0*(purchase_server->A_remain_core_num - cpu_cores)/purchase_server->total_core_num + 1.0*(purchase_server->B_remain_core_num) / purchase_server->total_core_num) / 2;
                // double _memory_remain_rate = (1.0*(purchase_server->A_remain_memory_size - memory_size)/purchase_server->total_memory_size + 1.0*(purchase_server->B_remain_memory_size) / purchase_server->total_memory_size) / 2;
                // if(_cpu_remain_rate + _memory_remain_rate < min_remain_rate) {
                //     min_remain_rate = _cpu_remain_rate + _memory_remain_rate;
                //     flag_server = purchase_server;
                //     which_node = 'A';
                // }
                if( 2 * max(_cpu_remain_rate , _memory_remain_rate) < min_remain_rate) {
                    min_remain_rate = _cpu_remain_rate + _memory_remain_rate;
                    flag_server = purchase_server;
                    which_node = 'A';
                }
            }else if (evaluate_B) {
                double _cpu_remain_rate = 1.0*(purchase_server->B_remain_core_num- cpu_cores) / purchase_server->total_core_num;
                double _memory_remain_rate = 1.0*(purchase_server->B_remain_memory_size- memory_size) / purchase_server->total_memory_size;
                // double _cpu_remain_rate = (1.0*(purchase_server->A_remain_core_num )/purchase_server->total_core_num + 1.0*(purchase_server->B_remain_core_num- cpu_cores) / purchase_server->total_core_num) / 2;
                // double _memory_remain_rate = (1.0*(purchase_server->A_remain_memory_size)/purchase_server->total_memory_size + 1.0*(purchase_server->B_remain_memory_size- memory_size) / purchase_server->total_memory_size) / 2;
                //  double _cpu_remain_rate = max(1.0*(purchase_server->A_remain_core_num )/purchase_server->total_core_num , 1.0*(purchase_server->B_remain_core_num - cpu_cores)/ purchase_server->total_core_num) ;
                // double _memory_remain_rate =  max(1.0*(purchase_server->A_remain_memory_size )/purchase_server->total_memory_size , 1.0*(purchase_server->B_remain_memory_size - memory_size) / purchase_server->total_memory_size) ;
                // if(_cpu_remain_rate + _memory_remain_rate < min_remain_rate) {
                //     min_remain_rate = _cpu_remain_rate + _memory_remain_rate;
                //     flag_server = purchase_server;
                //     which_node = 'B';
                // }
                if( 2 * max(_cpu_remain_rate , _memory_remain_rate) < min_remain_rate) {
                    min_remain_rate = _cpu_remain_rate + _memory_remain_rate;
                    flag_server = purchase_server;
                    which_node = 'B';
                }
            }
        }
        if(min_remain_rate !=2.0) {
            //代表从关机的服务器中选取的
            if(flag_server->A_vm_id.size()+flag_server->B_vm_id.size()+flag_server->AB_vm_id.size() == 0){
                from_off_2_start.insert(flag_server->server_id);
            }
            if (which_node == 'A') {
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
                vm_id2info[add_data.vm_id] = vm_id_info;
            } else if (which_node == 'B') {
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
                vm_id2info[add_data.vm_id] = vm_id_info;
            }
        }

        double min_dense_cost = 99999999999999;
        SoldServer* flag_sold_server;
        for (auto& sold_server : sold_servers) {
            if (deployed) {
                break;
            }
            if (sold_server.cpu_cores >= cpu_cores && sold_server.memory_size >= memory_size) {
                double dense_cost;
                if(isDenseBuy == 1) {
                    // double use_rate = (1.0 *(cpu_cores) / sold_server.cpu_cores + 1.0 *(memory_size) / sold_server.memory_size) / 2;
                    // double use_rate = max(1.0 *(cpu_cores) / sold_server.cpu_cores , 1.0 *(memory_size) / sold_server.memory_size) ;
                    // double use_cpu_rate = pow(2.718,1.0 *(cpu_cores) / sold_server.cpu_cores) / ( pow(2.718,1.0 *(cpu_cores) / sold_server.cpu_cores)+pow(2.718,1.0 *(memory_size) / sold_server.memory_size)) ;
                    // double use_memory_rate = pow(2.718,1.0 *(memory_size) / sold_server.memory_size) / ( pow(2.718,1.0 *(cpu_cores) / sold_server.cpu_cores)+pow(2.718,1.0 *(memory_size) / sold_server.memory_size)) ;
                    // double use_rate = 0.5 * (use_cpu_rate + use_memory_rate);
                    double _cpu_rate = 1.0 * cpu_cores / sold_server.cpu_cores;
                    double _memory_rate = 1.0 *(memory_size) / sold_server.memory_size ;
                    double T = 0.9;
                    double _temp_sum = pow(_cpu_rate,1.0 / T) + pow(_memory_rate,1.0 / T);
                    double use_cpu_rate = pow(_cpu_rate,1.0 / T) / _temp_sum;
                    double use_memory_rate = pow(_memory_rate,1.0 / T) / _temp_sum;
                    double use_rate = max  (use_cpu_rate , use_memory_rate);
                    dense_cost = 1.0 * (sold_server.hardware_cost + sold_server.daily_energy_cost * (total_days_num - now_day)) * use_rate;

                }else{
                    dense_cost = 1.0 * (sold_server.hardware_cost + sold_server.daily_energy_cost * (total_days_num - now_day));
                }
                if(dense_cost < min_dense_cost) {
                    min_dense_cost = dense_cost;
                    flag_sold_server = &sold_server;
                }
            }
        }

        if(!deployed) {
            //购买一台新服务器
            total_server_cost+=flag_sold_server->hardware_cost;
            deployed = true;
            PurchasedServer* purchase_server = new PurchasedServer;
            purchase_server->total_core_num = flag_sold_server->cpu_cores;
            purchase_server->total_memory_size = flag_sold_server->memory_size;
            purchase_server->A_remain_core_num = flag_sold_server->cpu_cores - cpu_cores;
            purchase_server->A_remain_memory_size = flag_sold_server->memory_size - memory_size;
            purchase_server->B_remain_core_num = flag_sold_server->cpu_cores;
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
            vm_id2info[add_data.vm_id] = vm_id_info;
        }


    }
}
void DeleteVm(int vm_id) {
    VmIdInfo vm_info = vm_id2info[vm_id];
    PurchasedServer* purchase_server = vm_info.purchase_server;
    char node = vm_info.node;
    int cpu_cores = vm_info.cpu_cores;
    int memory_size = vm_info.memory_size;
    if (node == 'A') {
        purchase_server->A_remain_core_num += cpu_cores;
        purchase_server->A_remain_memory_size += memory_size;
        purchase_server->A_vm_id.erase(vm_id);
    } else if (node == 'B') {
        purchase_server->B_remain_core_num += cpu_cores;
        purchase_server->B_remain_memory_size += memory_size;
        purchase_server->B_vm_id.erase(vm_id);
    } else if (node == 'C') {
        purchase_server->A_remain_core_num += cpu_cores;
        purchase_server->A_remain_memory_size += memory_size;
        purchase_server->B_remain_core_num += cpu_cores;
        purchase_server->B_remain_memory_size += memory_size;
        purchase_server->AB_vm_id.erase(vm_id);
    } else {
        //cerr << "DeleError";
    }
}
void Print(vector<int>& vm_ids, vector<MigrationInfo> &migration_infos) {
    int purchase_type_num = purchase_infos.size();
    cout << "(purchase, " << purchase_type_num << ')' << endl;
    for (auto& purchase_info : purchase_infos) {
        cout << '(' << purchase_info.first << ", " << purchase_info.second.size() << ')' << endl;
    }
    //cout << "(migration, 0)" << endl;
    cout << "(migration, " << migration_infos.size() << ")" << endl;
    for (auto &migration_info : migration_infos) {
        cout << "(" << migration_info.vm_id << ", " << migration_info.server->server_id;
        if (migration_info.node != 'C') cout << ", " << migration_info.node;
        cout << ")" << endl;
    }
    for (auto& vm_id : vm_ids) {
        VmIdInfo vm_info = vm_id2info[vm_id];
        char node = vm_info.node;
        if (node == 'C') {
            cout << '(' << vm_info.purchase_server->server_id << ')' << endl;
        } else {
            cout << '(' << vm_info.purchase_server->server_id << ", " << node << ')' << endl;
        }
    }
}
void Numbering() {
    for (auto& purchase_info : purchase_infos) {
        for (auto& server : purchase_info.second) {
            server->server_id = number++;
            //当天购买的服务器都是从关机到开机的服务器
            from_off_2_start.insert(server->server_id);
        }
    }
}
void Compute_Power_Cost(){
    for(auto& server:purchase_servers){
        if(server->A_vm_id.size() + server->B_vm_id.size() + server->AB_vm_id.size()!=0){
            //当天结束时候有虚拟机的服务器
            total_power_cost += server_name2info[server->server_name].daily_energy_cost;
        }else{
            //当天结束时关机，但是当天开过机的服务器，算电费
            if(from_off_2_start.count(server->server_id)!=0){
                total_power_cost += server_name2info[server->server_name].daily_energy_cost;
            }
        }
    }
}

vector<int> GetAllResourceOfOwnServers(bool isRemained = false){
/**
 * @description:获取当前所有已经购买的服务器的总资源 或 剩余资源
 * @param {isRemained 为false ：总资源，true：剩余资源}
 * @return {vector<int> 二维数组 [0]--cpu，[1]--memory}
 */
    int _total_cpu = 0;
    int _total_memory = 0;
    if(!isRemained){
        for(auto& purchase_server:purchase_servers){
            string _serverName = purchase_server->server_name;
            _total_cpu+= server_name2info[_serverName].cpu_cores *2;
            _total_memory +=server_name2info[_serverName].memory_size*2;
        }
    }else{
        for(auto& purchase_server:purchase_servers){
            _total_cpu+= purchase_server->A_remain_core_num + purchase_server->B_remain_core_num;
            _total_memory +=purchase_server->A_remain_memory_size + purchase_server->B_remain_memory_size;
        }
    }
    return {_total_cpu,_total_memory};
}

vector<int> GetAllResourceOfToday(vector<RequestData> &reqInfoOfToday){
/**
 * @description: 获取当天请求所需的总资源,当天增加的减去删除的
 * @param {reqInfoOfToday ： 当天的请求信息}
 * @return {vector<int> 二维数组 [0]--cpu，[1]--memory}
 */
    int _total_cpu = 0;
    int _total_memory = 0;
    for(auto& req:reqInfoOfToday){
        string vm_name = req.vm_name;
        int vm_isTwoNode = vm_name2info[vm_name].deployment_way;
        int vm_cpu,vm_memory;
        if(vm_isTwoNode == 1){
            vm_cpu = vm_name2info[vm_name].cpu_cores * 2;
            vm_memory = vm_name2info[vm_name].memory_size * 2;
        }else{
            vm_cpu = vm_name2info[vm_name].cpu_cores;
            vm_memory = vm_name2info[vm_name].memory_size;
        }
        if(req.operation == "add"){
            _total_cpu+=vm_cpu;
            _total_memory+=vm_memory;
        }else if(req.operation == "del"){
            _total_cpu-=vm_cpu;
            _total_memory-=vm_memory;
        }
    }
    return {_total_cpu,_total_memory};
}

vector<int> GetMaxResourceOfToday(vector<RequestData> &reqInfoOfToday){
/**
 * @description: 获取当天请求巅峰时刻的Cpu值和Memory值
 * @param {reqInfoOfToday ： 当天的请求信息}
 * @return {vector<int> 二维数组 [0]--max_cpu，[1]--max_memory}
 */
    int _total_cpu = 0;
    int _total_memory = 0;
    int _max_cpu = 0;
    int _max_memory = 0;
    for(auto& req:reqInfoOfToday){
        string vm_name = req.vm_name;
        int vm_isTwoNode = vm_name2info[vm_name].deployment_way;
        int vm_cpu,vm_memory;
        if(vm_isTwoNode == 1){
            vm_cpu = vm_name2info[vm_name].cpu_cores * 2;
            vm_memory = vm_name2info[vm_name].memory_size * 2;
        }else{
            vm_cpu = vm_name2info[vm_name].cpu_cores;
            vm_memory = vm_name2info[vm_name].memory_size;
        }
        if(req.operation == "add"){
            _total_cpu+=vm_cpu;
            _total_memory+=vm_memory;
            if(_total_cpu > _max_cpu){
                _max_cpu = _total_cpu;
            }
            if(_total_memory > _max_memory){
                _max_memory = _total_memory;
            }
        }else if(req.operation == "del"){
            _total_cpu-=vm_cpu;
            _total_memory-=vm_memory;
        }
    }
    return {_max_cpu,_max_memory};
}

vector<int> GetAllResourceOfFutureNDays(int req_num){
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
    while(_temp.size()>=1 && _cnt<req_num){
        vector<RequestData>_tempReqs = _temp.front();
        _temp.pop();
        for(auto& _req:_tempReqs){
            if(_cnt>=req_num){
                break;
            }
            _cnt++;
            string vm_name = _req.vm_name;
            int vm_isTwoNode = vm_name2info[vm_name].deployment_way;
            int vm_cpu,vm_memory;
            if(vm_isTwoNode == 1){
            vm_cpu = vm_name2info[vm_name].cpu_cores * 2;
            vm_memory = vm_name2info[vm_name].memory_size * 2;
            }else{
                vm_cpu = vm_name2info[vm_name].cpu_cores;
                vm_memory = vm_name2info[vm_name].memory_size;
            }
            if(_req.operation == "add"){
                _total_cpu+=vm_cpu;
                _total_memory+=vm_memory;
                if(_total_cpu > _max_cpu){
                    _max_cpu = _total_cpu;
                }
                if(_total_memory > _max_memory){
                    _max_memory = _total_memory;
                }
            }else if(_req.operation == "del"){
                _total_cpu-=vm_cpu;
                _total_memory-=vm_memory;
            }
        }
    }
    return {_total_cpu,_total_memory,_max_cpu,_max_memory};
}

void SolveProblem() {
    Cmp cmp;
    sort(sold_servers.begin(), sold_servers.end(), cmp.SoldServers);
    for (int i = 0; i < total_days_num; ++i) {
        now_day = i+1;
        from_off_2_start.erase(from_off_2_start.begin(),from_off_2_start.end());
        // vector<MigrationInfo> migration_infos;
        vector<MigrationInfo> migration_infos = Migration();

        //获取迁移之后的系统可以提供的总资源
        vector<int> allResouceAfterMigration = GetAllResourceOfOwnServers(true);


        //获取未来N条请求所需要的总资源
        vector<int> allResourceOfNReqs = GetAllResourceOfFutureNDays(5000);


        vector<RequestData> intraday_requests = request_datas.front();
        request_datas.pop();
        int request_num = intraday_requests.size();
        vector<int> vm_ids;
        for (int j = 0; j < request_num; ++j) {       //把请求添加的虚拟机ID有序存起来   !!可能有bug
            if (intraday_requests[j].operation == "add") {
                vm_ids.emplace_back(intraday_requests[j].vm_id);
            }
        }

        //获取当天请求所需要的峰值时刻的cpu和memory
        vector<int> max_resource_of_today_reqs = GetMaxResourceOfToday(intraday_requests);
        double _rate = 1.0;
        if(allResouceAfterMigration[0] * _rate >= max_resource_of_today_reqs[0] && allResouceAfterMigration[1] * _rate >= max_resource_of_today_reqs[1]){
            isDenseBuy = 1;
        }else{
            isDenseBuy = 0;
        }


        for (int j = 0; j < request_num; ++j) {
            string operation = intraday_requests[j].operation;
            vector<AddData> continuous_add_datas;
            while (operation == "add" && j < request_num) {
                string vm_name = intraday_requests[j].vm_name;
                SoldVm sold_vm = vm_name2info[vm_name];
                int deployment_way = sold_vm.deployment_way;
                int cpu_cores = sold_vm.cpu_cores;
                int memory_size = sold_vm.memory_size;
                int vm_id = intraday_requests[j].vm_id;
                continuous_add_datas.emplace_back(deployment_way, cpu_cores, memory_size, vm_id, vm_name);
                ++j;
                if (j == request_num) {
                    break;
                }
                operation = intraday_requests[j].operation;
            }
            sort(continuous_add_datas.begin(), continuous_add_datas.end(), cmp.ContinuousADD);
            for (auto& add_data : continuous_add_datas) {
                now_req_num ++;
                AddVm(add_data);
            }
            if (j == request_num) {
                    break;
            }
            if (operation == "del") {
                now_req_num ++;
                int vm_id = intraday_requests[j].vm_id;
                DeleteVm(vm_id);
                vm_id2info.erase(vm_id);
            }
        }
        Numbering(); //给购买了的服务器编号
        Print(vm_ids, migration_infos);
        fflush(stdout);
        purchase_infos.clear();
        from_off_2_start.clear();
        if (i < total_days_num - foreseen_days_num) ParseRequest(1);
        Compute_Power_Cost();
    }
}

void PrintCostInfo(){
    cout<<"Server Num : "<<purchase_servers.size()<<endl;
    cout<<"Total Migration Num : " <<total_migration_num<<endl;
    #ifdef PRINTINFO
    cout<<"Time: "<<double(_end - _start) / CLOCKS_PER_SEC<< " s"<<endl;
    #endif
    cout<<"Total Cost : "<< to_string(total_server_cost)<<" + "<<to_string(total_power_cost) <<" = "<<total_power_cost + total_server_cost<<endl;
}

int main(int argc, char* argv[]) {


#ifdef REDIRECT
    freopen("/Users/wangtongling/Desktop/training-data/training-2.txt", "r", stdin);
    // freopen("out1.txt", "w", stdout);
#endif
#ifdef PRINTINFO
    _start = clock();
#endif

    ParseInput();
    SolveProblem();
#ifdef PRINTINFO
    _end = clock();
#endif

#ifdef PRINTINFO
    PrintCostInfo();
#endif

#ifdef REDIRECT
    // fclose(stdin);
    // fclose(stdout);
#endif
    return 0;
}
