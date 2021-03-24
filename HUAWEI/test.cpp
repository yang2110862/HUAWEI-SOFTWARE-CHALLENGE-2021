#include <vector>
#include <algorithm>
#include <iostream>
using namespace std;
class CMP {
public:
    static bool t(int a, int b) {
        return a > b;
    }
};
int main() {
    CMP cmp;
    vector<int> x{2,1};
    sort(x.begin(), x.end(), cmp.t);
    for (auto a : x) {
        cout << a;
    }
    return 0;
}