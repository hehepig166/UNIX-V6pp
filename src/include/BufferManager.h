#ifndef HEHEPIG_BUFFER_MANAGER_H
#define HEHEPIG_BUFFER_MANAGER_H

#include "DeviceManager.h"
#include "Buf.h"
#include "Parameters.h"


class BufferManager {

public:
    Buf             m_Buf[PARAMS::NBUF];
    DeviceManager   *m_DeviceManager;

public:

    void Initialize();

    Buf* GetBlk(int dev, int blkno);
    void RlsBlk(Buf *bp);   // count--
    void DelBlk(Buf *bp);   // munmap

};


#endif