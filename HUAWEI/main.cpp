#include "H.h"
//#define TEST_PARSEINPUT
#define REDIRECT
vector<SoldServer> sold_servers;
unordered_map<string, SoldVM> VM_type2info;
vector<vector<RequestData>> request_datas;

void ParseServerInfo() {
    int server_num;
    cin >> server_num;
    string server_type, cpu_core, memory_size, hardware_cost, daily_energy_cost;
    for (int i = 0; i < server_num; ++i) {
        SoldServer sold_server;
        cin >> server_type >> cpu_core >> memory_size >> hardware_cost >> daily_energy_cost;
        sold_server.server_type = server_type.substr(1, server_type.size() - 2);
        sold_server.cpu_cores = stoi(cpu_core.substr(0, cpu_core.size() - 1)) / 2;
        sold_server.memory_size = stoi(memory_size.substr(0, memory_size.size() - 1)) / 2;
        sold_server.hardware_cost = stoi(hardware_cost.substr(0, hardware_cost.size() - 1));
        sold_server.daily_energy_cost = stoi(daily_energy_cost.substr(0, daily_energy_cost.size() - 1));   
        sold_servers.emplace_back(sold_server);   //可优化，不需要sold_server
    }
}
void ParseVmInfo() {
    int VM_num;
    cin >> VM_num;
    string VM_type, cpu_core, memory_size, deployment_way;
    for (int i = 0; i < VM_num; ++i) {
        SoldVM sold_VM;
        cin >> VM_type >> cpu_core >> memory_size >> deployment_way;
        VM_type = VM_type.substr(1, VM_type.size() - 2);
        sold_VM.cpu_cores = stoi(cpu_core.substr(0, cpu_core.size() - 1));
        sold_VM.memory_size = stoi(memory_size.substr(0, memory_size.size() - 1));
        sold_VM.deployment_way = stoi(deployment_way.substr(0, deployment_way.size() - 1));
        if (sold_VM.deployment_way == 1) {
            sold_VM.cpu_cores /= 2;
            sold_VM.memory_size /= 2;
        }
        VM_type2info[VM_type] = sold_VM;
    }
}
void ParseRequest() {
    int day_num;
    cin >> day_num;
    int operation_num;
    string operation, VM_type, VM_ID;
    for (int i = 0; i < day_num; ++i) {
        cin >> operation_num;
        vector<RequestData> request_data;
        for (int j = 0; j < operation_num; ++j) {
            cin >> operation;
            operation = operation.substr(1, operation.size() - 2);
            if (operation == "add") {
                RequestData data;
                data.operation = "add";
                cin >> VM_type >> VM_ID;
                data.VM_type = VM_type.substr(0, VM_type.size() - 1);
                data.VM_ID = stoi(VM_ID.substr(0, VM_ID.size() - 1));
                request_data.emplace_back(data);
            } else if (operation == "del") {
                RequestData data;
                data.operation = "del";
                cin >> VM_ID;
                data.VM_ID = stoi(VM_ID.substr(0, VM_ID.size() - 1));
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
    for (auto sold_server : sold_servers) {
        cout << sold_server.server_type << " " << sold_server.cpu_cores << " " << sold_server.memory_size 
        << " " << sold_server.hardware_cost << " " << sold_server.daily_energy_cost << endl;
    }
    cout << VM_type2info.size() << endl;
    for (auto vm : VM_type2info) {
        cout << vm.first << " " << vm.second.cpu_cores << " " << vm.second.memory_size << " "
        << vm.second.deployment_way << endl;
    }
    cout << request_datas.size() << endl;
    for (auto request : request_datas) {
        cout << request.size() << endl;
        for (auto data : request) {
            if (data.operation == "add") {
                cout << data.operation << " " << data.VM_type << " " << data.VM_ID << endl;
            } else if (data.operation == "del") {
                cout << data.operation << " " << data.VM_ID << endl;
            } else {
                cerr << "operation error!";
            }
        }
    }
#endif
}

void SolveProblem() {

    if (/*add*/) {

    } else {  // del

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