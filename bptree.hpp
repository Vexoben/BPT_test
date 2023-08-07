#ifndef BPTREE_HPP_BPTREE2_HPP
#define BPTREE_HPP_BPTREE2_HPP

#include <fstream>
#include "bufferList.hpp"
#include "vector.hpp"

template<class Key, class T, int M = 100, int L = 100>
class BPTree {
private:
    std::fstream file_tree, file_leaf;
    int rear_tree, rear_leaf, sum_data;
    const int len_of_head_leaf = 2 * sizeof(int);
    const int len_of_head_tree = 2 * sizeof(int);
    sjtu::vector<int> empty_tree;
    sjtu::vector<int> empty_leaf;

    struct Node {
        bool is_leaf;
        int pos, n;
        int A[M];
        std::pair<Key, T> K[M - 1];//K[now]>=A[now]中所有元素 0base
    };

    struct Leaf {
        int nxt, pos;
        int n;
        std::pair<Key, T> value[L];//0 base
    };

    std::string file_tree_name, file_leaf_name;
    bufferList<Node> node_buffer;
    bufferList<Leaf> leaf_buffer;
    Node root;
    Leaf leaf;
public:
    explicit BPTree(const std::string &name) {
        file_tree_name = name + "_file_tree", file_leaf_name = name + "_file_leaf";
        file_tree.open(file_tree_name);
        file_leaf.open(file_leaf_name);
        if (!file_leaf || !file_tree) { //第一次打开文件 要新建文件 初始化一些东西进去
            initialize();
        } else {
            file_tree.seekg(0), file_leaf.seekg(0);
            int root_tree;
            file_tree.read(reinterpret_cast<char *>(&root_tree), sizeof(int));
            file_tree.read(reinterpret_cast<char *>(&rear_tree), sizeof(int));
            file_tree.seekg(len_of_head_tree + root_tree * sizeof(Node));
            file_tree.read(reinterpret_cast<char *>(&root), sizeof(Node));
            int node_empty_size, leaf_empty_size;
            file_tree.seekg(len_of_head_tree + (rear_tree + 1) * sizeof(Node));
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
            Node tmp = node_buffer.pop_back();
            //     std::cout<<tmp.pos<<std::endl;
            file_tree.seekg(tmp.pos * sizeof(Node) + len_of_head_tree);
            file_tree.write(reinterpret_cast<char *>(&tmp), sizeof(Node));
        }
        while (!leaf_buffer.empty()) {
            Leaf tmp = leaf_buffer.pop_back();
            //   std::cout<<tmp.pos<<std::endl;
            file_leaf.seekg(tmp.pos * sizeof(Leaf) + len_of_head_leaf);
            file_leaf.write(reinterpret_cast<char *>(&tmp), sizeof(Leaf));
        }
        file_tree.seekg(len_of_head_tree + (rear_tree + 1) * sizeof(Node));
        int size_tree = empty_tree.size(), size_leaf = empty_leaf.size();
        //  std::cout<<size_tree<<" "<<size_leaf<<std::endl;
        file_tree.write(reinterpret_cast<char *>(&size_tree), sizeof(int));
        for (int i = 0; i < empty_tree.size(); i++)
            file_tree.write(reinterpret_cast<char *>(&empty_tree[i]), sizeof(int));
        file_leaf.seekg(len_of_head_leaf + (rear_leaf + 1) * sizeof(Leaf));
        file_leaf.write(reinterpret_cast<char *>(&size_leaf), sizeof(int));
        for (int i = 0; i < empty_leaf.size(); i++)
            file_leaf.write(reinterpret_cast<char *>(&empty_leaf[i]), sizeof(int));
        file_leaf.close();
        file_tree.close();
    }

    int size() { return sum_data; }

    void insert(const std::pair<Key, T> &val) {
        if (Insert(val, root)) {//分裂根节点
            Node new_root;//创建一个新的根节点
            Node new_node;//新的兄弟节点
            new_node.pos = get_rear_node(), new_node.is_leaf = root.is_leaf, new_node.n = M / 2;
            int mid = M / 2;//对半分
            for (int i = 0; i < mid; i++)new_node.A[i] = root.A[mid + i];
            for (int i = 0; i < mid - 1; i++)new_node.K[i] = root.K[mid + i];
            root.n = mid;
            write_node(root);
            write_node(new_node);
            new_root.n = 2, new_root.pos = get_rear_node(), new_root.is_leaf = false;
            new_root.A[0] = root.pos, new_root.A[1] = new_node.pos, new_root.K[0] = root.K[mid - 1];
            root = new_root;
            write_node(root);
        }
    }

    sjtu::vector<T> Find(const Key &key) {
        sjtu::vector<T> ans;
        Node p = root;
        while (!p.is_leaf)read_node(p, p.A[binary_search_node(key, p)]);//A[now]中元素小于等于Key[now] 循环找到叶节点
        read_leaf(leaf, p.A[binary_search_node(key, p)]);//找到叶子节点
        int now = binary_search_leaf(key, leaf);
        while (now < leaf.n && leaf.value[now].first == key)ans.push_back(leaf.value[now++].second);
        while (leaf.nxt && now == leaf.n) {//读到文件尾 寻找下一块
            read_leaf(leaf, leaf.nxt);
            now = 0;
            while (now < leaf.n && leaf.value[now].first == key)ans.push_back(leaf.value[now++].second);
        }
        return ans;
    }

    std::pair<bool, T> find(const Key &key) {
        T val;
        Node p = root;
        while (!p.is_leaf)read_node(p, p.A[binary_search_node(key, p)]);//A[now]中元素小于等于Key[now] 循环找到叶节点
        read_leaf(leaf, p.A[binary_search_node(key, p)]);//找到叶子节点
        int now = binary_search_leaf(key, leaf);
        bool flag = true;
        if (now == leaf.n || leaf.value[now].first != key)flag = false;
        return std::make_pair(flag, leaf.value[now].second);
    }

    void remove(const std::pair<Key, T> &val) {
        if (Remove(val, root)) {
            if (!root.is_leaf && root.n == 1) {//若根只有一个儿子，且根不为叶子，将儿子作为新的根
                Node son;
                read_node(son, root.A[0]);
                node_buffer.remove(root.pos);
                empty_tree.push_back(root.pos);
                root = son;
            }
        }
    }

    void modify(const std::pair<Key, T> &val, T new_val) {
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
    sjtu::vector<std::pair<Key,T>> roll(const Key &key) {
        sjtu::vector<std::pair<Key,T>> ans;
        Node p = root;
        while (!p.is_leaf)read_node(p, p.A[binary_search_node(key, p)]);//A[now]中元素小于等于Key[now] 循环找到叶节点
        read_leaf(leaf, p.A[binary_search_node(key, p)]);//找到叶子节点
        int now = binary_search_leaf(key, leaf);
        while (now < leaf.n)ans.push_back(leaf.value[now++]);
        while (leaf.nxt) {//读到文件尾 寻找下一块
            read_leaf(leaf, leaf.nxt);
            for(int i=0;i<leaf.n;i++)ans.push_back(leaf.value[i]);
        }
        return ans;
    }

private:
    bool Remove(const std::pair<Key, T> &val, Node &fa) {
        if (fa.is_leaf) {
            //若已经找到叶子
            int pos_node = binary_search_node_val(val, fa);//找到叶节点的位置
            read_leaf(leaf, fa.A[pos_node]);//读入叶节点
            int pos_leaf = binary_search_leaf_val(val, leaf);//找到数据在叶节点中的位置
            if (pos_leaf == leaf.n || leaf.value[pos_leaf] != val) {
                return false;//删除失败 return false}
            }

            leaf.n--, sum_data--;
            for (int i = pos_leaf; i < leaf.n; i++)leaf.value[i] = leaf.value[i + 1];//移动删除数据
            if (leaf.n < L / 2) {//并块
                Leaf pre, nxt;
                if (pos_node - 1 >= 0) {//若有前面的兄弟
                    read_leaf(pre, fa.A[pos_node - 1]);
                    if (pre.n > L / 2) {//若前面的兄弟有足够多的儿子可以借
                        //          std::cout << "get_pre_leaf!" << std::endl;
                        leaf.n++, pre.n--;
                        for (int i = leaf.n - 1; i > 0; i--)leaf.value[i] = leaf.value[i - 1];
                        leaf.value[0] = pre.value[pre.n];
                        fa.K[pos_node - 1] = pre.value[pre.n - 1];
                        write_leaf(leaf);
                        write_leaf(pre);
                        write_node(fa);
                        return false;
                    }
                }
                if (pos_node + 1 < fa.n) {//若有后面的兄弟
                    read_leaf(nxt, fa.A[pos_node + 1]);
                    if (nxt.n > L / 2) {//若后面的兄弟有足够多的儿子借
                        //             std::cout << "get_nxt_leaf!" << std::endl;
                        leaf.n++, nxt.n--;
                        leaf.value[leaf.n - 1] = nxt.value[0];
                        fa.K[pos_node] = nxt.value[0];
                        for (int i = 0; i < nxt.n; i++)nxt.value[i] = nxt.value[i + 1];
                        write_leaf(leaf);
                        write_leaf(nxt);
                        write_node(fa);
                        return false;
                    }
                }
                //前后都没有兄弟可以借儿子
                if (pos_node - 1 >= 0) {
                    //前面有兄弟 和前面合并
                    //          std::cout << "merge_pre_leaf!" << std::endl;
                    for (int i = 0; i < leaf.n; i++)pre.value[pre.n + i] = leaf.value[i];
                    pre.n += leaf.n;
                    pre.nxt = leaf.nxt;
                    write_leaf(pre);
                    leaf_buffer.remove(leaf.pos);
                    empty_leaf.push_back(leaf.pos);
                    fa.n--;
                    //更新fa的关键字和数据
                    for (int i = pos_node; i < fa.n; i++)fa.A[i] = fa.A[i + 1];
                    for (int i = pos_node - 1; i < fa.n - 1; i++)fa.K[i] = fa.K[i + 1];
                    if (fa.n < M / 2)return true;//需要继续调整
                    write_node(fa);
                    return false;
                }
                if (pos_node + 1 < fa.n) {
                    //后面有兄弟 和后面合并
                    //    std::cout << "merge_nxt_leaf!" << std::endl;
                    for (int i = 0; i < nxt.n; i++)leaf.value[leaf.n + i] = nxt.value[i];
                    leaf.n += nxt.n;
                    leaf.nxt = nxt.nxt;
                    write_leaf(leaf);
                    leaf_buffer.remove(nxt.pos);
                    empty_leaf.push_back(nxt.pos);
                    fa.n--;
                    //更新fa的关键字和数据
                    for (int i = pos_node + 1; i < fa.n; i++)fa.A[i] = fa.A[i + 1];
                    for (int i = pos_node; i < fa.n - 1; i++)fa.K[i] = fa.K[i + 1];
                    if (fa.n < M / 2)return true;//需要继续调整
                    write_node(fa);
                    return false;
                }
                write_leaf(leaf);
            } else write_leaf(leaf);
            return false;
        }
        Node son;
        int now = binary_search_node_val(val, fa);
        read_node(son, fa.A[now]);
        if (Remove(val, son)) {
            Node pre, nxt;
            if (now - 1 >= 0) {
                read_node(pre, fa.A[now - 1]);
                if (pre.n > M / 2) {
                    //          std::cout << "get_pre_node!" << std::endl;
                    son.n++, pre.n--;
                    for (int i = son.n - 1; i > 0; i--)son.A[i] = son.A[i - 1];
                    for (int i = son.n - 2; i > 0; i--)son.K[i] = son.K[i - 1];
                    son.A[0] = pre.A[pre.n];
                    son.K[0] = fa.K[now - 1];
                    fa.K[now - 1] = pre.K[pre.n - 1];
                    write_node(son);
                    write_node(pre);
                    write_node(fa);
                    return false;
                }
            }
            if (now + 1 < fa.n) {
                read_node(nxt, fa.A[now + 1]);
                if (nxt.n > M / 2) {
                    //             std::cout << "get_next_node!" << std::endl;
                    son.n++, nxt.n--;
                    son.A[son.n - 1] = nxt.A[0];
                    son.K[son.n - 2] = fa.K[now];
                    fa.K[now] = nxt.K[0];
                    for (int i = 0; i < nxt.n; i++)nxt.A[i] = nxt.A[i + 1];
                    for (int i = 0; i < nxt.n - 1; i++)nxt.K[i] = nxt.K[i + 1];
                    write_node(son);
                    write_node(nxt);
                    write_node(fa);
                    return false;
                }
            }
            if (now - 1 >= 0) {
                //         std::cout << "merge_pre_node!" << std::endl;
                for (int i = 0; i < son.n; i++)pre.A[pre.n + i] = son.A[i];
                pre.K[pre.n - 1] = fa.K[now - 1];
                for (int i = 0; i < son.n - 1; i++)pre.K[pre.n + i] = son.K[i];
                pre.n += son.n;
                write_node(pre);
                node_buffer.remove(son.pos);
                empty_tree.push_back(son.pos);
                fa.n--;
                for (int i = now; i < fa.n; i++)fa.A[i] = fa.A[i + 1];
                for (int i = now - 1; i < fa.n - 1; i++)fa.K[i] = fa.K[i + 1];
                if (fa.n < M / 2)return true;
                write_node(fa);
                return false;
            }
            if (now + 1 < fa.n) {
                //             std::cout << "merge_next_node!" << std::endl;
                for (int i = 0; i < nxt.n; i++)son.A[son.n + i] = nxt.A[i];
                son.K[son.n - 1] = fa.K[now];
                for (int i = 0; i < nxt.n - 1; i++)son.K[son.n + i] = nxt.K[i];
                son.n += nxt.n;
                write_node(son);
                node_buffer.remove(nxt.pos);
                empty_tree.push_back(nxt.pos);
                fa.n--;
                for (int i = now + 1; i < fa.n; i++)fa.A[i] = fa.A[i + 1];
                for (int i = now; i < fa.n - 1; i++)fa.K[i] = fa.K[i + 1];
                if (fa.n < M / 2)return true;
                write_node(fa);
                return false;
            }
        }
        return false;
    }

    bool Insert(const std::pair<Key, T> &val, Node &fa) {
        if (fa.is_leaf) {
            int pos_node = binary_search_node_val(val, fa);
            read_leaf(leaf, fa.A[pos_node]);
            int pos_leaf = binary_search_leaf_val(val, leaf);
            leaf.n++, sum_data++;
            for (int i = leaf.n - 1; i > pos_leaf; i--)leaf.value[i] = leaf.value[i - 1];
            leaf.value[pos_leaf] = val;
            if (leaf.n == L) {//裂块
                //    std::cout << "split L" << std::endl;
                Leaf new_leaf;
                new_leaf.pos = get_rear_leaf(), new_leaf.nxt = leaf.nxt, leaf.nxt = new_leaf.pos;
                int mid = L / 2;
                for (int i = 0; i < mid; i++)new_leaf.value[i] = leaf.value[i + mid];
                leaf.n = new_leaf.n = mid;
                write_leaf(leaf);
                write_leaf(new_leaf);
                for (int i = fa.n; i > pos_node + 1; i--)fa.A[i] = fa.A[i - 1];
                fa.A[pos_node + 1] = new_leaf.pos;
                for (int i = fa.n - 1; i > pos_node; i--)fa.K[i] = fa.K[i - 1];
                fa.K[pos_node] = leaf.value[mid - 1];
                fa.n++;
                if (fa.n == M) {//需要继续分裂
                    //      std::cout << "Split!" << std::endl;
                    return true;
                } else write_node(fa);
                return false;
            }
            write_leaf(leaf);
            return false;
        }
        Node son;
        int now = binary_search_node_val(val, fa);
        read_node(son, fa.A[now]);
        if (Insert(val, son)) {
            Node new_node;
            new_node.pos = get_rear_node(), new_node.is_leaf = son.is_leaf;
            int mid = M / 2;
            for (int i = 0; i < mid; i++)new_node.A[i] = son.A[mid + i];
            for (int i = 0; i < mid - 1; i++)new_node.K[i] = son.K[mid + i];
            new_node.n = son.n = mid;
            write_node(son);
            write_node(new_node);
            for (int i = fa.n; i > now + 1; i--)fa.A[i] = fa.A[i - 1];
            fa.A[now + 1] = new_node.pos;
            for (int i = fa.n - 1; i > now; i--)fa.K[i] = fa.K[i - 1];
            fa.K[now] = son.K[mid - 1];
            fa.n++;
            if (fa.n == M)return true;//需要继续分裂
            else write_node(fa);
            return false;
        } else return false;

    }

    void write_node(const Node &node) {
        std::pair<bool, Node> buffer = node_buffer.insert(node);
        if (buffer.first) {
            file_tree.seekg(buffer.second.pos * sizeof(Node) + len_of_head_tree);
            file_tree.write(reinterpret_cast<char *>(&buffer.second), sizeof(Node));
        }
    }

    void write_leaf(const Leaf &lef) {
        std::pair<bool, Leaf> buffer = leaf_buffer.insert(lef);
        if (buffer.first) {
            file_leaf.seekg(buffer.second.pos * sizeof(Leaf) + len_of_head_leaf);
            file_leaf.write(reinterpret_cast<char *>(&buffer.second), sizeof(Leaf));
        }
    }

    void read_node(Node &node, int pos) {
        std::pair<bool, Node> buffer = node_buffer.find(pos);
        if (buffer.first)node = buffer.second;
        else {
            file_tree.seekg(pos * sizeof(Node) + len_of_head_tree);
            file_tree.read(reinterpret_cast<char *>(&node), sizeof(Node));
        }
        write_node(node);
    }

    void read_leaf(Leaf &lef, int pos) {
        std::pair<bool, Leaf> buffer = leaf_buffer.find(pos);
        if (buffer.first)lef = buffer.second;
        else {
            file_leaf.seekg(pos * sizeof(Leaf) + len_of_head_leaf);
            file_leaf.read(reinterpret_cast<char *>(&lef), sizeof(Leaf));
        }
        write_leaf(lef);
    }

    int binary_search_leaf_val(const std::pair<Key, T> &val, const Leaf &lef) {
        int l = -1, r = lef.n - 1;
        while (l < r) {
            int mid = (l + r + 1) / 2;
            if (lef.value[mid] >= val)r = mid - 1;
            else l = mid;
        }
        return l + 1;
    }

    int binary_search_node_val(const std::pair<Key, T> &val, const Node &node) {
        int l = -1, r = node.n - 2;
        while (l < r) {
            int mid = (l + r + 1) / 2;
            if (node.K[mid] >= val)r = mid - 1;
            else l = mid;
        }
        return l + 1;
    }

    int binary_search_leaf(const Key &key, const Leaf &lef) {
        int l = -1, r = lef.n - 1;
        while (l < r) {
            int mid = (l + r + 1) / 2;
            if (lef.value[mid].first >= key)r = mid - 1;
            else l = mid;
        }
        return l + 1;
    }

    int binary_search_node(const Key &key, const Node &node) {
        int l = -1, r = node.n - 2;
        while (l < r) {
            int mid = (l + r + 1) / 2;
            if (node.K[mid].first >= key)r = mid - 1;
            else l = mid;
        }
        return l + 1;
    }

    void initialize() {
        file_tree.open(file_tree_name, std::ios::out);
        file_leaf.open(file_leaf_name, std::ios::out);
        root.is_leaf = root.pos = root.A[0] = 1, sum_data = 0;
        root.n = 1;
        rear_leaf = rear_tree = 1;//1 base
        Leaf ini_leaf;
        ini_leaf.nxt = ini_leaf.n = 0;
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
