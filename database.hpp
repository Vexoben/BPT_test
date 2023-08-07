#ifndef TRAINTICKETSYSTEM_DATABASE_HPP
#define TRAINTICKETSYSTEM_DATABASE_HPP

#include <iostream>
#include <fstream>

//索引指向的总数据库
//文件*1 数据库主体
template<class T>
class Database {
private:
    T data;
    std::fstream file;
    int rear;//文件头尾所在位置
    const int head_of_file = sizeof(int);
    const int train_day = sizeof(T) * 92;
    std::string database_name;
public:
    Database() = default;

    Database(std::string name) {
        database_name = name += "_database";
        file.open(database_name);
        if (!file) {
            rear = 0;
            file.open(database_name,std::ios::out);
            file.close();
            file.open(database_name);
        } else {
            file.seekg(0);
            file.read(reinterpret_cast<char *>(&rear), sizeof(int));
        }
    }
    //初始化 打开文件

    ~Database() {
        file.seekg(0);
        file.write(reinterpret_cast<char *>(&rear), sizeof(int));
    }
    //析构 关闭文件

    T find(int idx) {
        file.seekg(head_of_file + idx * sizeof(T));
        file.read(reinterpret_cast<char *>(&data), sizeof(T));
        return data;
    }
    //根据bptree索引搜到的文件位置找到数据

    T find(int idx, int day) {
        file.seekg(head_of_file + train_day * idx + day * sizeof(T));
        file.read(reinterpret_cast<char *>(&data), sizeof(T));
        return data;
    }

    //修改某条信息
    void modify(const T &val,int idx,int day=100){
        if(day<=92){
            file.seekg(head_of_file + train_day * idx + day * sizeof(T));
            file.write(reinterpret_cast<char *>(&val), sizeof(T));
        }
        else{
            file.seekg(head_of_file+idx* sizeof(T));
            file.write(reinterpret_cast<char *>(&val), sizeof(T));
        }
    }
    //用于查找某车次车票某天的售卖情况 idx为某车次的车票所在位置

    int insert(const T &value, int time = 0) {
        rear++;
        file.seekg(head_of_file + rear * sizeof(T));
        file.write(reinterpret_cast<char *>(&value), sizeof(T));
        return rear;
    }

    void clear(){
        rear = 0;
        file.close();
        file.open(database_name,std::ios::out);
        file.close();
        file.open(database_name);
    }
    //插入新的数据组
};


#endif //TRAINTICKETSYSTEM_DATABASE_HPP
