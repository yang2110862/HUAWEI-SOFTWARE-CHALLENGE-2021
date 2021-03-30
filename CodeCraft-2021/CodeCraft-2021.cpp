#include "H.h"

vector<SoldServer> sold_servers;
unordered_map<string, SoldVm> vm_name2info;
unordered_map<int, VmIdInfo> vm_id2info;
queue<vector<RequestData>> request_datas;
vector<PurchasedServer*> purchase_servers;
unordered_map<string, vector<PurchasedServer*>> purchase_infos;

int total_days_num, foreseen_days_num; //总天数和可预知的天数
int total_req_num  = 0;
int now_req_num = 0;

int number = 0; //给服务器编号
//ifstream cin("training-2.txt");
//ofstream cout("t2.txt");
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
bool NeedMigration(PurchasedServer *server) {
    if(server->A_vm_id.size() + server->B_vm_id.size() + server->AB_vm_id.size() == 0) return false;

    double _A_cpu_remain_rate = 1.0 * server->A_remain_core_num / server->total_core_num;
    double _B_cpu_remain_rate = 1.0 * server->B_remain_core_num / server->total_core_num;
    double _A_memory_remain_rate = 1.0 * server->A_remain_memory_size / server->total_memory_size;
    double _B_memory_remain_rate = 1.0 * server->B_remain_memory_size / server->total_memory_size;
    double threadhold = 0.23;
    if((_A_cpu_remain_rate > threadhold ) + (_A_memory_remain_rate > threadhold) + (_B_cpu_remain_rate > threadhold) + (_B_memory_remain_rate>threadhold) >=2)
        return true;
    else
        return false;
}
//低于0.5 往高于0.5迁！！！


vector<MigrationInfo> Migration() {
    int max_migration_num = vm_id2info.size() * 5 / 1000;
    vector<MigrationInfo> migration_infos;
    // unordered_set<int> migrated_vms;
    unordered_set<int> happened_migtation_serverID;
    vector<PurchasedServer *> original_servers, target_servers;
    for (auto server : purchase_servers) {
        target_servers.emplace_back(server);
        if (NeedMigration(server)) original_servers.emplace_back(server);
        // else
        //     target_servers.emplace_back(server);

        // else if ((server->A_remain_core_num + server->B_remain_core_num) / 2.0 / server->total_core_num < 0.8 ||
        //            (server->A_remain_memory_size + server->B_remain_memory_size) / 2.0 / server->total_memory_size < 0.8)
        //     target_servers.emplace_back(server);
    }
    sort(original_servers.begin(),original_servers.end(),[](PurchasedServer* a,PurchasedServer*b) {
        return a->A_vm_id.size() + a->B_vm_id.size() + a->AB_vm_id.size() <b->A_vm_id.size() + b->B_vm_id.size() + b->AB_vm_id.size() ;
    });

    for (auto &original_server : original_servers) {
        if (migration_infos.size() == max_migration_num) return migration_infos;
        Evaluate evaluate;
        unordered_set<int> a_vms = original_server->A_vm_id;
        for (auto &vm_id : a_vms) {
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
                // if(1.0 * now_req_num / total_req_num < 3.0 / 5 &&1.0 * now_req_num / total_req_num > 1.0 / 5 ) {
                //     int _threadhold = 100;
                //     int _threadhold_rate = 20;
                //     int _A_remain_cpu = purchase_server->A_remain_core_num - cpu_cores;
                //     int _B_remain_cpu = purchase_server->B_remain_core_num - cpu_cores;
                //     int _A_remain_memory = purchase_server->A_remain_memory_size - memory_size;
                //     int _B_remain_memory = purchase_server->B_remain_memory_size - memory_size;
                //     if( _A_remain_memory ==0) {
                //         if(_A_remain_cpu >= _threadhold) continue;
                //     }
                //     if(_B_remain_memory ==0) {
                //         if(_B_remain_cpu >= _threadhold) continue;
                //     }
                //     if(1.0 * _A_remain_cpu  / _A_remain_memory > _threadhold_rate || 1.0 * _A_remain_cpu / _A_remain_memory < 1.0 / _threadhold_rate ||
                //         1.0 * _B_remain_cpu  / _B_remain_memory > _threadhold_rate || 1.0 * _B_remain_cpu / _B_remain_memory < 1.0 / _threadhold_rate ) {
                //             continue;
                //     }
                // }
                

                double _cpu_remain_rate = (1.0*(purchase_server->A_remain_core_num - cpu_cores)/purchase_server->total_core_num + 1.0*(purchase_server->B_remain_core_num - cpu_cores)/ purchase_server->total_core_num) / 2;
                double _memory_remain_rate = (1.0*(purchase_server->A_remain_memory_size - memory_size)/purchase_server->total_memory_size + 1.0*(purchase_server->B_remain_memory_size - memory_size) / purchase_server->total_memory_size) / 2;
                if(_cpu_remain_rate + _memory_remain_rate < min_remain_rate) {
                    min_remain_rate = _cpu_remain_rate + _memory_remain_rate;
                    flag_server = purchase_server;
                }
            }
        }

        if(min_remain_rate != 2.0) {
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
                if(1.0 * now_req_num / total_req_num < 3.0 / 6) {
                    double use_rate = (1.0 *(cpu_cores) / sold_server.cpu_cores + 1.0 *(memory_size) / sold_server.memory_size) / 2;
                    dense_cost = 1.0 * sold_server.hardware_cost * use_rate;
                }else{
                    dense_cost = sold_server.hardware_cost;
                }
                
                if(dense_cost < min_dense_cost) {
                    min_dense_cost = dense_cost;
                    flag_sold_server = &sold_server;
                }
            }
        }
        if(!deployed) {
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
                // double _cpu_remain_rate = (1.0*(purchase_server->A_remain_core_num - cpu_cores)/purchase_server->total_core_num + 1.0*purchase_server->B_remain_core_num / purchase_server->total_core_num) / 2;
                // double _memory_remain_rate = (1.0*(purchase_server->A_remain_memory_size - memory_size)/purchase_server->total_memory_size + 1.0*purchase_server->B_remain_memory_size / purchase_server->total_memory_size) / 2;
                if(_cpu_remain_rate + _memory_remain_rate < min_remain_rate) {
                    min_remain_rate = _cpu_remain_rate + _memory_remain_rate;
                    flag_server = purchase_server;
                    which_node = 'A';
                }

                _cpu_remain_rate =  1.0*(purchase_server->B_remain_core_num- cpu_cores) / purchase_server->total_core_num;
                _memory_remain_rate =  1.0*(purchase_server->B_remain_memory_size- memory_size) / purchase_server->total_memory_size;
                // _cpu_remain_rate = (1.0*(purchase_server->A_remain_core_num )/purchase_server->total_core_num + 1.0*(purchase_server->B_remain_core_num- cpu_cores) / purchase_server->total_core_num) / 2;
                // _memory_remain_rate = (1.0*(purchase_server->A_remain_memory_size)/purchase_server->total_memory_size + 1.0*(purchase_server->B_remain_memory_size- memory_size) / purchase_server->total_memory_size) / 2;
                if(_cpu_remain_rate + _memory_remain_rate < min_remain_rate) {
                    min_remain_rate = _cpu_remain_rate + _memory_remain_rate;
                    flag_server = purchase_server;
                    which_node = 'B';
                }
            }else if (evaluate_A) {
                double _cpu_remain_rate = 1.0*(purchase_server->A_remain_core_num - cpu_cores)/purchase_server->total_core_num ;
                double _memory_remain_rate = 1.0*(purchase_server->A_remain_memory_size - memory_size)/purchase_server->total_memory_size;
                // double _cpu_remain_rate = (1.0*(purchase_server->A_remain_core_num - cpu_cores)/purchase_server->total_core_num + 1.0*(purchase_server->B_remain_core_num) / purchase_server->total_core_num) / 2;
                // double _memory_remain_rate = (1.0*(purchase_server->A_remain_memory_size - memory_size)/purchase_server->total_memory_size + 1.0*(purchase_server->B_remain_memory_size) / purchase_server->total_memory_size) / 2;
                if(_cpu_remain_rate + _memory_remain_rate < min_remain_rate) {
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
                if(_cpu_remain_rate + _memory_remain_rate < min_remain_rate) {
                    min_remain_rate = _cpu_remain_rate + _memory_remain_rate;
                    flag_server = purchase_server;
                    which_node = 'B';
                }
            }
        }
        if(min_remain_rate !=2.0) {
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
                if(1.0 * now_req_num / total_req_num < 3.0 / 6) {
                    double use_rate = (1.0 *(cpu_cores) / sold_server.cpu_cores + 1.0 *(memory_size) / sold_server.memory_size) / 2;
                    dense_cost = 1.0 * sold_server.hardware_cost * use_rate;
                }else{
                    dense_cost = sold_server.hardware_cost;
                }
                if(dense_cost < min_dense_cost) {
                    min_dense_cost = dense_cost;
                    flag_sold_server = &sold_server;
                }
            }
        }

        if(!deployed) {
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
        //         purchase_server->B_remain_core_num = sold_server.cpu_cores;
        //         purchase_server->B_remain_memory_size = sold_server.memory_size;

        //         purchase_server->A_vm_id.insert(add_data.vm_id);
        //         purchase_server->server_name = sold_server.server_name;
        //         purchase_servers.emplace_back(purchase_server);
        //         purchase_infos[sold_server.server_name].emplace_back(purchase_server);

        //         VmIdInfo vm_id_info;
        //         vm_id_info.purchase_server = purchase_server;
        //         vm_id_info.vm_name = add_data.vm_name;
        //         vm_id_info.cpu_cores = cpu_cores;
        //         vm_id_info.memory_size = memory_size;
        //         vm_id_info.node = "A";
        //         vm_id2info[add_data.vm_id] = vm_id_info;
        //         //通过虚拟机ID，知道部署再哪个服务器的哪个端口
        //         break;
        //     }
        // }
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
        }
    }
}
void SolveProblem() {
    Cmp cmp;
    sort(sold_servers.begin(), sold_servers.end(), cmp.SoldServers);
    for (int i = 0; i < total_days_num; ++i) {
        vector<MigrationInfo> migration_infos;
        // vector<MigrationInfo> migration_infos = Migration();
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
        if (i < total_days_num - foreseen_days_num) ParseRequest(1);
    }
}

int main(int argc, char* argv[]) {
#ifdef REDIRECT
    freopen("/Users/wangtongling/Desktop/training-data/training-2.txt", "r", stdin);
    // freopen("out1.txt", "w", stdout);
#endif
    ParseInput();
    SolveProblem();
#ifdef REDIRECT
    fclose(stdin);
    fclose(stdout);
#endif
    return 0;
}