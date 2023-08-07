#include <iostream>
#include <cstdio>
#include <cstring>
#include <vector>
#include "bptree.hpp"

struct String {
    char index[65];

    String(const String &other) {
        for (int i = 0; i < 65; i++)index[i] = other.index[i];
    }

    String() = default;

    friend bool operator>(const String &lhs, const String &rhs) {
        return std::string(lhs.index) > std::string(rhs.index);
    }

    friend bool operator>=(const String &lhs, const String &rhs) {
        return std::string(lhs.index) >= std::string(rhs.index);
    }

    friend bool operator<(const String &lhs, const String &rhs) {
        return std::string(lhs.index) < std::string(rhs.index);
    }

    friend bool operator<=(const String &lhs, const String &rhs) {
        return std::string(lhs.index) <= std::string(rhs.index);
    }

    friend bool operator==(const String &lhs, const String &rhs) {
        return std::string(lhs.index) == std::string(rhs.index);
    }

    friend bool operator!=(const String &lhs, const String &rhs) {
        return std::string(lhs.index) != std::string(rhs.index);
    }

    friend std::ostream &operator<<(std::ostream &os, const String &obj) {
        os << obj.index;
        return os;
    }


};

int main() {
    BPTree<std::string, int, 100, 100> bpTree("test");
    std::pair<std::string, int> val;
    int operationCount;
    std::string cmd;
    int data;
    std::cin >> operationCount;
    for (int i = 1; i <= operationCount; i++) {
        std::cin >> cmd;
        if (cmd[0] == 'i') {  // insert
            std::cin >> val.first >> val.second;
            bpTree.insert(val);
        } else if (cmd[0] == 'f') {  // find
            std::cin >> val.first;
            std::vector<int> ans = bpTree.find(val.first);
            if (!ans.empty()) {
                for (int i = 0; i < ans.size() - 1; i++)printf("%d ", ans[i]);
                printf("%d\n",ans[ans.size()-1]);
            } else puts("null");

        } else if (cmd[0] == 'd') {  // delete
            std::cin >> val.first >> val.second;
            bpTree.remove(val);
        }
    }
    return 0;
}