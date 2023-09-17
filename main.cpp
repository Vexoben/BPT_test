#include <iostream>
#include <cstdio>
#include <cstring>
#include "BPlusTree.h"
#include "List.h"
#include "Pair.h"

// 需要使用一个定长的字符串作为索引，而不是使用std::string
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
    trainsys::BPlusTree<String, int> bpTree("test");
    String key;
    int value;
    int cnt;
    char cmd[10];
    std::cin >> cnt;
    for (int i = 1; i <= cnt; i++) {
        std::cin >> cmd;
        if (cmd[0] == 'i') {
            std::cin >> key.index >> value;
            bpTree.insert(key, value);
        } else if (cmd[0] == 'f') {
            std::cin >> key.index;
            trainsys::seqList<int> ans = bpTree.find(key);
            if (!ans.empty()) {
                for (int i = 0; i < ans.length(); i++) {
                    std::cout << ans.visit(i) << ' ';
                }
                std::cout << std::endl;
            } else puts("null");
        } else if (cmd[0] == 'd') {
            std::cin >> key.index >> value;
            bpTree.remove(key, value);
        }
    }
    return 0;
}