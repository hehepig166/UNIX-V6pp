#ifndef HEHEPIG_INODETABLE_H
#define HEHEPIG_INODETABLE_H

#include "Inode.h"



class InodeTable {

public:
    static const int NINODE = 100;

public:
    Inode       m_Inode[NINODE];


public:

    void Initialize();
    void Reset();


    // 根据 dev 和 inumber 找到对应在 m_Inode 中的项
    // 若没装配，则分配一个
    // 会对 Inode 上锁，count++
    Inode*  IGet(int dev, int inumber);

    // 该 Inode 引用计数减一 count--
    // 若减为零，则释放，并同步改动到磁盘
    void    IPut(Inode *pInode);

    // 将被修改过的内存Inode更新到外存Inode
    void    UpdateInodeTable();

    // 检查该Inode是否在m_Inode中有对应的项
    // 若有，则返回下标，否则返回 -1
    int     IsLoaded(int dev, int inumber);

    // 找一个空闲的Inode
    Inode*  GetFreeInode();

    // 根据指针返回下标
    int     GetIndex(Inode *pInode);

};




#endif