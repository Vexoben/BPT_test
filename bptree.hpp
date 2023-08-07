#ifndef BPTREE_HPP_BPTREE2_HPP
#define BPTREE_HPP_BPTREE2_HPP

#include <fstream>
#include "bufferList.hpp"
#include "vector.hpp"

template<class Key, class Value, int M = 100, int L = 100>
class BPTree {
private:
    std::fstream file_tree, file_leaf;
    int rear_tree, rear_leaf, sum_data;
    const int len_of_head_leaf = 2 * sizeof(int);
    const int len_of_head_tree = 2 * sizeof(int);
    std::vector<int> empty_tree;
    std::vector<int> empty_leaf;

    struct TreeNode {
        bool isBottomNode;
        int pos, dataCount;
        int childrenPos[M];
        std::pair<Key, Value> septal[M - 1];  // septal[now]>=childrenPos[now]中所有元素; 0base
    };

    struct Leaf {
        int nxt, pos;
        int dataCount;
        std::pair<Key, Value> value[L];//0 base
    };

    std::string file_tree_name, file_leaf_name;
    bufferList<TreeNode> node_buffer;
    bufferList<Leaf> leaf_buffer;
    TreeNode root;
    Leaf leaf;
public:
    explicit BPTree(const std::string &name) {
        file_tree_name = name + "_file_tree", file_leaf_name = name + "_file_leaf";
        file_tree.open(file_tree_name);
        file_leaf.open(file_leaf_name);
        if (!file_leaf || !file_tree) { // 第一次打开文件 要新建文件 初始化一些东西进去
            initialize();
        } else {
            file_tree.seekg(0), file_leaf.seekg(0);
            int root_tree;
            file_tree.read(reinterpret_cast<char *>(&root_tree), sizeof(int));
            file_tree.read(reinterpret_cast<char *>(&rear_tree), sizeof(int));
            file_tree.seekg(len_of_head_tree + root_tree * sizeof(TreeNode));
            file_tree.read(reinterpret_cast<char *>(&root), sizeof(TreeNode));
            int node_empty_size, leaf_empty_size;
            file_tree.seekg(len_of_head_tree + (rear_tree + 1) * sizeof(TreeNode));
            file_tree.read(reinterpret_cast<char *>(&node_empty_size), sizeof(int));
            for (int i = 0; i < node_empty_size; i++) {
                int data;
                file_tree.read(reinterpret_cast<char *>(&data), sizeof(int));
                empty_tree.push_back(data);
            }
            file_leaf.read(reinterpret_cast<char *>(&rear_leaf), sizeof(int));
            file_leaf.read(reinterpret_cast<char *>(&sum_data), sizeof(int));
            file_leaf.seekg(len_of_head_leaf + (rear_leaf + 1) * sizeof(Leaf));
            file_leaf.read(reinterpret_cast<char *>(&leaf_empty_size), sizeof(int));
            for (int i = 0; i < leaf_empty_size; i++) {
                int data;
                file_leaf.read(reinterpret_cast<char *>(&data), sizeof(int));
                empty_leaf.push_back(data);
            }
        }
    }

    ~BPTree() {
        file_tree.seekg(0), file_leaf.seekg(0);
        file_tree.write(reinterpret_cast<char *>(&root.pos), sizeof(int));
        file_tree.write(reinterpret_cast<char *>(&rear_tree), sizeof(int));
        write_node(root);
        file_leaf.write(reinterpret_cast<char *>(&rear_leaf), sizeof(int));
        file_leaf.write(reinterpret_cast<char *>(&sum_data), sizeof(int));
        while (!node_buffer.empty()) {
            TreeNode tmp = node_buffer.pop_back();
            file_tree.seekg(tmp.pos * sizeof(TreeNode) + len_of_head_tree);
            file_tree.write(reinterpret_cast<char *>(&tmp), sizeof(TreeNode));
        }
        while (!leaf_buffer.empty()) {
            Leaf tmp = leaf_buffer.pop_back();
            file_leaf.seekg(tmp.pos * sizeof(Leaf) + len_of_head_leaf);
            file_leaf.write(reinterpret_cast<char *>(&tmp), sizeof(Leaf));
        }
        file_tree.seekg(len_of_head_tree + (rear_tree + 1) * sizeof(TreeNode));
        int size_tree = empty_tree.size(), size_leaf = empty_leaf.size();
        file_tree.write(reinterpret_cast<char *>(&size_tree), sizeof(int));
        for (int i = 0; i < empty_tree.size(); i++) {
            file_tree.write(reinterpret_cast<char *>(&empty_tree[i]), sizeof(int));
        }
        file_leaf.seekg(len_of_head_leaf + (rear_leaf + 1) * sizeof(Leaf));
        file_leaf.write(reinterpret_cast<char *>(&size_leaf), sizeof(int));
        for (int i = 0; i < empty_leaf.size(); i++) {
            file_leaf.write(reinterpret_cast<char *>(&empty_leaf[i]), sizeof(int));
        }
        file_leaf.close();
        file_tree.close();
    }

    int size() { return sum_data; }

    void insert(const std::pair<Key, Value> &val) {
        if (insertDfs(val, root)) {  // 分裂根节点
            TreeNode new_root;  // 创建一个新的根节点
            TreeNode new_node;  // 新的兄弟节点
            new_node.pos = get_rear_node(), new_node.isBottomNode = root.isBottomNode, new_node.dataCount = M / 2;
            int mid = M / 2;  // 对半分
            for (int i = 0; i < mid; i++) {
                new_node.childrenPos[i] = root.childrenPos[mid + i];
            }
            for (int i = 0; i < mid - 1; i++) {
                new_node.septal[i] = root.septal[mid + i];
            }
            root.dataCount = mid;
            write_node(root);
            write_node(new_node);
            new_root.dataCount = 2;
            new_root.pos = get_rear_node();
            new_root.isBottomNode = false;
            new_root.childrenPos[0] = root.pos;
            new_root.childrenPos[1] = new_node.pos;
            new_root.septal[0] = root.septal[mid - 1];
            root = new_root;
            write_node(root);
        }
    }

    std::vector<Value> Find(const Key &key) {
        std::vector<Value> ans;
        TreeNode p = root;
        while (!p.isBottomNode) {  // childrenPos[now]中元素小于等于Key[now] 循环找到叶节点
            readTreeNode(p, p.childrenPos[binarySearchTreeNode(key, p)]); 
        }
        readLeaf(leaf, p.childrenPos[binarySearchTreeNode(key, p)]);  // 找到叶子节点
        int now = binarySearchLeaf(key, leaf);
        while (now < leaf.dataCount && leaf.value[now].first == key) {
            ans.push_back(leaf.value[now++].second);
        }
        while (leaf.nxt && now == leaf.dataCount) {  // 读到文件尾 寻找下一块
            readLeaf(leaf, leaf.nxt);
            now = 0;
            while (now < leaf.dataCount && leaf.value[now].first == key) {
                ans.push_back(leaf.value[now++].second);
            }
        }
        return ans;
    }

    std::pair<bool, Value> find(const Key &key) {
        Value val;
        TreeNode p = root;
        while (!p.isBottomNode) {  // childrenPos[now]中元素小于等于Key[now] 循环找到叶节点
            readTreeNode(p, p.childrenPos[binarySearchTreeNode(key, p)]);
        }
        readLeaf(leaf, p.childrenPos[binarySearchTreeNode(key, p)]);//找到叶子节点
        int now = binarySearchLeaf(key, leaf);
        bool flag = true;
        if (now == leaf.dataCount || leaf.value[now].first != key) {
            flag = false;
        }
        return std::make_pair(flag, leaf.value[now].second);
    }

    void remove(const std::pair<Key, Value> &val) {
        if (removeDfs(val, root)) {
            if (!root.isBottomNode && root.dataCount == 1) {  // 若根只有一个儿子，且根不为叶子，将儿子作为新的根
                TreeNode son;
                readTreeNode(son, root.childrenPos[0]);
                node_buffer.remove(root.pos);
                empty_tree.push_back(root.pos);
                root = son;
            }
        }
    }

    void modify(const std::pair<Key, Value> &val, Value new_val) {
        remove(val);
        insert(std::make_pair(val.first, new_val));
    }

    void clear() {
        file_tree.close();
        file_leaf.close();
        node_buffer.clear();
        leaf_buffer.clear();
        empty_tree.clear();
        empty_leaf.clear();
        initialize();
    }

    //用于回顾 返回key之后的所有数据
    std::vector<std::pair<Key,Value>> roll(const Key &key) {
        std::vector<std::pair<Key,Value>> ans;
        TreeNode p = root;
        while (!p.isBottomNode) {  // childrenPos[now]中元素小于等于Key[now] 循环找到叶节点
            readTreeNode(p, p.childrenPos[binarySearchTreeNode(key, p)]);
        }
        readLeaf(leaf, p.childrenPos[binarySearchTreeNode(key, p)]);//找到叶子节点
        int now = binarySearchLeaf(key, leaf);
        while (now < leaf.dataCount) {
            ans.push_back(leaf.value[now++]);
        }
        while (leaf.nxt) {//读到文件尾 寻找下一块
            readLeaf(leaf, leaf.nxt);
            for (int i=0;i<leaf.dataCount;i++) {
                ans.push_back(leaf.value[i]);
            }
        }
        return ans;
    }

private:
    bool removeDfs(const std::pair<Key, Value> &val, TreeNode &fa) {
        if (fa.isBottomNode) {  // 若已经找到叶子
            int pos_node = binarySearchTreeNodeValue(val, fa);  // 找到叶节点的位置
            readLeaf(leaf, fa.childrenPos[pos_node]);  // 读入叶节点
            int pos_leaf = binary_search_leaf_val(val, leaf);  // 找到数据在叶节点中的位置
            if (pos_leaf == leaf.dataCount || leaf.value[pos_leaf] != val) {
                return false;  // 删除失败 return false
            }
            leaf.dataCount--, sum_data--;
            for (int i = pos_leaf; i < leaf.dataCount; i++) {
                leaf.value[i] = leaf.value[i + 1];  // 移动删除数据
            }
            if (leaf.dataCount < L / 2) {  // 并块
                Leaf pre, nxt;
                if (pos_node - 1 >= 0) {  // 若有前面的兄弟
                    readLeaf(pre, fa.childrenPos[pos_node - 1]);
                    if (pre.dataCount > L / 2) {  // 若前面的兄弟有足够多的儿子可以借
                        leaf.dataCount++, pre.dataCount--;
                        for (int i = leaf.dataCount - 1; i > 0; i--) {
                            leaf.value[i] = leaf.value[i - 1];
                        }
                        leaf.value[0] = pre.value[pre.dataCount];
                        fa.septal[pos_node - 1] = pre.value[pre.dataCount - 1];
                        write_leaf(leaf);
                        write_leaf(pre);
                        write_node(fa);
                        return false;
                    }
                }
                if (pos_node + 1 < fa.dataCount) {  // 若有后面的兄弟
                    readLeaf(nxt, fa.childrenPos[pos_node + 1]);
                    if (nxt.dataCount > L / 2) {  // 若后面的兄弟有足够多的儿子借
                        leaf.dataCount++, nxt.dataCount--;
                        leaf.value[leaf.dataCount - 1] = nxt.value[0];
                        fa.septal[pos_node] = nxt.value[0];
                        for (int i = 0; i < nxt.dataCount; i++) {
                            nxt.value[i] = nxt.value[i + 1];
                        }
                        write_leaf(leaf);
                        write_leaf(nxt);
                        write_node(fa);
                        return false;
                    }
                }
                //前后都没有兄弟可以借儿子
                if (pos_node - 1 >= 0) {  // 前面有兄弟 和前面合并
                    for (int i = 0; i < leaf.dataCount; i++) {
                        pre.value[pre.dataCount + i] = leaf.value[i];
                    }
                    pre.dataCount += leaf.dataCount;
                    pre.nxt = leaf.nxt;
                    write_leaf(pre);
                    leaf_buffer.remove(leaf.pos);
                    empty_leaf.push_back(leaf.pos);
                    //更新fa的关键字和数据
                    fa.dataCount--;
                    for (int i = pos_node; i < fa.dataCount; i++) {
                        fa.childrenPos[i] = fa.childrenPos[i + 1];
                    }
                    for (int i = pos_node - 1; i < fa.dataCount - 1; i++) {
                        fa.septal[i] = fa.septal[i + 1];
                    }
                    if (fa.dataCount < M / 2) {  // 需要继续调整
                        return true;
                    }
                    write_node(fa);
                    return false;
                }
                if (pos_node + 1 < fa.dataCount) {  // 后面有兄弟 和后面合并
                    for (int i = 0; i < nxt.dataCount; i++) {
                        leaf.value[leaf.dataCount + i] = nxt.value[i];
                    }
                    leaf.dataCount += nxt.dataCount;
                    leaf.nxt = nxt.nxt;
                    write_leaf(leaf);
                    leaf_buffer.remove(nxt.pos);
                    empty_leaf.push_back(nxt.pos);
                    fa.dataCount--;
                    //更新fa的关键字和数据
                    for (int i = pos_node + 1; i < fa.dataCount; i++) {
                        fa.childrenPos[i] = fa.childrenPos[i + 1];
                    }
                    for (int i = pos_node; i < fa.dataCount - 1; i++) {
                        fa.septal[i] = fa.septal[i + 1];
                    }
                    if (fa.dataCount < M / 2) {  // 需要继续调整
                        return true;
                    }
                    write_node(fa);
                    return false;
                }
                write_leaf(leaf);
            } else write_leaf(leaf);
            return false;
        }
        TreeNode son;
        int now = binarySearchTreeNodeValue(val, fa);
        readTreeNode(son, fa.childrenPos[now]);
        if (removeDfs(val, son)) {
            TreeNode pre, nxt;
            if (now - 1 >= 0) {
                readTreeNode(pre, fa.childrenPos[now - 1]);
                if (pre.dataCount > M / 2) {
                    son.dataCount++, pre.dataCount--;
                    for (int i = son.dataCount - 1; i > 0; i--) {
                        son.childrenPos[i] = son.childrenPos[i - 1];
                    }
                    for (int i = son.dataCount - 2; i > 0; i--) {
                        son.septal[i] = son.septal[i - 1];
                    }
                    son.childrenPos[0] = pre.childrenPos[pre.dataCount];
                    son.septal[0] = fa.septal[now - 1];
                    fa.septal[now - 1] = pre.septal[pre.dataCount - 1];
                    write_node(son);
                    write_node(pre);
                    write_node(fa);
                    return false;
                }
            }
            if (now + 1 < fa.dataCount) {
                readTreeNode(nxt, fa.childrenPos[now + 1]);
                if (nxt.dataCount > M / 2) {
                    son.dataCount++, nxt.dataCount--;
                    son.childrenPos[son.dataCount - 1] = nxt.childrenPos[0];
                    son.septal[son.dataCount - 2] = fa.septal[now];
                    fa.septal[now] = nxt.septal[0];
                    for (int i = 0; i < nxt.dataCount; i++) {
                        nxt.childrenPos[i] = nxt.childrenPos[i + 1];
                    }
                    for (int i = 0; i < nxt.dataCount - 1; i++) {
                        nxt.septal[i] = nxt.septal[i + 1];
                    }
                    write_node(son);
                    write_node(nxt);
                    write_node(fa);
                    return false;
                }
            }
            if (now - 1 >= 0) {
                for (int i = 0; i < son.dataCount; i++) {
                    pre.childrenPos[pre.dataCount + i] = son.childrenPos[i];
                }
                pre.septal[pre.dataCount - 1] = fa.septal[now - 1];
                for (int i = 0; i < son.dataCount - 1; i++) {
                    pre.septal[pre.dataCount + i] = son.septal[i];
                }
                pre.dataCount += son.dataCount;
                write_node(pre);
                node_buffer.remove(son.pos);
                empty_tree.push_back(son.pos);
                fa.dataCount--;
                for (int i = now; i < fa.dataCount; i++) {
                    fa.childrenPos[i] = fa.childrenPos[i + 1];
                }
                for (int i = now - 1; i < fa.dataCount - 1; i++) {
                    fa.septal[i] = fa.septal[i + 1];
                }
                if (fa.dataCount < M / 2)return true;
                write_node(fa);
                return false;
            }
            if (now + 1 < fa.dataCount) {
                for (int i = 0; i < nxt.dataCount; i++)son.childrenPos[son.dataCount + i] = nxt.childrenPos[i];
                son.septal[son.dataCount - 1] = fa.septal[now];
                for (int i = 0; i < nxt.dataCount - 1; i++)son.septal[son.dataCount + i] = nxt.septal[i];
                son.dataCount += nxt.dataCount;
                write_node(son);
                node_buffer.remove(nxt.pos);
                empty_tree.push_back(nxt.pos);
                fa.dataCount--;
                for (int i = now + 1; i < fa.dataCount; i++)fa.childrenPos[i] = fa.childrenPos[i + 1];
                for (int i = now; i < fa.dataCount - 1; i++)fa.septal[i] = fa.septal[i + 1];
                if (fa.dataCount < M / 2)return true;
                write_node(fa);
                return false;
            }
        }
        return false;
    }

    bool insertDfs(const std::pair<Key, Value> &val, TreeNode &fa) {
        if (fa.isBottomNode) {
            int pos_node = binarySearchTreeNodeValue(val, fa);
            readLeaf(leaf, fa.childrenPos[pos_node]);
            int pos_leaf = binary_search_leaf_val(val, leaf);
            leaf.dataCount++, sum_data++;
            for (int i = leaf.dataCount - 1; i > pos_leaf; i--)leaf.value[i] = leaf.value[i - 1];
            leaf.value[pos_leaf] = val;
            if (leaf.dataCount == L) {//裂块
                Leaf new_leaf;
                new_leaf.pos = get_rear_leaf(), new_leaf.nxt = leaf.nxt, leaf.nxt = new_leaf.pos;
                int mid = L / 2;
                for (int i = 0; i < mid; i++)new_leaf.value[i] = leaf.value[i + mid];
                leaf.dataCount = new_leaf.dataCount = mid;
                write_leaf(leaf);
                write_leaf(new_leaf);
                for (int i = fa.dataCount; i > pos_node + 1; i--)fa.childrenPos[i] = fa.childrenPos[i - 1];
                fa.childrenPos[pos_node + 1] = new_leaf.pos;
                for (int i = fa.dataCount - 1; i > pos_node; i--)fa.septal[i] = fa.septal[i - 1];
                fa.septal[pos_node] = leaf.value[mid - 1];
                fa.dataCount++;
                if (fa.dataCount == M) {//需要继续分裂
                    return true;
                } else write_node(fa);
                return false;
            }
            write_leaf(leaf);
            return false;
        }
        TreeNode son;
        int now = binarySearchTreeNodeValue(val, fa);
        readTreeNode(son, fa.childrenPos[now]);
        if (insertDfs(val, son)) {
            TreeNode new_node;
            new_node.pos = get_rear_node(), new_node.isBottomNode = son.isBottomNode;
            int mid = M / 2;
            for (int i = 0; i < mid; i++)new_node.childrenPos[i] = son.childrenPos[mid + i];
            for (int i = 0; i < mid - 1; i++)new_node.septal[i] = son.septal[mid + i];
            new_node.dataCount = son.dataCount = mid;
            write_node(son);
            write_node(new_node);
            for (int i = fa.dataCount; i > now + 1; i--)fa.childrenPos[i] = fa.childrenPos[i - 1];
            fa.childrenPos[now + 1] = new_node.pos;
            for (int i = fa.dataCount - 1; i > now; i--)fa.septal[i] = fa.septal[i - 1];
            fa.septal[now] = son.septal[mid - 1];
            fa.dataCount++;
            if (fa.dataCount == M)return true;//需要继续分裂
            else write_node(fa);
            return false;
        } else return false;

    }

    void write_node(const TreeNode &node) {
        std::pair<bool, TreeNode> buffer = node_buffer.insert(node);
        if (buffer.first) {
            file_tree.seekg(buffer.second.pos * sizeof(TreeNode) + len_of_head_tree);
            file_tree.write(reinterpret_cast<char *>(&buffer.second), sizeof(TreeNode));
        }
    }

    void write_leaf(const Leaf &lef) {
        std::pair<bool, Leaf> buffer = leaf_buffer.insert(lef);
        if (buffer.first) {
            file_leaf.seekg(buffer.second.pos * sizeof(Leaf) + len_of_head_leaf);
            file_leaf.write(reinterpret_cast<char *>(&buffer.second), sizeof(Leaf));
        }
    }

    void readTreeNode(TreeNode &node, int pos) {
        std::pair<bool, TreeNode> buffer = node_buffer.find(pos);
        if (buffer.first)node = buffer.second;
        else {
            file_tree.seekg(pos * sizeof(TreeNode) + len_of_head_tree);
            file_tree.read(reinterpret_cast<char *>(&node), sizeof(TreeNode));
        }
        write_node(node);
    }

    void readLeaf(Leaf &lef, int pos) {
        std::pair<bool, Leaf> buffer = leaf_buffer.find(pos);
        if (buffer.first)lef = buffer.second;
        else {
            file_leaf.seekg(pos * sizeof(Leaf) + len_of_head_leaf);
            file_leaf.read(reinterpret_cast<char *>(&lef), sizeof(Leaf));
        }
        write_leaf(lef);
    }

    int binary_search_leaf_val(const std::pair<Key, Value> &val, const Leaf &lef) {
        int l = -1, r = lef.dataCount - 1;
        while (l < r) {
            int mid = (l + r + 1) / 2;
            if (lef.value[mid] >= val)r = mid - 1;
            else l = mid;
        }
        return l + 1;
    }

    int binarySearchTreeNodeValue(const std::pair<Key, Value> &val, const TreeNode &node) {
        int l = -1, r = node.dataCount - 2;
        while (l < r) {
            int mid = (l + r + 1) / 2;
            if (node.septal[mid] >= val)r = mid - 1;
            else l = mid;
        }
        return l + 1;
    }

    // 在叶子节点中二分查找
    int binarySearchLeaf(const Key &key, const Leaf &lef) {
        int l = -1, r = lef.dataCount - 1;
        while (l < r) {
            int mid = (l + r + 1) / 2;
            if (lef.value[mid].first >= key)r = mid - 1;
            else l = mid;
        }
        return l + 1;
    }

    int binarySearchTreeNode(const Key &key, const TreeNode &node) {
        int l = -1, r = node.dataCount - 2;
        while (l < r) {
            int mid = (l + r + 1) / 2;
            if (node.septal[mid].first >= key)r = mid - 1;
            else l = mid;
        }
        return l + 1;
    }

    void initialize() {
        file_tree.open(file_tree_name, std::ios::out);
        file_leaf.open(file_leaf_name, std::ios::out);
        root.isBottomNode = root.pos = root.childrenPos[0] = 1, sum_data = 0;
        root.dataCount = 1;
        rear_leaf = rear_tree = 1;//1 base
        Leaf ini_leaf;
        ini_leaf.nxt = ini_leaf.dataCount = 0;
        ini_leaf.pos = 1;
        write_leaf(ini_leaf);
        file_tree.close();
        file_leaf.close();
        file_tree.open(file_tree_name);
        file_leaf.open(file_leaf_name);
    }

    int get_rear_node() {
        if (empty_tree.empty())return ++rear_tree;
        else {
            int new_index = empty_tree.back();
            empty_tree.pop_back();
            return new_index;
        }
    }

    int get_rear_leaf() {
        if (empty_leaf.empty())return ++rear_leaf;
        else {
            int new_index = empty_leaf.back();
            empty_leaf.pop_back();
            return new_index;
        }

    }
};

#endif //BPTREE_HPP_BPTREE2_HPP
