#ifndef HEHEPIG_FILESYSTEM_H
#define HEHEPIG_FILESYSTEM_H


#include "Inode.h"
#include "Buf.h"
#include "BufferManager.h"


struct SuperBlock {

public:
    int     s_isize;        // 外存 Inode 区占用的盘块数
    int     s_fsize;        // 盘块总数

    int     s_nfree;        // 直接管理的空闲盘块数量
    int     s_free[100];    // 直接管理的空闲盘块索引表

    int     s_ninode;       // 直接管理的空闲外存 Inode 数量
    int     s_inode[100];   // 直接管理的空闲外存 Inode 索引表

    int     s_flock;        // 封锁空闲盘块索引表标志
    int     s_ilock;        // 封锁空闲 Inode 表标志

    int     s_fmod;         // 内存 SuperBlock 脏标志（这里没用）
    int     s_ronly;        // 本文件系统只读
    int     s_time;         // 最近一次更新时间
    int     padding[47];    // 填充，使 SuperBlock 大小 1024 Bytes，两个扇区
};





// 文件系统装配块
struct Mount {

public:
    short           m_dev;          // 文件系统设备号
    SuperBlock      *m_spb;         // 指向文件系统的 SuperBlock 
    int             index;          // 挂在内存 Inode 表的下标

public:
    Mount(): m_dev(-1), m_spb(NULL), index(-1) {}
    ~Mount() {}

};


// 管理文件存储设备中的各类存储资源，
// 磁盘块、外存 Inode 的分配、释放
class FileSystem {

public:
    static const int NMOUNT = 5;                            // 最大挂载数

    static const int SUPER_BLOCK_START_SECTOR = 200;        // SuperBlock 起始扇区
    static const int SUPER_BLOCK_SECTOR_NUMBER = 2;         // SuperBlock 占两个扇区（200，201）

    static const int ROOTINO = 1;                           // 文件系统根目录外存 Inode 编号

private:
    Mount           m_Mount[NMOUNT];
    SuperBlock      *m_spb;             // 与磁盘镜像映射同步

public:

    void Initialize();
    void Reset();   // 清空，包括 InodeTable
    void Shutdown();

    // 映射主超级块（ ROOTDEV 的）
    void LoadSuperBlock();
    SuperBlock* GetSuperBlock(int dev);

    // 根据 dev 获取已存在的 SuperBlock 指针
    SuperBlock* GetFS(int dev);

    // 设备dev上申请一个空闲外存 Inode（用于创建新文件）
    // 返回指向对应在 InodeTable 中的项的指针
    // 会对该项上锁，记得解锁
    Inode* IAlloc(int dev);

    // 释放设备dev编号为number的外存 Inode（用于删除文件）
    void IFree(int dev, int number);

    // 设备dev上分配空闲磁盘块
    Buf* Alloc(int dev);

    // 回收设备dev编号为blkno的磁盘块
    // 不可多次 free（没做检查）
    void Free(int dev, int blono);

    // 查找 Inode 所在的 Mount 装配块
    Mount* GetMount(Inode *pInode);

private:

    // 检查设备dev上编号blkno的磁盘块是否属于数据盘块区
    bool BadBlock(SuperBlock *spb, short dev, int blkno);

    

};



#endif