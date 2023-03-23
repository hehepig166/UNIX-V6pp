#include "FileSystem.h"
#include "Kernel.h"
#include "Utility.h"
#include "Buf.h"

#include <sys/mman.h>
#include <cstring>

static const int flag_debug = 0;

static BufferManager   *m_BufferManager;
static InodeTable      *m_InodeTable;



void FileSystem::Initialize() {

    // alloc m_spb;
    m_spb = NULL;

    // realloc m_spb in m_Mount
    for (int i=0; i<PARAMS::NMOUNT; i++) m_Mount[i].m_spb = GetSuperBlock(m_Mount[i].m_dev);

    m_BufferManager = &Kernel::Instance().GetBufferManager();
    m_InodeTable = &Kernel::Instance().GetInodeTable();
}

void FileSystem::Reset() {
    for (int i=0; i<PARAMS::NMOUNT; i++) m_Mount[i] = Mount();
}

void FileSystem::Shutdown() {
    
}



void FileSystem::LoadSuperBlock() {
    if (flag_debug) Utility::LogError("FileSystem::LoadSuperBlock");
    m_spb = GetSuperBlock(PARAMS::ROOTDEV);
    if (m_spb == NULL) {
        Utility::Panic("cannot map superblock of rootdev!");
    }

    m_Mount[0].m_dev = PARAMS::ROOTDEV;
    m_Mount[0].m_spb = m_spb;
    m_Mount[0].index = ROOTINO;

    m_spb->s_flock = 0;
    m_spb->s_ilock = 0;
    m_spb->s_ronly = 0;
    m_spb->s_time = Utility::GetTime();
}


SuperBlock* FileSystem::GetSuperBlock(int dev) {
    if (flag_debug) Utility::LogError("FileSystem::GetSuperBlock");
    if (dev < 0) return NULL;
    SuperBlock *ret = NULL;
    int fd = Kernel::Instance().GetDeviceManager().GetBlockDevice(dev)->fd;
    if (fd <= 0) {
        return NULL;
    }
    ret = (SuperBlock*)mmap(0, sizeof(SuperBlock), PROT_READ|PROT_WRITE, MAP_SHARED, fd, SUPER_BLOCK_START_SECTOR*PARAMS::BUFFER_SIZE);
    if (ret == MAP_FAILED) {
        return NULL;
    }

    return ret;
}


SuperBlock* FileSystem::GetFS(int dev) {
    if (flag_debug) Utility::LogError("FileSystem::GetFS");
    for (int i=0; i<PARAMS::NMOUNT; i++) {
        if (m_Mount[i].m_spb!=NULL && m_Mount[i].m_dev==dev) {
            return m_Mount[i].m_spb;
        }
    }
    return NULL;
}


Inode* FileSystem::IAlloc(int dev) {
    if (flag_debug) Utility::LogError("FileSystem::IAlloc");
    SuperBlock *sb;
    Buf *pBuf;
    Inode *pNode;
    int ino;        // 分配到的空闲外存Inode编号

    sb = GetFS(dev);

    while (sb->s_ilock) {
        Utility::Sleep(1);
    }

    // 空闲 Inode 索引表空了
    // 上锁，搜索空闲 Inode 加入，解锁
    if (sb->s_ninode <= 0) {

        sb->s_ilock = 1;

        ino = -1;

        for (int i=0; i<sb->s_isize; i++) {
            pBuf = m_BufferManager->GetBlk(dev, PARAMS::INODE_ZONE_START_SECTOR + i);

            int *p = (int*)pBuf->b_addr;

            // 找空闲 Inode
            for (int j=0; j<PARAMS::INODE_NUMBER_PER_SECTOR; j++) {
                ino++;

                // 0 不用（Inode 下标从 1 开始）
                if (ino == 0) continue;

                // 外存 Inode 占用与否
                int mode = *(p + j*sizeof(DiskInode)/sizeof(int));
                if (mode != 0) {
                    continue;
                }

                // 还要判断是否有延迟写的 inode
                if (m_InodeTable->IsLoaded(dev, ino) == -1) {

                    // 加入空闲 Inode 表
                    sb->s_inode[sb->s_ninode++] = ino;

                    // 满了就停
                    if (sb->s_ninode >= 100) {
                        break;
                    }
                }
            }

            // 可以释放缓存了
            m_BufferManager->RlsBlk(pBuf);

            if (sb->s_ninode >= 100) {
                break;
            }
        }
        
        if (sb->s_ninode <= 0) {
            sb->s_ilock = 0;
            Utility::LogError("No Space On Disk!  -IAlloc\n");
            return NULL;
        }
    }

    // 到此处空闲 Inode 表非空
    while (true) {
        // 出栈
        ino = sb->s_inode[--sb->s_ninode];

        // 引用加一，加锁
        pNode = m_InodeTable->IGet(dev, ino);

        if (pNode == NULL) {
            sb->s_ilock = 0;
            return NULL;
        }

        if (pNode->i_mode == 0) {
            pNode->Clean();

            sb->s_fmod = 1;
            sb->s_ilock = 0;
            return pNode;
        }
        else {
            // 不用这个了，引用减一
            m_InodeTable->IPut(pNode);
            continue;
        }
    }

    sb->s_ilock = 0;
    return NULL;
}



void FileSystem::IFree(int dev, int number) {
    if (flag_debug) Utility::LogError("FileSystem::IFree");
    SuperBlock *sb;

    sb = GetFS(dev);

    if (sb->s_ilock) {
        return;
    }

    if (sb->s_ninode >= 100) {
        return;
    }

    sb->s_ilock = 1;

    sb->s_inode[sb->s_ninode++] = number;

    sb->s_ilock = 0;
}



Buf* FileSystem::Alloc(int dev) {
    if (flag_debug) Utility::LogError("FileSystem::Alloc");
    int blkno;      // 分配到的空闲磁盘块编号
    SuperBlock *sb;
    Buf *pBuf;

    sb = GetFS(dev);

    while (sb->s_flock) {
        Utility::Sleep(1);
    }
    sb->s_flock = 1;

    // 从索引表栈顶获取空闲磁盘块编号
    blkno = sb->s_free[--sb->s_nfree];

    sb->s_flock = 0;

    // 若磁盘块编号为零，说明空闲磁盘块分配完了
    if (blkno == 0) {
        sb->s_nfree = 0;
        sb->s_flock = 0;
        Utility::LogError("No Space on Disk! -Alloc\n");
        return NULL;
    }

    // 栈空，说明刚刚那个磁盘块记录了下一组空闲磁盘块的信息
    // 先用它的内容更新空闲磁盘块栈 s_free
    if (sb->s_nfree <= 0) {
        sb->s_flock = 1;

        pBuf = m_BufferManager->GetBlk(dev, blkno);

        // 该磁盘块结构： 4(s_nfree) + 400(s_free[100]) 个字节
        int *p = (int*)pBuf->b_addr;

        sb->s_nfree = *p++;

        Utility::DWordCopy(p, sb->s_free, 100);

        m_BufferManager->RlsBlk(pBuf);

        sb->s_flock = 0;
    }

    pBuf = m_BufferManager->GetBlk(dev, blkno);

    // 清空数据
    Utility::DWordMemset(pBuf->b_addr, 0, PARAMS::BUFFER_SIZE/sizeof(int));

    return pBuf;
}



void FileSystem::Free(int dev, int blkno) {
    if (flag_debug) Utility::LogError("FileSystem::Free");
    SuperBlock *sb = NULL;
    Buf *pBuf = NULL;

    sb = GetFS(dev);

    while (sb->s_flock) {
        Utility::Sleep(1);
    }


    if (sb->s_nfree <= 0) {
        sb->s_nfree = 1;
        sb->s_free[0] = 0;  // 用 0 标记空闲盘块链结束
    }

    // 空闲盘块栈已满
    if (sb->s_nfree >= 100) {
        sb->s_flock = 1;

        pBuf = m_BufferManager->GetBlk(dev, blkno);

        int *p = (int*)pBuf->b_addr;

        *p++ = sb->s_nfree;

        Utility::DWordCopy(sb->s_free, p, 100);

        sb->s_nfree = 0;

        sb->s_flock = 0;

        m_BufferManager->RlsBlk(pBuf);
    }

    // 现在栈有足够空间塞入一个项了
    sb->s_free[sb->s_nfree++] = blkno;
}



Mount* FileSystem::GetMount(Inode *pInode) {
    for (int i=0; i<FileSystem::NMOUNT; i++) {
        Mount *pMount = &m_Mount[i];

        if (pMount->index == m_InodeTable->GetIndex(pInode)) {
            return pMount;
        }
    }
    return NULL;
}



bool FileSystem::BadBlock(SuperBlock *spb, short dev, int blkno)
{
	return false;
}