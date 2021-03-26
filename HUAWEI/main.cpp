#include "H.h"

vector<SoldServer> sold_servers;
unordered_map<string, SoldVm> vm_name2info;
unordered_map<int, VmIdInfo> vm_id2info;
vector<vector<RequestData>> request_datas;
vector<PurchasedServer*> purchase_servers;
unordered_map<string, vector<PurchasedServer*>> purchase_infos;
int number = 0; //给服务器编号
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
void ParseRequest() {
    int day_num;
    cin >> day_num;
    int operation_num;
    string operation, vm_name, vm_id;
    for (int i = 0; i < day_num; ++i) {
        cin >> operation_num;
        vector<RequestData> request_data;
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
        request_datas.emplace_back(request_data);
    }
}
void ParseInput() {
    ParseServerInfo();
    ParseVmInfo();
    ParseRequest();
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
void Migration() {

}
void AddVm(AddData& add_data) {
    bool isDeploy = false;
    Evaluate evaluate;  //创建一个评价类的实例
    int deployment_way = add_data.deployment_way;
    if (deployment_way == 1) { //双节点部署
        int cpu_cores = add_data.cpu_cores;
        int memory_size = add_data.memory_size;
        for (auto& purchase_server : purchase_servers) {
            if (isDeploy) {
                break;
            }
            if (evaluate.PurchasedServerAB(purchase_server, cpu_cores, memory_size)) {
                isDeploy = true;
                purchase_server->A_remain_core_num -= cpu_cores;
                purchase_server->A_remain_memory_size -= memory_size;
                purchase_server->B_remain_core_num -= cpu_cores;
                purchase_server->B_remain_memory_size -= memory_size;
                purchase_server->AB_vm_id.insert(add_data.vm_id);

                vm_id2info[add_data.vm_id].purchase_server = purchase_server;
                vm_id2info[add_data.vm_id].vm_name = add_data.vm_name;
                vm_id2info[add_data.vm_id].cpu_cores = cpu_cores;
                vm_id2info[add_data.vm_id].memory_size = memory_size;
                vm_id2info[add_data.vm_id].node = "C";
                break;
            }
        }
        for (auto& sold_server : sold_servers) {
            if (isDeploy) {
                break;
            }
            if (sold_server.cpu_cores >= cpu_cores && sold_server.memory_size >= memory_size) {
                isDeploy = true;
                PurchasedServer* purchase_server = new PurchasedServer;
                purchase_server->total_core_num = sold_server.cpu_cores;
                purchase_server->total_memory_size = sold_server.memory_size;
                purchase_server->A_remain_core_num = sold_server.cpu_cores - cpu_cores;
                purchase_server->A_remain_memory_size = sold_server.memory_size - memory_size;
                purchase_server->B_remain_core_num = sold_server.cpu_cores - cpu_cores;
                purchase_server->B_remain_memory_size = sold_server.memory_size - memory_size;
                purchase_server->AB_vm_id.insert(add_data.vm_id);
                purchase_server->server_name = sold_server.server_name;
                purchase_servers.emplace_back(purchase_server);
                purchase_infos[sold_server.server_name].emplace_back(purchase_server);

                vm_id2info[add_data.vm_id].purchase_server = purchase_server;
                vm_id2info[add_data.vm_id].vm_name = add_data.vm_name;
                vm_id2info[add_data.vm_id].cpu_cores = cpu_cores;
                vm_id2info[add_data.vm_id].memory_size = memory_size;
                vm_id2info[add_data.vm_id].node = "C";
                //通过虚拟机ID，知道部署再哪个服务器的哪个端口
                break;
            }
        }
    } else {       //单节点部署
        int cpu_cores = add_data.cpu_cores;
        int memory_size = add_data.memory_size;
        for (auto& purchase_server : purchase_servers) {
            if (isDeploy) {
                break;
            }
            if (evaluate.PurchasedServerA(purchase_server, cpu_cores, memory_size)) {
                isDeploy = true;
                purchase_server->A_remain_core_num -= cpu_cores;
                purchase_server->A_remain_memory_size -= memory_size;
                purchase_server->A_vm_id.insert(add_data.vm_id);

                vm_id2info[add_data.vm_id].purchase_server = purchase_server;
                vm_id2info[add_data.vm_id].vm_name = add_data.vm_name;
                vm_id2info[add_data.vm_id].cpu_cores = cpu_cores;
                vm_id2info[add_data.vm_id].memory_size = memory_size;
                vm_id2info[add_data.vm_id].node = "A";
                break;
            } else if (evaluate.PurchasedServerB(purchase_server, cpu_cores, memory_size)) {
                isDeploy = true;
                purchase_server->B_remain_core_num -= cpu_cores;
                purchase_server->B_remain_memory_size -= memory_size;
                purchase_server->B_vm_id.insert(add_data.vm_id);

                vm_id2info[add_data.vm_id].purchase_server = purchase_server;
                vm_id2info[add_data.vm_id].vm_name = add_data.vm_name;
                vm_id2info[add_data.vm_id].cpu_cores = cpu_cores;
                vm_id2info[add_data.vm_id].memory_size = memory_size;
                vm_id2info[add_data.vm_id].node = "B";
                break;
            } else {
                continue;
            }
        }
        for (auto& sold_server : sold_servers) {
            if (isDeploy) {
                break;
            }
            if (sold_server.cpu_cores >= cpu_cores && sold_server.memory_size >= memory_size) {
                isDeploy = true;
                PurchasedServer* purchase_server = new PurchasedServer;
                purchase_server->total_core_num = sold_server.cpu_cores;
                purchase_server->total_memory_size = sold_server.memory_size;
                purchase_server->A_remain_core_num = sold_server.cpu_cores - cpu_cores;
                purchase_server->A_remain_memory_size = sold_server.memory_size - memory_size;
                purchase_server->B_remain_core_num = sold_server.cpu_cores;
                purchase_server->B_remain_memory_size = sold_server.memory_size;
                purchase_server->A_vm_id.insert(add_data.vm_id);
                purchase_server->server_name = sold_server.server_name;
                purchase_servers.emplace_back(purchase_server);
                purchase_infos[sold_server.server_name].emplace_back(purchase_server);

                vm_id2info[add_data.vm_id].purchase_server = purchase_server;
                vm_id2info[add_data.vm_id].vm_name = add_data.vm_name;
                vm_id2info[add_data.vm_id].cpu_cores = cpu_cores;
                vm_id2info[add_data.vm_id].memory_size = memory_size;
                vm_id2info[add_data.vm_id].node = "A";
                //通过虚拟机ID，知道部署再哪个服务器的哪个端口
                break;
            }
        }
    }
}
void DeleteVm(int vm_id) {
    PurchasedServer* purchase_server = vm_id2info[vm_id].purchase_server;
    string node = vm_id2info[vm_id].node;
    int cpu_cores = vm_id2info[vm_id].cpu_cores;
    int memory_size = vm_id2info[vm_id].memory_size;
    if (node == "A") {
        purchase_server->A_remain_core_num += cpu_cores;
        purchase_server->A_remain_memory_size += memory_size;
        purchase_server->A_vm_id.erase(vm_id);
    } else if (node == "B") {
        purchase_server->B_remain_core_num += cpu_cores;
        purchase_server->B_remain_memory_size += memory_size;
        purchase_server->B_vm_id.erase(vm_id);
    } else if (node == "C") {
        purchase_server->A_remain_core_num += cpu_cores;
        purchase_server->A_remain_memory_size += memory_size;
        purchase_server->B_remain_core_num += cpu_cores;
        purchase_server->B_remain_memory_size += memory_size;
        purchase_server->AB_vm_id.erase(vm_id);
    } else {
        cerr << "DeleError";
    }
}
void Print(vector<int>& vm_ids) {
    int purchase_type_num = purchase_infos.size();
    cout << "(purchase, " << purchase_type_num << ')' << endl;
    for (auto& purchase_info : purchase_infos) {
        cout << '(' << purchase_info.first << ", " << purchase_info.second.size() << ')' << endl;
    }
    cout << "(migration, 0)" << endl;
    for (auto& vm_id : vm_ids) {
        string node = vm_id2info[vm_id].node;
        if (node == "C") {
            cout << '(' << vm_id2info[vm_id].purchase_server->server_id << ')' << endl;
        } else {
            cout << '(' << vm_id2info[vm_id].purchase_server->server_id << ", " << node << ')' << endl;
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
    int day_num = request_datas.size();
    for (int i = 0; i < day_num; ++i) {
        Migration();
        int request_num = request_datas[i].size();
        vector<int> vm_ids;
        for (int j = 0; j < request_num; ++j) {       //把请求添加的虚拟机ID有序存起来   !!可能有bug
            if (request_datas[i][j].operation == "add") {
                vm_ids.emplace_back(request_datas[i][j].vm_id);
            }
        }
        for (int j = 0; j < request_num; ++j) {
            string operation = request_datas[i][j].operation;
            vector<AddData> continuous_add_datas;
            while (operation == "add" && j < request_num) {
                string vm_name = request_datas[i][j].vm_name;
                int deployment_way = vm_name2info[vm_name].deployment_way;
                int cpu_cores = vm_name2info[vm_name].cpu_cores;
                int memory_size = vm_name2info[vm_name].memory_size;
                int vm_id = request_datas[i][j].vm_id;
                continuous_add_datas.emplace_back(deployment_way, cpu_cores, memory_size, vm_id, vm_name);
                ++j;
                if (j == request_num) {
                    break;
                }
                operation = request_datas[i][j].operation;
            }
            sort(continuous_add_datas.begin(), continuous_add_datas.end(), cmp.ContinuousADD);
            for (auto& add_data : continuous_add_datas) {
                AddVm(add_data);
            }
            if (j == request_num) {
                    break;
            }
            if (operation == "del") {
                int vm_id = request_datas[i][j].vm_id;
                DeleteVm(vm_id);
            }
        }
        Numbering(); //给购买了的服务器编号
        Print(vm_ids);
        purchase_infos.erase(purchase_infos.begin(), purchase_infos.end());
    }
}

int main(int argc, char* argv[]) {
#ifdef REDIRECT
    freopen("training-1.txt", "r", stdin);
    freopen("out1.txt", "w", stdout); 
#endif
    ParseInput();
    SolveProblem();
#ifdef REDIRECT
    fclose(stdin);
    fclose(stdout);
#endif
    return 0;
}