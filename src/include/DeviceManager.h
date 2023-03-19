#ifndef HEHEPIG_DEVICE_H
#define HEHEPIG_DEVICE_H


#include "Buf.h"
#include "Parameters.h"



class BlockDevice {

public:
    int fd;
    char path[PARAMS::MAXLEN_DEVICE_NAME];

public:
    BlockDevice();

    bool is_open();

    int Open(const char *path, int asfd = -1);  // asfd 若不是正数，则尝试打开到指定fd
    int Close();
};



class DeviceManager {

private:
    int nblkdev;
    BlockDevice blkdevsw[PARAMS::MAX_DEVICE_NUM];

public:
    DeviceManager();

    void Initialize();

    void ReOpen();

    int Add(const char *path);

    int Del(int dev);
    
    int GetNblkDev();

    const char* GetBlockDeviceName(int dev);

    BlockDevice* GetBlockDevice(int dev);
};




#endif