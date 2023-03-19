#ifndef HEHEPIG_INODE_H
#define HEHEPIG_INODE_H

#include "Buf.h"


// 内存文件树节点
class Inode {

public:
    enum InodeFlag {
        ILOCK   = 0x1,      // 上锁
        IUPD    = 0x2,      // 内存 inode 被修改过，要更新外存 inode
        IACC    = 0x4,      // 内存 inode 被访问过，要修改最近一次访问时间
        IMOUNT  = 0x8,      // 内存 inode 用于挂载子文件系统
        IWANT   = 0x10      // 有进程正在等待该内存 inode 被解锁（这里用不到）
    };

public:
    unsigned int i_flag;    // 状态标志位
    unsigned int i_mode;    // 文件工作方式信息，非零代表被占用（目录、块设备等）

    int     i_count;        // 引用计数
    int     i_nlink;        // 该文件在目录树中不同路径名的数量（没用到）

    int     i_dev;          // 外存 inode 所在存储设备的设备号
    int     i_number;       // 外存 inode 区中的编号

    short   i_uid;          // 文件所有者的用户标志
    short   i_gid;          // 文件所有者的组标志

    int     i_size;         // 文件大小/Byte
    int     i_addr[10];     // 基本索引表


public:

    static const unsigned int IALLOC    = 0x8000;   // 文件被使用
    static const unsigned int IFMT      = 0x6000;   // 文件类型掩码，0 表示常规数据文件
    static const unsigned int IFDIR     = 0x4000;   // 目录文件
    static const unsigned int ILARG     = 0x1000;   // 是否是大型文件
    static const unsigned int IREAD     = 0x0100;   // 读权限
    static const unsigned int IWRITE    = 0x0080;   // 写权限
    static const unsigned int IEXEC     = 0x0040;   // 执行权限
    static const unsigned int IRWXU     = (IREAD|IWRITE|IEXEC);
    static const unsigned int IRWXG     = ((IRWXU) >> 3);
    static const unsigned int IRWXO     = ((IRWXU) >> 6);

    static const int BLOCK_SIZE = 512;      // 文件逻辑块大小：512 B
    static const int ADDRESS_PER_INDEX_BLOCK = BLOCK_SIZE / sizeof(int);    // 每个间接索引块包含的物理盘块号个数

    static const int SMALL_FILE_BLOCK = 6;                              // 小型文件：直接索引表最多可寻址的逻辑块号
    static const int LARGE_FILE_BLOCK = 128 * 2 + 6;                    // 大型文件：经过一次间接索引表最多可寻址的逻辑块号
    static const int HUGE_FILE_BLOCK = 128 * 128 * 2 + 128 * 2 + 6;     // 巨型文件：经过两次简介索引表最多可寻址的逻辑块号



public:
    Inode();

    // 将文件指定内容写到指定内存位置
    void ReadI();

    // 将指定内存内容写到文件指定位置
    void WriteI();

    // 文件逻辑块号转成物理盘块号
    int Bmap(int lbn);

    // 更新外存 Inode 的最后访问时间、修改时间
    void IUpdate(int tme);

    // 释放 Inode 对应文件占用的磁盘块（但是文件还存在，只是变成空文件）
    void ITrunc();
    //void ITrunc_rec(int phyBlkno, int dep);


    bool IsLocked();

    // 锁 Inode（阻塞）
    void Lock();

    // 解锁 Inode
    void Unlock();

    // 清空当前 Inode 结构体内容
    // 特定用于 IAlloc 中清空新分配 DiskNode 的原有数据
    // 所以保留 i_dev, i_number, i_flag, i_count
    void Clean();

    // 将指定位置的外存 Inode 读取到当前结构体中
    void ICopy(Buf *bp, int inumber);

};


// 外存索引节点（64 Bytes，每个块可以有 512/64 = 8 个 DiskInode）
class DiskInode {

public:
    unsigned int    d_mode;         // 状态标志位
    int             d_nlink;

    short           d_uid;
    short           d_gid;

    int             d_size;         // 文件大小
    int             d_addr[10];     // 基本索引表

    int             d_atime;        // 最后访问时间
    int             d_mtime;        // 最后修改时间

public:
    DiskInode();
    

};



#endif