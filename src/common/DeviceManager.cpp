#include "DeviceManager.h"
#include "Utility.h"

#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include <cstdio>


//========================================
// BlockDevice
//========================================

BlockDevice::BlockDevice() {
    fd = -1;
}

bool BlockDevice::is_open() {
    return fd >= 0;
}

int BlockDevice::Open(const char *fpath, int asfd) {
    //strcpy(this->path, fpath);
    Utility::ByteCopy(fpath, this->path, Utility::Min(strlen(fpath)+1, PARAMS::MAXLEN_DEVICE_NAME));
    this->path[PARAMS::MAXLEN_DEVICE_NAME-1] = 0;
    
    // 系统分配 fd
    if (asfd <= 0) {
        fd = open(fpath, O_RDWR);
        return fd;
    }

    // 指定 fd
    else {
        int tmpfd, tmp;

        if ((tmpfd=open(fpath, O_RDWR)) < 0) return -1;
        if ((tmp = dup2(tmpfd, asfd)) < 0) {
            close(tmpfd);
            return -1;
        }
        if (tmpfd != tmp) close(tmpfd);
        return fd = asfd;
    }
}

int BlockDevice::Close() {
    memset(path, 0, sizeof(path));
    return is_open() ? close(fd) : -1;
}




//========================================
// DeviceManager
//========================================

DeviceManager::DeviceManager() {
    nblkdev = 0;
    for (int i=0; i<PARAMS::MAX_DEVICE_NUM; i++) blkdevsw[i].fd = -1;
}

void DeviceManager::Initialize() {
    *this = DeviceManager();
}

void DeviceManager::ReOpen() {
    for (int i=0; i<PARAMS::MAX_DEVICE_NUM; i++) if (blkdevsw[i].is_open()) {
        blkdevsw[i].Open(blkdevsw[i].path, blkdevsw[i].fd);
    }
}

int DeviceManager::Add(const char *path) {
    for (int i=0; i<PARAMS::MAX_DEVICE_NUM; i++) if (!blkdevsw[i].is_open()) {
        blkdevsw[i].Open(path);
        if (blkdevsw[i].is_open()) {
            nblkdev++;
            return i;
        }
    }
    return -1;
}

int DeviceManager::Del(int dev) {
    if (dev < PARAMS::MAX_DEVICE_NUM)
        return blkdevsw[dev].Close();
    return -1;
}

int DeviceManager::GetNblkDev() {
    return nblkdev;
}

const char* DeviceManager::GetBlockDeviceName(int dev) {
    if (dev < PARAMS::MAX_DEVICE_NUM && blkdevsw[dev].is_open())
        return blkdevsw[dev].path;
    return NULL;
}

BlockDevice* DeviceManager::GetBlockDevice(int dev) {
    if (dev < PARAMS::MAX_DEVICE_NUM && blkdevsw[dev].is_open())
        return &(blkdevsw[dev]);
    return NULL;
}