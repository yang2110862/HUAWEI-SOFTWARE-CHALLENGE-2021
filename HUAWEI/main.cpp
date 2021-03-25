#include "H.h"

vector<SoldServer> sold_servers;
unordered_map<string, SoldVm> vm_name2info;
unordered_map<int, VmIdInfo> vm_id2info;
vector<vector<RequestData>> request_datas;
vector<PurchasedServer*> purchase_servers;
unordered_map<string, vector<PurchasedServer*>> purchase_infos;
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
#ifdef TEST_PaRSEINPUT
    cout << sold_servers.size() << endl;
    for (auto sold_server : sold_servers) {
        cout << sold_server.server_name << " " << sold_server.cpu_cores << " " << sold_server.memory_size 
        << " " << sold_server.hardware_cost << " " << sold_server.daily_energy_cost << endl;
    }
    cout << vm_name2info.size() << endl;
    for (auto vm : vm_name2info) {
        cout << vm.first << " " << vm.second.cpu_cores << " " << vm.second.memory_size << " "
        << vm.second.deployment_way << endl;
    }
    cout << request_datas.size() << endl;
    for (auto request : request_datas) {
        cout << request.size() << endl;
        for (auto data : request) {
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
void addVm() {

}
void DeleteVm() {

}
void SolveProblem() {
    CMP cmp;
    int day_num = request_datas.size();
    for (int i = 0; i < day_num; ++i) {
        Migration();
        int request_num = request_datas[i].size();
        for (int j = 0; j < request_num; ++j) {
            string operation = request_datas[i][j].operation;
            if (operation == "add") {
                string vm_name = request_datas[i][j].vm_name;
                int vm_id = request_datas[i][j].vm_id;
                vector<RequestData> continuous_add_requests;
                while (j < request_num && request_datas[i][j].operation == "add") {
                    continuous_add_requests.emplace_back(request_datas[i][j]);
                    ++j;
                }
                --j;
                sort(continuous_add_requests.begin(), continuous_add_requests.end(), cmp.Continuous);
                for (auto request : continuous_add_requests) {
                    addVm();
                }
            }
            else {
                DeleteVm();
            }
        }
    }
}
void Print() {

}
int main(int argc, char* argv[]) {
#ifdef REDIRECT
    freopen("train.txt", "r", stdin);
    freopen("out.txt", "w", stdout); 
#endif

    ParseInput();
    SolveProblem();
    //Print();

#ifdef REDIRECT
    fclose(stdin);
    fclose(stdout);
#endif
    return 0;
}