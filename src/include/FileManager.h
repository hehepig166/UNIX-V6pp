#ifndef HEHEPIG_FILEMANAGER_H
#define HEHEPIG_FILEMANAGER_H

#include "Inode.h"

#include <vector>
#include <string>





// 目录文件项
class DirectoryEntry {

public:
    static const int DIRSIZ     = 28;

public:
    int     m_ino;              // 目录项 Inode 编号，0 代表空项
    char    m_name[DIRSIZ];     // 目录项路径名

public:
    DirectoryEntry() {
        m_ino = 0;
        m_name[0] = '\0';
    }

    DirectoryEntry(int ino, const char *name) {
        int i;
        m_ino = ino;
        for (i=0; i<DIRSIZ-1 && name[i] ; i++) {
            m_name[i] = name[i];
        }
        m_name[i] = '\0';
    }

};






// 封装各种系统调用
// 各自进程的该对象独立（只是一个函数集合）
class FileManager {

public:
    enum DirectorySearchMode {
        OPEN    = 0,
        CREATE  = 1,
        DELETE  = 2
    };

public:
    void Initialize();
    void ShutDown();

public:


    int Open(const char *path, int mode);
    int Create(const char *path, int mode);
    int Close(int fd);

    int Read(int fd, void *src, int size);
    int Write(int fd, void *src, int size);

    int SetCurDir(const char *path);

public:

    // 公共函数
    // 接收的是上锁的 pInode
    // 若出错了，函数内会对 pInode 减一引用
    // 正常，则不会改变 pInode 的引用
    // 返回全局打开文件表的 fd
    int Open1(Inode *pInode, int mode, int trf);

    // 分析目录文件，返回有效的文件名
    // ret[i] = <dirEntry, offset>
    // ret[0] 是预留项，offset 为第一个空项（没有则为最后一项的末尾（新））
    // 函数中，不对上锁与否进行检查，不改变锁性，所以调用前最好上锁保证不被干扰
    // 若 Inode 无法打开/不是目录文件，则 ret.size() = 0
    // 若成功分析，则 ret.size() >= 1 （至少有一个预留项）
    std::vector<std::pair<DirectoryEntry, int>> GetDirList(Inode *pInode);

    // 根据路径获取对应 Inode
    // 有就有返回上锁后的 Inode，
    // 没有就返回 NULL
    Inode* NameI(const char *path);
    

    // Read, Write 公共部分
    int Rdwr(int fd, const void *src, int size, int mode);


    // 在 parInode 目录下加一个文件
    // 返回新分配的 Inode（上锁）
    // 要指定目录项的 offset
    Inode* MakNode(Inode *parInode, const char *name, int mode, int offset);

    // 根据 User 检查 mode 对应的权限
    // 同时会修改 User 的 u_error
    // 可以访问则返回 0
    int Access(Inode *pInode, int mode);

};






#endif