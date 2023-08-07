#include <iostream>
#include <cstdio>
#include <cstring>
#include "bptree.hpp"

int main() {
    BPTree<std::string, int> bpTree("test");
    std::pair<std::string, int> val;
    int cnt;
    std::string cmd;
    std::cin >> cnt;
    for (int i = 1; i <= cnt; i++) {
        std::cin >> cmd;
        if (cmd[0] == 'i') {
            std::cin >> val.first >> val.second;
            bpTree.insert(val);
        } else if (cmd[0] == 'f') {
            std::cin >> val.first;
            std::vector<int> ans = bpTree.find(val.first);
            if (!ans.empty()) {
                for (int i = 0; i < ans.size() - 1; i++) std::cout << ans[i] << ' ';
                std::cout << ans[ans.size() - 1] << std::endl;
            } else puts("null");
        } else if (cmd[0] == 'd') {
            std::cin >> val.first >> val.second;
            bpTree.remove(val);
        }

    }
    return 0;
}