#include "BufferManager.h"
#include "DeviceManager.h"
#include "Kernel.h"
#include "Parameters.h"

#include "Utility.h"

#include <sys/mman.h>

static const int flag_debug = 0;

void BufferManager::Initialize() {
    *this = BufferManager();
    m_DeviceManager = &Kernel::Instance().GetDeviceManager();
}

Buf* BufferManager::GetBlk(int dev, int blkno) {
    if (flag_debug) Utility::LogError("BufferManager::GetBlk");
    Buf *pBuf;
    Buf *pBuf_empty = NULL;
    Buf *pBuf_nouse = NULL;

    while (true) {
        // 找是否有已存在的对应缓存块（顺便找找空的缓存块和未引用的缓存块）
        for (int i=0; i<PARAMS::NBUF; i++) {
            if (m_Buf[i].is_devblk(dev, blkno)) {
                pBuf = &(m_Buf[i]);
                pBuf->b_count++;
                return pBuf;
            }
            else if (!m_Buf[i].is_connected()) {
                pBuf_empty = &(m_Buf[i]);
            }
            else if (m_Buf[i].b_count == 0) {
                pBuf_nouse = &(m_Buf[i]);
            }
        }


        // 没找到，则优先选用空的缓存块，没有再释放未引用的缓存块
        if (pBuf_empty) {
            pBuf = pBuf_empty;
            break;
        }
        else if (pBuf_nouse) {
            pBuf = pBuf_nouse;
            DelBlk(pBuf);
            break;
        }
        else {
            continue;
        }
    }
    
    pBuf->init(dev, blkno);

    BlockDevice *pdev = m_DeviceManager->GetBlockDevice(dev);
    if (!pdev) return NULL;
    int fd = pdev->fd;
    // mmap 的 offset 必须是页大小（4096）的倍数
    // 现在这样会有点浪费，不过贪方便好写一点，munmap释放的时候注意一下start就行
    int offset = blkno * PARAMS::BUFFER_SIZE / 4096 * 4096;
    int size = (blkno%PARAMS::BUF_PER_PAGE + 1) * PARAMS::BUFFER_SIZE;
    char *p = (char*)mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, offset);
    pBuf->b_addr = p + blkno*PARAMS::BUFFER_SIZE - offset;
    
    pBuf->b_count++;
    
    return pBuf;
}


void BufferManager::RlsBlk(Buf *bp) {
    if (flag_debug) Utility::LogError("BufferManager::RlsBlk");
    if (bp->b_count > 0) bp->b_count--;
}

void BufferManager::DelBlk(Buf *bp) {
    if (flag_debug) Utility::LogError("BufferManager::DelBlk");
    if (bp->is_connected()) {
        int blkno = bp->b_blkno;
        int offset = blkno * PARAMS::BUFFER_SIZE / 4096 * 4096;
        int size = (blkno+1) * PARAMS::BUFFER_SIZE - offset;
        char *p = bp->b_addr - (blkno*PARAMS::BUFFER_SIZE - offset);
        munmap(p, size);
    }
    bp->init();
}