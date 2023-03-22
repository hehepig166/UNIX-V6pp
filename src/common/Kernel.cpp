#include "Kernel.h"
#include "IOParameter.h"
#include "Parameters.h"

#include <sys/mman.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

Kernel Kernel::instance;


static BufferManager    g_BufferManager;
static FileSystem       g_FileSystem;
static FileManager      g_FileManager;
static User             g_User;


Kernel& Kernel::Instance() {
    return Kernel::instance;
}



void Kernel::Initialize(const char *rootdev_path) {

    m_BufferManager     = &g_BufferManager;
    m_FileSystem        = &g_FileSystem;
    m_FileManager       = &g_FileManager;
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

    GetOpenFileTable().Reset();

    GetFileManager().Initialize();

    GetUser().Initialize();

}

void Kernel::Connect(const char *path) {
    int fd = open(path, O_RDWR|O_CREAT, 00777);
    lseek(fd, 4*PARAMS::PAGE_SIZE, SEEK_SET);
    write(fd, "\0", 1);
    void *p = mmap(0, 4*PARAMS::PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);

    int szsum=0;

    m_DeviceManager = (DeviceManager*)(p);
    szsum += sizeof(DeviceManager);

    m_InodeTable = (InodeTable*)((char*)p+szsum);
    szsum += sizeof(InodeTable);

    m_OpenFileTable = (OpenFileTable*)((char*)p+szsum);
    szsum += sizeof(OpenFileTable);
    
}

void Kernel::ConnectInitialize(const char *path) {

    Connect(KERNEL_IMG_PATH);

    m_BufferManager     = &g_BufferManager;
    m_FileSystem        = &g_FileSystem;
    m_FileManager       = &g_FileManager;
    m_User              = &g_User;

    // 已有的 Device 要重新连一下，因为 fd 不同了
    m_DeviceManager->ReOpen();

    // Buffer 要重置，进程间的 Buffer 独立（不在内核镜像文件中）。
    // 虽然独立，但都用 mmap 映射，相同的块会被映射到宿主机同一页面，所以还行
    m_BufferManager->Initialize();
    
    // FileSystem 的 m_spb 和 M_mount 里的 m_spb 要初始化
    m_FileSystem->Initialize();
    m_FileSystem->Reset();
    m_FileSystem->LoadSuperBlock();

    GetFileManager().Initialize();

    m_User->Initialize();
}



void Kernel::Shutdown() {
    m_User->Shutdown();
    m_FileManager->ShutDown();
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

OpenFileTable& Kernel::GetOpenFileTable() {
    return *(m_OpenFileTable);
}

FileManager& Kernel::GetFileManager() {
    return *(m_FileManager);
}

User& Kernel::GetUser() {
    return *(m_User);
}