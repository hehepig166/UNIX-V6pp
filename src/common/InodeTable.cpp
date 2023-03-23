#include "InodeTable.h"
#include "Inode.h"
#include "Kernel.h"
#include "Utility.h"


static const int flag_debug = 0;


static FileSystem *m_FileSystem;


void InodeTable::Initialize() {
    m_FileSystem = &Kernel::Instance().GetFileSystem();
}

void InodeTable::Reset() {
    for (int i=0; i<NINODE; i++) {
        m_Inode[i] = Inode();
    }
}


Inode* InodeTable::IGet(int dev, int inumber) {
    if (flag_debug) Utility::LogError("InodeTable::IGet");
    Inode *pInode;
    

    while (true) {
        int index = IsLoaded(dev, inumber);

        if (index >= 0) {   // 有内存映射

            pInode = &m_Inode[index];
            
            // 上锁
            pInode->Lock();

            pInode->i_count++;

            // 连接了子文件系统
            if (pInode->i_flag & Inode::IMOUNT) {
                Mount *pMount = m_FileSystem->GetMount(pInode);
                if (pMount == NULL) {
                    pInode->Unlock();
                    Utility::Panic("No Mount Tab...");
                }
                else {
                    dev = pMount->m_dev;
                    inumber = FileSystem::ROOTINO;
                    continue;   // 在新的文件系统中找根目录
                }
            }
            
            return pInode;
        }
        else {      // 无内存映射，要创建一个映射
            pInode = GetFreeInode();

            if (pInode == NULL) {
                Utility::Sleep(1);
                continue;   // 等，直到有位置                
            }

            // 上锁，并填充
            pInode->Lock();
            pInode->i_dev = dev;
            pInode->i_number = inumber;
            pInode->i_count = 1;

            BufferManager &bm = Kernel::Instance().GetBufferManager();

            Buf *pBuf = bm.GetBlk(dev, PARAMS::INODE_ZONE_START_SECTOR + inumber/PARAMS::INODE_NUMBER_PER_SECTOR );

            // 外存Inode信息拷贝到新分配的Inode中
            pInode->ICopy(pBuf, inumber);

            bm.RlsBlk(pBuf);
            return pInode;
        }
    }
}


void InodeTable::IPut(Inode *pInode) {
    if (flag_debug) Utility::LogError("InodeTable::IPut");
    //if (pInode->i_count == 1) {
    if (true) {
        
        // 上锁
        pInode->i_flag |= Inode::ILOCK;

        // 更新外存Inode
        pInode->IUpdate(Utility::GetTime());

        // 解锁
        pInode->Unlock();

        if (pInode->i_count == 1) {
            pInode->i_flag = 0;
            pInode->i_number = -1;
        }
    }

    pInode->i_count--;
    pInode->Unlock();
}


void InodeTable::UpdateInodeTable() {
    if (flag_debug) Utility::LogError("InodeTable::UpdateInodeTable");
    for (int i=0; i<InodeTable::NINODE; i++) {
        if ((m_Inode[i].i_flag & Inode::ILOCK)==0 && m_Inode[i].i_count != 0) {
            m_Inode[i].Lock();
            m_Inode[i].IUpdate(Utility::GetTime());
            m_Inode[i].Unlock();
        }
    }
}


int InodeTable::IsLoaded(int dev, int inumber) {
    for (int i=0; i<InodeTable::NINODE; i++) {
        if (m_Inode[i].i_dev==dev && m_Inode[i].i_number==inumber && m_Inode[i].i_count != 0) {
            return i;
        }
    }
    return -1;
}


Inode* InodeTable::GetFreeInode() {
    if (flag_debug) Utility::LogError("InodeTable::GetFreeInode");
    while (true) {
        for (int i=0; i<InodeTable::NINODE; i++) {
            // 引用计数为零，说明空闲，这与 Buf 不同
            if (!m_Inode[i].IsLocked() && m_Inode[i].i_count == 0) {
                return &(m_Inode[i]);
            }
        }
    }
    return NULL;
}


int InodeTable::GetIndex(Inode *pInode) {
    return (pInode-m_Inode);
}