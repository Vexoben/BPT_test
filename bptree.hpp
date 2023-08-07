#ifndef BPTREE_HPP_BPTREE_HPP
#define BPTREE_HPP_BPTREE_HPP

#include <vector>
#include <fstream>
#include "bufferList.hpp"

template<class Key, class Value, int M = 100, int L = 100>
class BPTree {
private:
    std::fstream treeNodeFile, leafFile;  // 存放树节点的文件和叶子节点的文件
    int rearTreeNode, rearLeaf;           // 最后一个树节点的位置和最后一个叶子节点的位置
    int sizeData;                         // 数据的个数
    // 存放树节点的文件的头部长度（前面预留两个int的空间用来存储树节点数量和最后一个树节点位置）
    const int headerLengthOfTreeNodeFile = 2 * sizeof(int);
    // 存放叶子节点的文件的头部长度（前面预留两个int的空间用来存储叶子节点数量和最后一个叶子节点位置）
    const int headerLengthOfLeafFile = 2 * sizeof(int);   
    // 被删除的树节点和叶子节点的位置，在插入的时候优先使用这些位置 
    std::vector<int> emptyTreeNode;
    std::vector<int> emptyLeaf;

    // B+树的节点
    struct TreeNode {
        bool isBottomNode;  // 记录是否是叶子节点上面一层的节点
        int pos, dataCount; // pos是节点的位置，dataCount是节点中子节点的个数
        int childrenPos[M]; // 子节点的位置，0 base
        std::pair<Key, Value> septal[M - 1]; // 各个子树之间的分隔关键字，0 base
    };

    // B+树的叶子节点
    struct Leaf {
        int nxt, pos;  // nxt是下一个叶子节点的位置，pos是当前叶子节点的位置
        int dataCount; // dataCount是当前叶子节点中数据的个数
        std::pair<Key, Value> value[L];  // 存放数据，0 base
    };

    // 存放树节点的文件的名字和叶子节点的文件的名字
    std::string treeNodeFileName, leafFileName;
    // 用来存放树节点和叶子节点的缓冲区
    bufferList<TreeNode> treeNodeBuffer;
    bufferList<Leaf> leafBuffer;
    // 树的根节点
    TreeNode root;

public:
    // 构造函数；从文件中读取必要信息，在内存中记录树的根节点，元素个数等关键信息
    explicit BPTree(const std::string &name) {
        treeNodeFileName = name + "_treeNodeFile", leafFileName = name + "_leafFile";
        // 打开文件，一个存放树节点，一个存放叶子节点
        treeNodeFile.open(treeNodeFileName);
        leafFile.open(leafFileName);
        if (!leafFile || !treeNodeFile) { // 如果文件不存在，就创建文件，并初始化
            initialize();
        } else {
            // 读取树节点文件的头部，得到树的根节点的位置和最后一个树节点的位置
            treeNodeFile.seekg(0), leafFile.seekg(0);
            int rootPos;
            treeNodeFile.read(reinterpret_cast<char *>(&rootPos), sizeof(int));
            treeNodeFile.read(reinterpret_cast<char *>(&rearTreeNode), sizeof(int));
            // 找到并读取树的根节点
            treeNodeFile.seekg(headerLengthOfTreeNodeFile + rootPos * sizeof(TreeNode));
            treeNodeFile.read(reinterpret_cast<char *>(&root), sizeof(TreeNode));
            // 最后一个树节点后面存放了被删除的树节点
            int treeNodeEmptySize, leafEmptySize;
            treeNodeFile.seekg(headerLengthOfTreeNodeFile + (rearTreeNode + 1) * sizeof(TreeNode));
            // 读取被删除的树节点的数量和位置
            treeNodeFile.read(reinterpret_cast<char *>(&treeNodeEmptySize), sizeof(int));
            for (int i = 0; i < treeNodeEmptySize; i++) {
                int data;
                treeNodeFile.read(reinterpret_cast<char *>(&data), sizeof(int));
                emptyTreeNode.push_back(data);
            }
            // 读取叶子节点文件的头部，得到最后一个叶子节点的位置
            leafFile.read(reinterpret_cast<char *>(&rearLeaf), sizeof(int));
            // 插入的记录的数量存放在叶子节点文件的头部的第二个int中
            leafFile.read(reinterpret_cast<char *>(&sizeData), sizeof(int));
            // 最后一个叶子节点后面存放了被删除的叶子节点
            leafFile.seekg(headerLengthOfLeafFile + (rearLeaf + 1) * sizeof(Leaf));
            // 读取被删除的叶子节点的数量和位置
            leafFile.read(reinterpret_cast<char *>(&leafEmptySize), sizeof(int));
            for (int i = 0; i < leafEmptySize; i++) {
                int data;
                leafFile.read(reinterpret_cast<char *>(&data), sizeof(int));
                emptyLeaf.push_back(data);
            }
        }
    }

    // 析构函数；将树的根节点，被删除的节点的位置等信息写入文件
    ~BPTree() {
        // 将树的根节点、最后一个树节点的位置写入文件
        treeNodeFile.seekg(0), leafFile.seekg(0);
        treeNodeFile.write(reinterpret_cast<char *>(&root.pos), sizeof(int));
        treeNodeFile.write(reinterpret_cast<char *>(&rearTreeNode), sizeof(int));
        // 将树的根节点写入文件
        writeTreeNode(root);
        // 将最后一个叶子节点的位置写入文件
        leafFile.write(reinterpret_cast<char *>(&rearLeaf), sizeof(int));
        // 将插入的记录的数量写入文件
        leafFile.write(reinterpret_cast<char *>(&sizeData), sizeof(int));
        // 将缓存中的树节点和叶子节点写入文件
        while (!treeNodeBuffer.empty()) {
            TreeNode tmp = treeNodeBuffer.pop_back();
            treeNodeFile.seekg(tmp.pos * sizeof(TreeNode) + headerLengthOfTreeNodeFile);
            treeNodeFile.write(reinterpret_cast<char *>(&tmp), sizeof(TreeNode));
        }
        while (!leafBuffer.empty()) {
            Leaf tmp = leafBuffer.pop_back();
            leafFile.seekg(tmp.pos * sizeof(Leaf) + headerLengthOfLeafFile);
            leafFile.write(reinterpret_cast<char *>(&tmp), sizeof(Leaf));
        }
        // 将被删除的树节点和叶子节点的位置写入文件
        treeNodeFile.seekg(headerLengthOfTreeNodeFile + (rearTreeNode + 1) * sizeof(TreeNode));
        int emptyTreeNodeCount = emptyTreeNode.size(), emptyLeafCount = emptyLeaf.size();
        treeNodeFile.write(reinterpret_cast<char *>(&emptyTreeNodeCount), sizeof(int));
        for (int i = 0; i < emptyTreeNode.size(); i++) {
            treeNodeFile.write(reinterpret_cast<char *>(&emptyTreeNode[i]), sizeof(int));
        }
        leafFile.seekg(headerLengthOfLeafFile + (rearLeaf + 1) * sizeof(Leaf));
        leafFile.write(reinterpret_cast<char *>(&emptyLeafCount), sizeof(int));
        for (int i = 0; i < emptyLeaf.size(); i++) {
            leafFile.write(reinterpret_cast<char *>(&emptyLeaf[i]), sizeof(int));
        }
        // 关闭文件
        leafFile.close();
        treeNodeFile.close();
    }

    int size() { return sizeData; }

    // 插入记录
    void insert(const std::pair<Key, Value> &val) {
        if (insertDfs(val, root)) {  // 分裂根节点
            TreeNode newRoot;  // 创建一个新的根节点
            TreeNode newNode;  // 新的兄弟节点
            newNode.pos = getRearNode();
            newNode.isBottomNode = root.isBottomNode;
            newNode.dataCount = M / 2;
            int mid = M / 2;  // 对半分
            for (int i = 0; i < mid; i++) {
                newNode.childrenPos[i] = root.childrenPos[mid + i];
            }
            for (int i = 0; i < mid - 1; i++) {
                newNode.septal[i] = root.septal[mid + i];
            }
            root.dataCount = mid;
            writeTreeNode(root);
            writeTreeNode(newNode);
            newRoot.dataCount = 2;
            newRoot.pos = getRearNode();
            newRoot.isBottomNode = false;
            newRoot.childrenPos[0] = root.pos;
            newRoot.childrenPos[1] = newNode.pos;
            newRoot.septal[0] = root.septal[mid - 1];
            root = newRoot;
            writeTreeNode(root);
        }
    }

    // 查询记录；返回一个vector，因为可能一个key对应多个value
    std::vector<Value> find(const Key &key) {
        std::vector<Value> ans;
        TreeNode p = root;
        Leaf leaf;
        while (!p.isBottomNode) {  // childrenPos[now]中元素小于等于Key[now] 循环找到叶节点
            readTreeNode(p, p.childrenPos[binarySearchTreeNode(key, p)]); 
        }
        readLeaf(leaf, p.childrenPos[binarySearchTreeNode(key, p)]);  // 找到叶子节点
        int now = binarySearchLeaf(key, leaf);  // 在叶子节点中二分查找，找到第一个大于等于key的位置
        while (now < leaf.dataCount && leaf.value[now].first == key) {  // 读取所有关键字等于key的记录
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

    void remove(const std::pair<Key, Value> &val) {
        if (removeDfs(val, root)) {
            if (!root.isBottomNode && root.dataCount == 1) {  // 若根只有一个儿子，且根不为叶子，将儿子作为新的根
                TreeNode son;
                readTreeNode(son, root.childrenPos[0]);
                treeNodeBuffer.remove(root.pos);
                emptyTreeNode.push_back(root.pos);
                root = son;
            }
        }
    }

    void modify(const std::pair<Key, Value> &val, Value new_val) {
        remove(val);
        insert(std::make_pair(val.first, new_val));
    }

    void clear() {
        treeNodeFile.close();
        leafFile.close();
        treeNodeBuffer.clear();
        leafBuffer.clear();
        emptyTreeNode.clear();
        emptyLeaf.clear();
        initialize();
    }

    //用于回顾 返回key之后的所有数据
    std::vector<std::pair<Key,Value>> roll(const Key &key) {
        std::vector<std::pair<Key,Value>> ans;
        TreeNode p = root;
        Leaf leaf;
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
            Leaf leaf;
            int nodePos = binarySearchTreeNodeValue(val, fa);  // 找到叶节点的位置
            readLeaf(leaf, fa.childrenPos[nodePos]);  // 读入叶节点
            int leafPos = binarySearchLeafValue(val, leaf);  // 找到数据在叶节点中的位置
            if (leafPos == leaf.dataCount || leaf.value[leafPos] != val) {
                return false;  // 删除失败 return false
            }
            leaf.dataCount--, sizeData--;
            for (int i = leafPos; i < leaf.dataCount; i++) {
                leaf.value[i] = leaf.value[i + 1];  // 移动删除数据
            }
            if (leaf.dataCount < L / 2) {  // 并块
                Leaf pre, nxt;
                if (nodePos - 1 >= 0) {  // 若有前面的兄弟
                    readLeaf(pre, fa.childrenPos[nodePos - 1]);
                    if (pre.dataCount > L / 2) {  // 若前面的兄弟有足够多的儿子可以借
                        leaf.dataCount++, pre.dataCount--;
                        for (int i = leaf.dataCount - 1; i > 0; i--) {
                            leaf.value[i] = leaf.value[i - 1];
                        }
                        leaf.value[0] = pre.value[pre.dataCount];
                        fa.septal[nodePos - 1] = pre.value[pre.dataCount - 1];
                        writeLeaf(leaf);
                        writeLeaf(pre);
                        writeTreeNode(fa);
                        return false;
                    }
                }
                if (nodePos + 1 < fa.dataCount) {  // 若有后面的兄弟
                    readLeaf(nxt, fa.childrenPos[nodePos + 1]);
                    if (nxt.dataCount > L / 2) {  // 若后面的兄弟有足够多的儿子借
                        leaf.dataCount++, nxt.dataCount--;
                        leaf.value[leaf.dataCount - 1] = nxt.value[0];
                        fa.septal[nodePos] = nxt.value[0];
                        for (int i = 0; i < nxt.dataCount; i++) {
                            nxt.value[i] = nxt.value[i + 1];
                        }
                        writeLeaf(leaf);
                        writeLeaf(nxt);
                        writeTreeNode(fa);
                        return false;
                    }
                }
                //前后都没有兄弟可以借儿子
                if (nodePos - 1 >= 0) {  // 前面有兄弟 和前面合并
                    for (int i = 0; i < leaf.dataCount; i++) {
                        pre.value[pre.dataCount + i] = leaf.value[i];
                    }
                    pre.dataCount += leaf.dataCount;
                    pre.nxt = leaf.nxt;
                    writeLeaf(pre);
                    leafBuffer.remove(leaf.pos);
                    emptyLeaf.push_back(leaf.pos);
                    //更新fa的关键字和数据
                    fa.dataCount--;
                    for (int i = nodePos; i < fa.dataCount; i++) {
                        fa.childrenPos[i] = fa.childrenPos[i + 1];
                    }
                    for (int i = nodePos - 1; i < fa.dataCount - 1; i++) {
                        fa.septal[i] = fa.septal[i + 1];
                    }
                    if (fa.dataCount < M / 2) {  // 需要继续调整
                        return true;
                    }
                    writeTreeNode(fa);
                    return false;
                }
                if (nodePos + 1 < fa.dataCount) {  // 后面有兄弟 和后面合并
                    for (int i = 0; i < nxt.dataCount; i++) {
                        leaf.value[leaf.dataCount + i] = nxt.value[i];
                    }
                    leaf.dataCount += nxt.dataCount;
                    leaf.nxt = nxt.nxt;
                    writeLeaf(leaf);
                    leafBuffer.remove(nxt.pos);
                    emptyLeaf.push_back(nxt.pos);
                    fa.dataCount--;
                    //更新fa的关键字和数据
                    for (int i = nodePos + 1; i < fa.dataCount; i++) {
                        fa.childrenPos[i] = fa.childrenPos[i + 1];
                    }
                    for (int i = nodePos; i < fa.dataCount - 1; i++) {
                        fa.septal[i] = fa.septal[i + 1];
                    }
                    if (fa.dataCount < M / 2) {  // 需要继续调整
                        return true;
                    }
                    writeTreeNode(fa);
                    return false;
                }
                writeLeaf(leaf);
            } else writeLeaf(leaf);
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
                    writeTreeNode(son);
                    writeTreeNode(pre);
                    writeTreeNode(fa);
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
                    writeTreeNode(son);
                    writeTreeNode(nxt);
                    writeTreeNode(fa);
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
                writeTreeNode(pre);
                treeNodeBuffer.remove(son.pos);
                emptyTreeNode.push_back(son.pos);
                fa.dataCount--;
                for (int i = now; i < fa.dataCount; i++) {
                    fa.childrenPos[i] = fa.childrenPos[i + 1];
                }
                for (int i = now - 1; i < fa.dataCount - 1; i++) {
                    fa.septal[i] = fa.septal[i + 1];
                }
                if (fa.dataCount < M / 2)return true;
                writeTreeNode(fa);
                return false;
            }
            if (now + 1 < fa.dataCount) {
                for (int i = 0; i < nxt.dataCount; i++) {
                    son.childrenPos[son.dataCount + i] = nxt.childrenPos[i];
                }
                son.septal[son.dataCount - 1] = fa.septal[now];
                for (int i = 0; i < nxt.dataCount - 1; i++) {
                    son.septal[son.dataCount + i] = nxt.septal[i];
                }
                son.dataCount += nxt.dataCount;
                writeTreeNode(son);
                treeNodeBuffer.remove(nxt.pos);
                emptyTreeNode.push_back(nxt.pos);
                fa.dataCount--;
                for (int i = now + 1; i < fa.dataCount; i++) {
                    fa.childrenPos[i] = fa.childrenPos[i + 1];
                }
                for (int i = now; i < fa.dataCount - 1; i++) {
                    fa.septal[i] = fa.septal[i + 1];
                }
                if (fa.dataCount < M / 2)return true;
                writeTreeNode(fa);
                return false;
            }
        }
        return false;
    }

    bool insertDfs(const std::pair<Key, Value> &val, TreeNode &fa) {
        if (fa.isBottomNode) {
            Leaf leaf;
            int nodePos = binarySearchTreeNodeValue(val, fa);
            readLeaf(leaf, fa.childrenPos[nodePos]);
            int leafPos = binarySearchLeafValue(val, leaf);
            leaf.dataCount++, sizeData++;
            for (int i = leaf.dataCount - 1; i > leafPos; i--) {
                leaf.value[i] = leaf.value[i - 1];
            }
            leaf.value[leafPos] = val;
            if (leaf.dataCount == L) {//裂块
                Leaf new_leaf;
                new_leaf.pos = get_rearLeaf();
                new_leaf.nxt = leaf.nxt;
                leaf.nxt = new_leaf.pos;
                int mid = L / 2;
                for (int i = 0; i < mid; i++) {
                    new_leaf.value[i] = leaf.value[i + mid];
                }
                leaf.dataCount = new_leaf.dataCount = mid;
                writeLeaf(leaf);
                writeLeaf(new_leaf);
                for (int i = fa.dataCount; i > nodePos + 1; i--) {
                    fa.childrenPos[i] = fa.childrenPos[i - 1];
                }
                fa.childrenPos[nodePos + 1] = new_leaf.pos;
                for (int i = fa.dataCount - 1; i > nodePos; i--) {
                    fa.septal[i] = fa.septal[i - 1];
                }
                fa.septal[nodePos] = leaf.value[mid - 1];
                fa.dataCount++;
                if (fa.dataCount == M) {  // 需要继续分裂
                    return true;
                } else writeTreeNode(fa);
                return false;
            }
            writeLeaf(leaf);
            return false;
        }
        TreeNode son;
        int now = binarySearchTreeNodeValue(val, fa);
        readTreeNode(son, fa.childrenPos[now]);
        if (insertDfs(val, son)) {
            TreeNode newNode;
            newNode.pos = getRearNode(), newNode.isBottomNode = son.isBottomNode;
            int mid = M / 2;
            for (int i = 0; i < mid; i++) {
                newNode.childrenPos[i] = son.childrenPos[mid + i];
            }
            for (int i = 0; i < mid - 1; i++) {
                newNode.septal[i] = son.septal[mid + i];
            }
            newNode.dataCount = son.dataCount = mid;
            writeTreeNode(son);
            writeTreeNode(newNode);
            for (int i = fa.dataCount; i > now + 1; i--) {
                fa.childrenPos[i] = fa.childrenPos[i - 1];
            }
            fa.childrenPos[now + 1] = newNode.pos;
            for (int i = fa.dataCount - 1; i > now; i--) {
                fa.septal[i] = fa.septal[i - 1];
            }
            fa.septal[now] = son.septal[mid - 1];
            fa.dataCount++;
            if (fa.dataCount == M) {  // 需要继续分裂
                return true;
            } else writeTreeNode(fa);
            return false;
        } else return false;
    }

    void writeTreeNode(const TreeNode &node) {
        std::pair<bool, TreeNode> buffer = treeNodeBuffer.insert(node);
        if (buffer.first) {
            treeNodeFile.seekg(buffer.second.pos * sizeof(TreeNode) + headerLengthOfTreeNodeFile);
            treeNodeFile.write(reinterpret_cast<char *>(&buffer.second), sizeof(TreeNode));
        }
    }

    void writeLeaf(const Leaf &lef) {
        std::pair<bool, Leaf> buffer = leafBuffer.insert(lef);
        if (buffer.first) {
            leafFile.seekg(buffer.second.pos * sizeof(Leaf) + headerLengthOfLeafFile);
            leafFile.write(reinterpret_cast<char *>(&buffer.second), sizeof(Leaf));
        }
    }

    void readTreeNode(TreeNode &node, int pos) {
        std::pair<bool, TreeNode> buffer = treeNodeBuffer.find(pos);
        if (buffer.first) node = buffer.second;
        else {
            treeNodeFile.seekg(pos * sizeof(TreeNode) + headerLengthOfTreeNodeFile);
            treeNodeFile.read(reinterpret_cast<char *>(&node), sizeof(TreeNode));
        }
        writeTreeNode(node);
    }

    void readLeaf(Leaf &lef, int pos) {
        std::pair<bool, Leaf> buffer = leafBuffer.find(pos);
        if (buffer.first) lef = buffer.second;
        else {
            leafFile.seekg(pos * sizeof(Leaf) + headerLengthOfLeafFile);
            leafFile.read(reinterpret_cast<char *>(&lef), sizeof(Leaf));
        }
        writeLeaf(lef);
    }

    int binarySearchLeafValue(const std::pair<Key, Value> &val, const Leaf &lef) {
        int l = -1, r = lef.dataCount - 1;
        while (l < r) {
            int mid = (l + r + 1) / 2;
            if (lef.value[mid] >= val) r = mid - 1;
            else l = mid;
        }
        return l + 1;
    }

    int binarySearchTreeNodeValue(const std::pair<Key, Value> &val, const TreeNode &node) {
        int l = -1, r = node.dataCount - 2;
        while (l < r) {
            int mid = (l + r + 1) / 2;
            if (node.septal[mid] >= val) r = mid - 1;
            else l = mid;
        }
        return l + 1;
    }

    // 在叶子节点中二分查找
    int binarySearchLeaf(const Key &key, const Leaf &lef) {
        int l = -1, r = lef.dataCount - 1;
        while (l < r) {
            int mid = (l + r + 1) / 2;
            if (lef.value[mid].first >= key) r = mid - 1;
            else l = mid;
        }
        return l + 1;
    }

    int binarySearchTreeNode(const Key &key, const TreeNode &node) {
        int l = -1, r = node.dataCount - 2;
        while (l < r) {
            int mid = (l + r + 1) / 2;
            if (node.septal[mid].first >= key) r = mid - 1;
            else l = mid;
        }
        return l + 1;
    }

    void initialize() {
        treeNodeFile.open(treeNodeFileName, std::ios::out);
        leafFile.open(leafFileName, std::ios::out);
        root.isBottomNode = root.pos = root.childrenPos[0] = 1, sizeData = 0;
        root.dataCount = 1;
        rearLeaf = rearTreeNode = 1;//1 base
        Leaf ini_leaf;
        ini_leaf.nxt = ini_leaf.dataCount = 0;
        ini_leaf.pos = 1;
        writeLeaf(ini_leaf);
        treeNodeFile.close();
        leafFile.close();
        treeNodeFile.open(treeNodeFileName);
        leafFile.open(leafFileName);
    }

    int getRearNode() {
        if (emptyTreeNode.empty()) {
            return ++rearTreeNode;
        } else {
            int newIndex = emptyTreeNode.back();
            emptyTreeNode.pop_back();
            return newIndex;
        }
    }

    int get_rearLeaf() {
        if (emptyLeaf.empty()) {
            return ++rearLeaf;
        } else {
            int newIndex = emptyLeaf.back();
            emptyLeaf.pop_back();
            return newIndex;
        }
    }
};

#endif //BPTREE_HPP_BPTREE_HPP
