#include "H.h"

vector<SoldServer> sold_servers;
unordered_map<string, SoldVm> vm_name2info;
unordered_map<string,SoldServer> server_name2info;
unordered_map<int, VmIdInfo> vm_id2info;
queue<vector<RequestData>> request_datas;
vector<PurchasedServer*> purchase_servers;
unordered_map<string, vector<PurchasedServer*>> purchase_infos;

unordered_set<int> from_off_2_start;
double k1 = 0.75, k2 = 1 - k1; //CPU和memory的加权系数
double r1 = 0.5, r2 = 1 - r1; //CPU和memory剩余率的加权系数

int number = 0; //给服务器编号
int total_days_num, foreseen_days_num; //总天数和可预知的天数
int total_req_num  = 0;
int now_req_num = 0;

long long total_server_cost = 0;
long long total_power_cost = 0;
int total_migration_num = 0;

#ifdef PRINTINFO
clock_t _start,_end;
#endif

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
    if (server->A_vm_id.size() + server->B_vm_id.size() + server->AB_vm_id.size() == 0) return false;
    double _A_cpu_remain_rate = 1.0 * server->A_remain_core_num / server->total_core_num;
    double _B_cpu_remain_rate = 1.0 * server->B_remain_core_num / server->total_core_num;
    double _A_memory_remain_rate = 1.0 * server->A_remain_memory_size / server->total_memory_size;
    double _B_memory_remain_rate = 1.0 * server->B_remain_memory_size / server->total_memory_size;
    double threadhold = 0.4; //减小能增加迁移数量。
    return (_A_cpu_remain_rate > threadhold) + (_A_memory_remain_rate > threadhold) + (_B_cpu_remain_rate > threadhold)
                    + (_B_memory_remain_rate > threadhold) >= 1;
}
// 筛选出几乎满了的服务器，不作为目标服务器。
bool NearlyFull(PurchasedServer *server) {
    double threshold = 0.07; //增大能去掉更多的服务器，减少时间；同时迁移次数会有轻微减少，成本有轻微增加。
    return (1.0 * server->A_remain_core_num / server->total_core_num < threshold || 1.0 * server->A_remain_memory_size / server->total_memory_size < threshold)
                    && (1.0 * server->B_remain_core_num / server->total_core_num < threshold || 1.0 * server->B_remain_memory_size / server->total_memory_size < threshold);
}

vector<MigrationInfo> Migration() {
    int max_migration_num = vm_id2info.size() * 30 / 1000;
    vector<MigrationInfo> migration_infos;
    // unordered_set<int> migrated_vms;
    unordered_set<int> happened_migration_serverID;
    vector<VmIdInfo *> migrating_vms;
    vector<PurchasedServer *> target_servers;
    for (auto server : purchase_servers) {
        if (NeedMigration(server)) {
            for (auto &vm_id : server->A_vm_id) migrating_vms.emplace_back(&vm_id2info[vm_id]);
            for (auto &vm_id : server->B_vm_id) migrating_vms.emplace_back(&vm_id2info[vm_id]);
            for (auto &vm_id : server->AB_vm_id) migrating_vms.emplace_back(&vm_id2info[vm_id]);
        }
        if (!NearlyFull(server)) target_servers.emplace_back(server);
        // else
        //     target_servers.emplace_back(server);

        // else if ((server->A_remain_core_num + server->B_remain_core_num) / 2.0 / server->total_core_num < 0.8 ||
        //            (server->A_remain_memory_size + server->B_remain_memory_size) / 2.0 / server->total_memory_size < 0.8)
        //     target_servers.emplace_back(server);
    }
    /* sort(migrating_vms.begin(), migrating_vms.end(), [](VmIdInfo *a,VmIdInfo *b) {
        return (a->cpu_cores * k1 + a->memory_size * k2) * (a->node == 'C' ? 10 : 1) > (b->cpu_cores * k1 + b->memory_size * k2) * (a->node == 'C' ? 10 : 1);
    }); */
    for (auto &vm_info : migrating_vms) {
        if (migration_infos.size() == max_migration_num) break;
        if (vm_info->node != 'C') {
            PurchasedServer *original_server = vm_info->purchase_server;
            int vm_id = vm_info->vm_id;
            double min_rate = 2.0;
            PurchasedServer* best_server;
            char which_node ;
            for (auto &target_server : target_servers) {
                if (happened_migration_serverID.find(target_server->server_id) != happened_migration_serverID.end()) continue;
                if (!(target_server == original_server && vm_info->node == 'A')
                        && (target_server->A_remain_core_num >= vm_info->cpu_cores && target_server->A_remain_memory_size >= vm_info->memory_size)) {
                    double _cpu_remain_rate = r1 * (target_server->A_remain_core_num - vm_info->cpu_cores) / target_server->total_core_num ;
                    double _memory_remain_rate = r2 * (target_server->A_remain_memory_size - vm_info->memory_size) / target_server->total_memory_size ;
                    if (_cpu_remain_rate + _memory_remain_rate < min_rate) {
                        min_rate = _cpu_remain_rate + _memory_remain_rate;
                        best_server = target_server;
                        which_node = 'A';
                    }
                    // double temp = fabs(_cpu_remain_rate - _memory_remain_rate);
                    // if (temp < min_rate) {
                    //     min_rate = temp;
                    //     best_server = target_server;
                    //     which_node = 'A';
                    // }
                }
                if (!(target_server == original_server && vm_info->node == 'B')
                        && (target_server->B_remain_core_num >= vm_info->cpu_cores && target_server->B_remain_memory_size >= vm_info->memory_size)) {
                    double _cpu_remain_rate = r1 * (target_server->B_remain_core_num- vm_info->cpu_cores) / target_server->total_core_num;
                    double _memory_remain_rate =  r2 * (target_server->B_remain_memory_size - vm_info->memory_size) / target_server->total_memory_size;
                    if (_cpu_remain_rate + _memory_remain_rate < min_rate) {
                        min_rate = _cpu_remain_rate + _memory_remain_rate;
                        best_server = target_server;
                        which_node = 'B';
                    }
                    // double temp = fabs(_cpu_remain_rate - _memory_remain_rate);
                    // if (temp < min_rate) {
                    //     min_rate = temp;
                    //     best_server = target_server;
                    //     which_node = 'B';
                    // }
                }
            }
            if (min_rate!=2.0) {
                total_migration_num++;
                happened_migration_serverID.emplace(original_server->server_id);
                int cpu_cores = vm_info->cpu_cores;
                int memory_size = vm_info->memory_size;
                if (vm_info->node == 'A') {
                    original_server->A_remain_core_num += cpu_cores;
                    original_server->A_remain_memory_size += memory_size;
                    original_server->A_vm_id.erase(vm_id);
                } else {
                    original_server->B_remain_core_num += cpu_cores;
                    original_server->B_remain_memory_size += memory_size;
                    original_server->B_vm_id.erase(vm_id);
                }
                
                if (which_node == 'A') {
                    best_server->A_remain_core_num -= cpu_cores;
                    best_server->A_remain_memory_size -= memory_size;
                    best_server->A_vm_id.insert(vm_id);
                    vm_info->purchase_server = best_server;
                    vm_info->node = 'A';
                    // migrated_vms.insert(vm_id);
                    migration_infos.emplace_back(MigrationInfo(vm_id, best_server, 'A'));
                } else if (which_node == 'B') {
                    best_server->B_remain_core_num -= cpu_cores;
                    best_server->B_remain_memory_size -= memory_size;
                    best_server->B_vm_id.insert(vm_id);
                    vm_info->purchase_server = best_server;
                    vm_info->node = 'B';
                    // migrated_vms.insert(vm_id);
                    migration_infos.emplace_back(MigrationInfo(vm_id, best_server, 'B'));
                }
            }
        } else {
            PurchasedServer *original_server = vm_info->purchase_server;
            int vm_id = vm_info->vm_id;
            double min_rate = 2.0;
            PurchasedServer* best_server;
            for (auto &target_server : target_servers) {
                if (target_server->server_id == original_server->server_id) continue;
                if (happened_migration_serverID.find(target_server->server_id) != happened_migration_serverID.end()) continue;
                if (target_server->A_remain_core_num >= vm_info->cpu_cores && target_server->A_remain_memory_size >= vm_info->memory_size
                        && target_server->B_remain_core_num >= vm_info->cpu_cores && target_server->B_remain_memory_size >= vm_info->memory_size) {
                    double _cpu_remain_rate = r1 * ((target_server->A_remain_core_num - vm_info->cpu_cores) / target_server->total_core_num + (target_server->B_remain_core_num - vm_info->cpu_cores) / target_server->total_core_num) / 2;
                    double _memory_remain_rate = r2 * ((target_server->A_remain_memory_size - vm_info->memory_size) / target_server->total_memory_size + (target_server->B_remain_memory_size - vm_info->memory_size) / target_server->total_memory_size) / 2;
                    if (_cpu_remain_rate + _memory_remain_rate < min_rate) {
                        min_rate = _cpu_remain_rate + _memory_remain_rate;
                        best_server = target_server;
                    }
                    // double temp = fabs(_cpu_remain_rate - _memory_remain_rate);
                    // if (temp < min_rate) {
                    //     min_rate = temp;
                    //     best_server = target_server;
                    // }
                }
            }
            if (min_rate!=2.0) {
                total_migration_num++;
                happened_migration_serverID.emplace(original_server->server_id);
                int cpu_cores = vm_info->cpu_cores;
                int memory_size = vm_info->memory_size;
                original_server->A_remain_core_num += cpu_cores;
                original_server->A_remain_memory_size += memory_size;
                original_server->B_remain_core_num += cpu_cores;
                original_server->B_remain_memory_size += memory_size;
                original_server->AB_vm_id.erase(vm_id);
                
                best_server->A_remain_core_num -= cpu_cores;
                best_server->A_remain_memory_size -= memory_size;
                best_server->B_remain_core_num -= cpu_cores;
                best_server->B_remain_memory_size -= memory_size;
                best_server->AB_vm_id.insert(vm_id);
                vm_info->purchase_server = best_server;
                // migrated_vms.insert(vm_id);
                migration_infos.emplace_back(MigrationInfo(vm_id, best_server, 'C'));
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
        PurchasedServer* best_server;
        for (auto& purchase_server : purchase_servers) {
            if (deployed) {
                break;
            }   //先筛选能用的服务器
            if (purchase_server->A_remain_core_num >= cpu_cores && purchase_server->A_remain_memory_size >= memory_size
            && purchase_server->B_remain_core_num >= cpu_cores && purchase_server->B_remain_memory_size >= memory_size) {
        
                double _cpu_remain_rate = (1.0*(purchase_server->A_remain_core_num - cpu_cores)/purchase_server->total_core_num + 1.0*(purchase_server->B_remain_core_num - cpu_cores)/ purchase_server->total_core_num) / 2;
                double _memory_remain_rate = (1.0*(purchase_server->A_remain_memory_size - memory_size)/purchase_server->total_memory_size + 1.0*(purchase_server->B_remain_memory_size - memory_size) / purchase_server->total_memory_size) / 2;
                if (_cpu_remain_rate + _memory_remain_rate < min_remain_rate) {
                    min_remain_rate = _cpu_remain_rate + _memory_remain_rate;
                    best_server = purchase_server;
                }
            }
        }

        if (min_remain_rate != 2.0) {
            //代表从关机的服务器中选取的
            if (best_server->A_vm_id.size()+best_server->B_vm_id.size()+best_server->AB_vm_id.size() == 0) {
                from_off_2_start.insert(best_server->server_id);
            }
            deployed = true;
            best_server->A_remain_core_num -= cpu_cores;
            best_server->A_remain_memory_size -= memory_size;
            best_server->B_remain_core_num -= cpu_cores;
            best_server->B_remain_memory_size -= memory_size;
            best_server->AB_vm_id.insert(add_data.vm_id);

            VmIdInfo vm_id_info;
            vm_id_info.purchase_server = best_server;
            vm_id_info.vm_name = add_data.vm_name;
            vm_id_info.cpu_cores = cpu_cores;
            vm_id_info.memory_size = memory_size;
            vm_id_info.node = 'C';
            vm_id_info.vm_id = add_data.vm_id;
            vm_id2info[add_data.vm_id] = vm_id_info;
        }


        double min_dense_cost = 99999999999999;
        SoldServer* flag_sold_server;
        for (auto& sold_server : sold_servers) {
            if (deployed)
                break;
            if (sold_server.cpu_cores >= cpu_cores && sold_server.memory_size >= memory_size) {
                double dense_cost;
                if (1.0 * now_req_num / total_req_num < 3.0 / 6) {
                    double use_rate = (1.0 *(cpu_cores) / sold_server.cpu_cores + 1.0 *(memory_size) / sold_server.memory_size) / 2;
                    dense_cost = 1.0 * sold_server.hardware_cost * use_rate;
                }else{
                    dense_cost = sold_server.hardware_cost;
                }
                
                if (dense_cost < min_dense_cost) {
                    min_dense_cost = dense_cost;
                    flag_sold_server = &sold_server;
                }
            }
        }
        if (!deployed) {
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
            vm_id_info.vm_id = add_data.vm_id;
            vm_id2info[add_data.vm_id] = vm_id_info;
        }

        // for (auto& sold_server : sold_servers) {
        //     if (deployed) {
        //         break;
        //     }
        //     if (sold_server.cpu_cores >= cpu_cores && sold_server.memory_size >= memory_size) {
        //         deployed = true;
        //         PurchasedServer* purchase_server = new PurchasedServer;
        //         purchase_server->total_core_num = sold_server.cpu_cores;
        //         purchase_server->total_memory_size = sold_server.memory_size;
        //         purchase_server->A_remain_core_num = sold_server.cpu_cores - cpu_cores;
        //         purchase_server->A_remain_memory_size = sold_server.memory_size - memory_size;
        //         purchase_server->B_remain_core_num = sold_server.cpu_cores - cpu_cores;
        //         purchase_server->B_remain_memory_size = sold_server.memory_size - memory_size;
        //         purchase_server->AB_vm_id.insert(add_data.vm_id);
        //         purchase_server->server_name = sold_server.server_name;
        //         purchase_servers.emplace_back(purchase_server);
        //         purchase_infos[sold_server.server_name].emplace_back(purchase_server);

        //         VmIdInfo vm_id_info;
        //         vm_id_info.purchase_server = purchase_server;
        //         vm_id_info.vm_name = add_data.vm_name;
        //         vm_id_info.cpu_cores = cpu_cores;
        //         vm_id_info.memory_size = memory_size;
        //         vm_id_info.node = "C";
        //         vm_id_info.vm_id = add_data.vm_id;
        //         vm_id2info[add_data.vm_id] = vm_id_info;
        //         //通过虚拟机ID，知道部署再哪个服务器的哪个端口
        //         break;
        //     }
        // }
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
        PurchasedServer* best_server;
        char which_node = 'C';
        for (auto& purchase_server : can_deploy_servers) {
            if (deployed) {
                break;
            }
            bool evaluate_A = evaluate.PurchasedServerA(purchase_server, cpu_cores, memory_size);
            bool evaluate_B = evaluate.PurchasedServerB(purchase_server, cpu_cores, memory_size);
            
            if (evaluate_A&&evaluate_B) {
                double _cpu_remain_rate = 1.0*(purchase_server->A_remain_core_num - cpu_cores)/purchase_server->total_core_num ;
                double _memory_remain_rate = 1.0*(purchase_server->A_remain_memory_size - memory_size)/purchase_server->total_memory_size ;
                // double _cpu_remain_rate = (1.0*(purchase_server->A_remain_core_num - cpu_cores)/purchase_server->total_core_num + 1.0*purchase_server->B_remain_core_num / purchase_server->total_core_num) / 2;
                // double _memory_remain_rate = (1.0*(purchase_server->A_remain_memory_size - memory_size)/purchase_server->total_memory_size + 1.0*purchase_server->B_remain_memory_size / purchase_server->total_memory_size) / 2;
                if (_cpu_remain_rate + _memory_remain_rate < min_remain_rate) {
                    min_remain_rate = _cpu_remain_rate + _memory_remain_rate;
                    best_server = purchase_server;
                    which_node = 'A';
                }

                _cpu_remain_rate =  1.0*(purchase_server->B_remain_core_num- cpu_cores) / purchase_server->total_core_num;
                _memory_remain_rate =  1.0*(purchase_server->B_remain_memory_size- memory_size) / purchase_server->total_memory_size;
                // _cpu_remain_rate = (1.0*(purchase_server->A_remain_core_num )/purchase_server->total_core_num + 1.0*(purchase_server->B_remain_core_num- cpu_cores) / purchase_server->total_core_num) / 2;
                // _memory_remain_rate = (1.0*(purchase_server->A_remain_memory_size)/purchase_server->total_memory_size + 1.0*(purchase_server->B_remain_memory_size- memory_size) / purchase_server->total_memory_size) / 2;
                if (_cpu_remain_rate + _memory_remain_rate < min_remain_rate) {
                    min_remain_rate = _cpu_remain_rate + _memory_remain_rate;
                    best_server = purchase_server;
                    which_node = 'B';
                }
            }else if (evaluate_A) {
                double _cpu_remain_rate = 1.0*(purchase_server->A_remain_core_num - cpu_cores)/purchase_server->total_core_num ;
                double _memory_remain_rate = 1.0*(purchase_server->A_remain_memory_size - memory_size)/purchase_server->total_memory_size;
                // double _cpu_remain_rate = (1.0*(purchase_server->A_remain_core_num - cpu_cores)/purchase_server->total_core_num + 1.0*(purchase_server->B_remain_core_num) / purchase_server->total_core_num) / 2;
                // double _memory_remain_rate = (1.0*(purchase_server->A_remain_memory_size - memory_size)/purchase_server->total_memory_size + 1.0*(purchase_server->B_remain_memory_size) / purchase_server->total_memory_size) / 2;
                if (_cpu_remain_rate + _memory_remain_rate < min_remain_rate) {
                    min_remain_rate = _cpu_remain_rate + _memory_remain_rate;
                    best_server = purchase_server;
                    which_node = 'A';
                }
            }else if (evaluate_B) {
                double _cpu_remain_rate = 1.0*(purchase_server->B_remain_core_num- cpu_cores) / purchase_server->total_core_num;
                double _memory_remain_rate = 1.0*(purchase_server->B_remain_memory_size- memory_size) / purchase_server->total_memory_size;
                // double _cpu_remain_rate = (1.0*(purchase_server->A_remain_core_num )/purchase_server->total_core_num + 1.0*(purchase_server->B_remain_core_num- cpu_cores) / purchase_server->total_core_num) / 2;
                // double _memory_remain_rate = (1.0*(purchase_server->A_remain_memory_size)/purchase_server->total_memory_size + 1.0*(purchase_server->B_remain_memory_size- memory_size) / purchase_server->total_memory_size) / 2;
                //  double _cpu_remain_rate = max(1.0*(purchase_server->A_remain_core_num )/purchase_server->total_core_num , 1.0*(purchase_server->B_remain_core_num - cpu_cores)/ purchase_server->total_core_num) ;
                // double _memory_remain_rate =  max(1.0*(purchase_server->A_remain_memory_size )/purchase_server->total_memory_size , 1.0*(purchase_server->B_remain_memory_size - memory_size) / purchase_server->total_memory_size) ;
                if (_cpu_remain_rate + _memory_remain_rate < min_remain_rate) {
                    min_remain_rate = _cpu_remain_rate + _memory_remain_rate;
                    best_server = purchase_server;
                    which_node = 'B';
                }
            }
        }
        if (min_remain_rate !=2.0) {
            //代表从关机的服务器中选取的
            if (best_server->A_vm_id.size()+best_server->B_vm_id.size()+best_server->AB_vm_id.size() == 0) {
                from_off_2_start.insert(best_server->server_id);
            }
            if (which_node == 'A') {
                deployed = true;
                best_server->A_remain_core_num -= cpu_cores;
                best_server->A_remain_memory_size -= memory_size;
                best_server->A_vm_id.insert(add_data.vm_id);

                VmIdInfo vm_id_info;
                vm_id_info.purchase_server = best_server;
                vm_id_info.vm_name = add_data.vm_name;
                vm_id_info.cpu_cores = cpu_cores;
                vm_id_info.memory_size = memory_size;
                vm_id_info.node = 'A';
                vm_id_info.vm_id = add_data.vm_id;
                vm_id2info[add_data.vm_id] = vm_id_info;
            } else if (which_node == 'B') {
                deployed = true;
                best_server->B_remain_core_num -= cpu_cores;
                best_server->B_remain_memory_size -= memory_size;
                best_server->B_vm_id.insert(add_data.vm_id);

                VmIdInfo vm_id_info;
                vm_id_info.purchase_server = best_server;
                vm_id_info.vm_name = add_data.vm_name;
                vm_id_info.cpu_cores = cpu_cores;
                vm_id_info.memory_size = memory_size;
                vm_id_info.node = 'B';
                vm_id_info.vm_id = add_data.vm_id;
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
                if (1.0 * now_req_num / total_req_num < 3.0 / 6) {
                    double use_rate = (1.0 *(cpu_cores) / sold_server.cpu_cores + 1.0 *(memory_size) / sold_server.memory_size) / 2;
                    dense_cost = 1.0 * sold_server.hardware_cost * use_rate;
                }else{
                    dense_cost = sold_server.hardware_cost;
                }
                if (dense_cost < min_dense_cost) {
                    min_dense_cost = dense_cost;
                    flag_sold_server = &sold_server;
                }
            }
        }

        if (!deployed) {
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
            vm_id_info.vm_id = add_data.vm_id;
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

void Compute_Power_Cost() {
    for (auto& server:purchase_servers) {
        if (server->A_vm_id.size() + server->B_vm_id.size() + server->AB_vm_id.size()!=0) {
            //当天结束时候有虚拟机的服务器
            total_power_cost += server_name2info[server->server_name].daily_energy_cost;
        }else{
            //当天结束时关机，但是当天开过机的服务器，算电费
            if (from_off_2_start.find(server->server_id) != from_off_2_start.end()) {
                total_power_cost += server_name2info[server->server_name].daily_energy_cost;
            }
        }
    }
}

void SolveProblem() {
    Cmp cmp;
    sort(sold_servers.begin(), sold_servers.end(), cmp.SoldServers);
    for (int i = 0; i < total_days_num; ++i) {
        // vector<MigrationInfo> migration_infos;
        vector<MigrationInfo> migration_infos = Migration();
        vector<RequestData> intraday_requests = request_datas.front();
        request_datas.pop();
        int request_num = intraday_requests.size();
        vector<int> vm_ids;
        for (int j = 0; j < request_num; ++j) {       //把请求添加的虚拟机ID有序存起来   !!可能有bug
            if (intraday_requests[j].operation == "add") {
                vm_ids.emplace_back(intraday_requests[j].vm_id);
            }
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

void PrintCostInfo() {
    cout<<"Server Num : "<<purchase_servers.size()<<endl;
    cout<<"Total Migration Num : " <<total_migration_num<<endl;
    #ifdef PRINTINFO
    cout<<"Time: "<<double(_end - _start) / CLOCKS_PER_SEC<< " s"<<endl;
    #endif
    cout<<"Total Cost : "<< to_string(total_server_cost)<<" + "<<to_string(total_power_cost) <<" = "<<total_power_cost + total_server_cost<<endl;
}

int main(int argc, char* argv[]) {
#ifdef REDIRECT
    freopen("training-2.txt", "r", stdin);
    freopen("out1.txt", "w", stdout);
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
    fclose(stdin);
    fclose(stdout);
#endif
    return 0;
}
