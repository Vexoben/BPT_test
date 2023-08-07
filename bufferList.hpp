#ifndef BPTREE_HPP_BUFFERLIST_HPP
#define BPTREE_HPP_BUFFERLIST_HPP


template<class T, int L = 50>

class bufferList {
private:
    int size = 0;

    struct linkNode {
        T val;
        linkNode *nxt, *fro;

        linkNode(const T &tmp, linkNode *nxt_ = nullptr, linkNode *fro_ = nullptr) {
            val = tmp, nxt = nxt_, fro = fro_;
        }
    };

    linkNode *head, *rear;
public:
    bufferList() {
        size = 0;
        T tmp;
        head = new linkNode(tmp, nullptr, nullptr);
        rear = new linkNode(tmp, nullptr, head);
        head->nxt = rear;
    }

    ~bufferList() {
        delete head;
        delete rear;
    }

    //查找是否存在相关元素
    std::pair<bool, T> find(int pos) {
        linkNode *p = head->nxt;
        while (p != rear) {
            if (pos == p->val.pos)return std::make_pair(true, p->val);
            p = p->nxt;
        }
        T tmp;
        return std::make_pair(false, tmp);
    }

    void remove(int pos) {
        linkNode *p = head->nxt, *del;
        while (p != rear) {
            del = p, p = p->nxt;
            if (pos == del->val.pos) {
                size--;
                del->fro->nxt = del->nxt;
                del->nxt->fro = del->fro;
                delete del;
            }
        }
    }

    //插入元素 并判断有没有需要写入文件的节点弹出
    std::pair<bool, T> insert(const T &val) {
        remove(val.pos);
        size++;
        head->nxt = new linkNode(val, head->nxt, head);
        head->nxt->nxt->fro = head->nxt;
        if (size == L) {
            size--;
            linkNode *del = rear->fro;
            del->fro->nxt = rear;
            rear->fro = del->fro;
            std::pair<bool, T> tmp= std::make_pair(true,del->val);
            delete del;
            return tmp;
        }
        return std::make_pair(false, val);
    }


    //判断bufferlist是否为空
    bool empty() {
        return size == 0;
    }

    //弹出需要写入的节点 从尾弹出
    T pop_back() {
        linkNode *p = head->nxt;
        p->fro->nxt = p->nxt;
        p->nxt->fro = p->fro;
        linkNode tmp=*p;
        size--;
        delete p;
        return tmp.val;
    }

    void clear(){
        linkNode *p,*del;
        p=head->nxt;
        while(p!=rear){
            del=p;
            p=p->nxt;
            delete del;
        }
        head->nxt=rear;
        rear->fro=head;
    }

};

#endif //BPTREE_HPP_BUFFERLIST_HPP
