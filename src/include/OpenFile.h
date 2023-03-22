#ifndef HEHEPIG_OPENFILE_H
#define HEHEPIG_OPENFILE_H

#include "Inode.h"

// 记录了一个打开文件
class File {

public:
    enum FileFlags {
        FREAD   = 0x1,
        FWRITE  = 0x2
    };

public:
    int     f_flag;
    int     f_count;
    int     f_ino;      // 指向的内存Inode下标
    int     f_offset;

public:
    File();
    Inode* GetInode();
};


// 记录了一个进程打开的文件
// 保存在各自的 User 结构中
class OpenFiles {

public:
    static const int NOFILES = 15;              // 进程允许打开的最大文件数

public:
    File    *ProcessOpenFileTable[NOFILES];     // 本进程打开的文件，指向 Kernel 中共享的全局打开文件表中的项

public:
    OpenFiles();

    // 申请一个空表项
    int AllocFreeSlot();

    // 获取对应的 File
    File* GetF(int fd);

    void SetF(int fd, File *pFile);
};


// 全局共享的系统打开文件表
// 保存在共享的 Kernel 中
class OpenFileTable {

public:
    static const int NFILE = 100;

public:
    File m_File[NFILE];

public:

    void Reset();

    // 在本进程的 openfiles 找一个空项，再在全局本对象的 m_File 找一个空项，对应
    // 返回指向本对象的一个项，同时将本进程的项 id 存在 user.u_tmp 中
    File *FAlloc();

    // File 引用计数 f_count--
    // 若减为零，则释放 File
    void CloseF(File *pFile);

};



#endif