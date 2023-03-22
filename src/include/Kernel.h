#ifndef HEHEPIG_KERNEL_H
#define HEHEPIG_KERNEL_H

#include "DeviceManager.h"
#include "BufferManager.h"
#include "Inode.h"
#include "InodeTable.h"
#include "FileSystem.h"
#include "IOParameter.h"
#include "OpenFile.h"
#include "FileManager.h"
#include "User.h"






class Kernel {

private:
    static Kernel instance;

    DeviceManager   *m_DeviceManager;
    BufferManager   *m_BufferManager;

    InodeTable      *m_InodeTable;
    FileSystem      *m_FileSystem;

    OpenFileTable   *m_OpenFileTable;

    FileManager     *m_FileManager;

    User            *m_User;

public:

    static Kernel& Instance();

    void Initialize(const char *rootdev_path = "disk.img"); // 初始化新的内核，并创建镜像文件
    void Connect(const char *path);                         // 连接镜像文件
    void ConnectInitialize(const char *path);               // 连接并根据镜像文件初始化（复制）内核
    void Shutdown();        // 清一些引用

    BufferManager&  GetBufferManager();
    DeviceManager&  GetDeviceManager();

    InodeTable&     GetInodeTable();
    FileSystem&     GetFileSystem();

    OpenFileTable&  GetOpenFileTable();

    FileManager&    GetFileManager();

    User&           GetUser();
};


#endif