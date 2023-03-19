#include "Kernel.h"
#include "IOParameter.h"
#include "Parameters.h"

#include <sys/mman.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

Kernel Kernel::instance;


BufferManager   g_BufferManager;
User            g_User;


Kernel& Kernel::Instance() {
    return Kernel::instance;
}



void Kernel::Initialize(const char *rootdev_path) {

    m_BufferManager     = &g_BufferManager;
    m_User              = &g_User;

    Connect(KERNEL_IMG_PATH);

    GetDeviceManager().Initialize();

    GetBufferManager().Initialize();

    GetDeviceManager().Add(rootdev_path);

    GetInodeTable().Initialize();
    GetInodeTable().Reset();

    GetFileSystem().Initialize();
    GetFileSystem().Reset();
    GetFileSystem().LoadSuperBlock();

}

void Kernel::Connect(const char *path) {
    int fd = open(path, O_RDWR|O_CREAT, 00777);
    lseek(fd, PARAMS::PAGE_SIZE-1, SEEK_SET);
    write(fd, "\0", 1);
    void *p = mmap(0, PARAMS::PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);

    int szsum=0;

    m_DeviceManager = (DeviceManager*)(p);
    szsum += sizeof(DeviceManager);

    m_InodeTable = (InodeTable*)((char*)p+szsum);
    szsum += sizeof(InodeTable);

    m_FileSystem = (FileSystem*)((char*)p+szsum);
    szsum += sizeof(FileSystem);
    
}

void Kernel::ConnectInitialize(const char *path) {

    Connect(path);

    // 已有的 Device 要重新连一下，因为 fd 不同了
    m_DeviceManager->ReOpen();

    m_BufferManager     = &g_BufferManager;
    m_User              = &g_User;

    // Buffer 要重置，进程间的 Buffer 独立（不在内核镜像文件中）。
    // 虽然独立，但都用 mmap 映射，相同的块会被映射到宿主机同一页面，所以还行
    m_BufferManager->Initialize();

}


BufferManager& Kernel::GetBufferManager() {
    return *(m_BufferManager);
}

DeviceManager& Kernel::GetDeviceManager() {
    return *(m_DeviceManager);
}

FileSystem& Kernel::GetFileSystem() {
    return *(m_FileSystem);
}

InodeTable& Kernel::GetInodeTable() {
    return *(m_InodeTable);
}

User& Kernel::GetUser() {
    return *(m_User);
}