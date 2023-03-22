#include "Inode.h"
#include "Kernel.h"
#include "BufferManager.h"
#include "DeviceManager.h"
#include "Utility.h"
#include "Parameters.h"


Inode::Inode() {
    this->i_flag = 0;
    this->i_mode = 0;
    this->i_count = 0;
    this->i_nlink = 0;
    this->i_dev = -1;
    this->i_number = -1;
    this->i_uid = -1;
    this->i_gid = -1;
    this->i_size = 0;
    for(int i = 0; i < 10; i++)
    {
        this->i_addr[i] = 0;
    }
}



void Inode::ReadI() {
    int lbn;            // 文件逻辑块号
    int bn;             // 对应到物理盘块号
    int offset;         // 当前字符块内编号
    int nbytes;         // 剩余待传送字节数量
    int dev;
    Buf *pBuf;
    User& u = Kernel::Instance().GetUser();
    BufferManager& bufMgr = Kernel::Instance().GetBufferManager();

    if (u.u_IOParam.m_Count == 0) {
        return;
    }

    i_flag |= IACC;

    // 获取设备号、物理块号
    while (u.u_IOParam.m_Count > 0) {
        lbn = bn = u.u_IOParam.m_Offset / PARAMS::BLOCK_SIZE;
        offset = u.u_IOParam.m_Offset % PARAMS::BLOCK_SIZE;

        nbytes = Utility::Min(Inode::BLOCK_SIZE - offset, u.u_IOParam.m_Count);
        nbytes = Utility::Min(nbytes, i_size - u.u_IOParam.m_Offset);

        if (nbytes <= 0) {
            return;
        }

        bn = Bmap(lbn);
        dev = i_dev;

        // 读（映射）
        pBuf = bufMgr.GetBlk(dev, bn);

        // 拷贝到目标地址
        char *start = pBuf->b_addr + offset;
        Utility::ByteCopy(start, u.u_IOParam.m_Base, nbytes);

        u.u_IOParam.m_Base += nbytes;
        u.u_IOParam.m_Offset += nbytes;
        u.u_IOParam.m_Count -= nbytes;

        // 解放缓存块
        bufMgr.RlsBlk(pBuf);
    }
}



void Inode::WriteI() {
    int lbn;            // 文件逻辑块号
    int bn;             // 对应到物理盘块号
    int offset;         // 当前字符块内编号
    int nbytes;         // 剩余待传送字节数量
    short dev;
    Buf *pBuf;
    User& u = Kernel::Instance().GetUser();
    BufferManager& bufMgr = Kernel::Instance().GetBufferManager();

    if (u.u_IOParam.m_Count == 0) {
        return;
    }

    i_flag |= (Inode::IACC | Inode::IUPD);

    while (u.u_IOParam.m_Count > 0) {
        lbn = bn = u.u_IOParam.m_Offset / PARAMS::BLOCK_SIZE;
        offset = u.u_IOParam.m_Offset % PARAMS::BLOCK_SIZE;

        nbytes = Utility::Min(Inode::BLOCK_SIZE - offset, u.u_IOParam.m_Count);
        // 这里不用和 i_size 判大小，因为写可以超出文件大小，往后追加即可

        if ((bn=Bmap(lbn)) == 0) {
            return; // 找不到块了，可能没空间了
        }

        dev = i_dev;

        // 与硬盘 IO 不同，这里是内存映射，所以不用先读后写，直接映射即可
        pBuf = bufMgr.GetBlk(dev, bn);

        // 从目标地址拷贝
        char *start = pBuf->b_addr + offset;
        Utility::ByteCopy(u.u_IOParam.m_Base, start, nbytes);

        u.u_IOParam.m_Base += nbytes;
        u.u_IOParam.m_Offset += nbytes;
        u.u_IOParam.m_Count -= nbytes;

        // 普通文件长度增加
        if (i_size < u.u_IOParam.m_Offset) {
            i_size = u.u_IOParam.m_Offset;
        }

        i_flag |= Inode::IUPD;

        // 释放缓存
        bufMgr.RlsBlk(pBuf);
    }
}



int Inode::Bmap(int lbn) {
    Buf *pFirstBuf;
    Buf *pSecondBuf;
    int phyBlkno;           // 转换后的物理盘块号
    int *iTable;            // 用于访问索引盘块的间接索引表项
    int index;
    BufferManager& bufMgr = Kernel::Instance().GetBufferManager();
    FileSystem& fileSys = Kernel::Instance().GetFileSystem();

    // 文件索引结构：
    // i_addr[0..5]     直接索引表
    // i_addr[6..7]     一次间接索引表（指向一个直接索引表块）
    // i_addr[8..9]     二次间接索引表（指向一个一次直接索引表块）

    if (lbn >= Inode::HUGE_FILE_BLOCK) {
        Utility::Panic("err: file size too large.");
        return 0;
    }

    if (lbn < 6) {  // 小文件
        phyBlkno = this->i_addr[lbn];

        // 若 i_addr[lbn] 为空，则分配一个物理块
        if (phyBlkno == 0 && (pFirstBuf = fileSys.Alloc(this->i_dev)) != NULL) {
            bufMgr.RlsBlk(pFirstBuf);       // 释放缓存
            phyBlkno = pFirstBuf->b_blkno;  // 记录物理块号

            this->i_addr[lbn] = phyBlkno;
            this->i_flag |= Inode::IUPD;
        }

        return phyBlkno;
    }
    else {  // 大、巨文件

        // 计算所需的索引表项下标
        if (lbn < Inode::LARGE_FILE_BLOCK) {    // 大文件，用到一次间接索引表
            index = (lbn - Inode::SMALL_FILE_BLOCK) / Inode::ADDRESS_PER_INDEX_BLOCK + 6;
        }
        else {  // 巨文件，用到二次间接索引表
            index = (lbn - Inode::LARGE_FILE_BLOCK) / (Inode::ADDRESS_PER_INDEX_BLOCK * Inode::ADDRESS_PER_INDEX_BLOCK) + 8;
        }

        phyBlkno = this->i_addr[index];
        // 若 i_addr[index] 为空，则分配一个物理块
        if (phyBlkno == 0) {
            if ((pFirstBuf = fileSys.Alloc(this->i_dev)) == NULL) {
                Utility::Panic("cannot fileSys.Alloc() in Bmap");
                return 0;
            }
            // 这里不用释放缓存，因为后面哈要用
            this->i_addr[index] = phyBlkno;
            this->i_flag |= Inode::IUPD;
        }
        else {
            pFirstBuf = bufMgr.GetBlk(this->i_dev, phyBlkno);
        }

        // 获取一级索引表
        iTable = (int*)(pFirstBuf->b_addr);

        if (index >= 8) {   // 巨型文件，要找到二级索引表
            index = ( (lbn - Inode::LARGE_FILE_BLOCK) / Inode::ADDRESS_PER_INDEX_BLOCK ) % Inode::ADDRESS_PER_INDEX_BLOCK;

            phyBlkno = iTable[index];
            
            if (phyBlkno == 0) {
                if ((pSecondBuf = fileSys.Alloc(this->i_dev)) != NULL) {
                    Utility::Panic("cannot fileSys.Alloc() in Bmap");
                    bufMgr.RlsBlk(pFirstBuf);   // 记得释放缓存
                    return 0;
                }
                // 这里不用释放pSecond缓存，因为后面哈要用
                iTable[index] = pSecondBuf->b_blkno;  // 记录物理块号
                bufMgr.RlsBlk(pFirstBuf);   // 记得释放缓存
            }
            else {
                pSecondBuf = bufMgr.GetBlk(this->i_dev, phyBlkno);
                bufMgr.RlsBlk(pFirstBuf);   // 记得释放缓存
            }

            // 为了让后面的代码复用，这里往后将耳机索引块看作一级索引块
            iTable = (int*)(pSecondBuf->b_addr);
            pFirstBuf = pSecondBuf;
        }

        // 计算逻辑块号 lbn 最终位于“一级索引表”中的下标
        if (lbn < Inode::LARGE_FILE_BLOCK) {
            index = (lbn - Inode::SMALL_FILE_BLOCK) % Inode::ADDRESS_PER_INDEX_BLOCK;
        }
        else {
            index = (lbn - Inode::LARGE_FILE_BLOCK) % Inode::ADDRESS_PER_INDEX_BLOCK;
        }

        if( (phyBlkno = iTable[index]) == 0 && (pSecondBuf = fileSys.Alloc(this->i_dev)) != NULL) {
            phyBlkno = pSecondBuf->b_blkno;
            iTable[index] = phyBlkno;
            bufMgr.RlsBlk(pSecondBuf);
            bufMgr.RlsBlk(pFirstBuf);
        }
        else {
            bufMgr.RlsBlk(pFirstBuf);
            Utility::Panic("err in Bmap.");
            return 0;
        }

        return phyBlkno;
    }
}


void Inode::IUpdate(int tme) {
    Buf* pBuf;
    DiskInode dInode;
    FileSystem& filesys = Kernel::Instance().GetFileSystem();
    BufferManager& bufMgr = Kernel::Instance().GetBufferManager();

    // 没有要更新的
    if ((this->i_flag & (Inode::IUPD | Inode::IACC)) == 0) {
        return;
    }

    // 只读文件系统
    if (filesys.GetFS(this->i_dev)->s_ronly) {
        return;
    }

    // 缓存本Inode对应在外存中的块
    pBuf = bufMgr.GetBlk(this->i_dev, PARAMS::INODE_ZONE_START_SECTOR + this->i_number / PARAMS::INODE_NUMBER_PER_SECTOR);

    // 制作要覆盖外存的DiskInode
    dInode.d_mode = this->i_mode;
    dInode.d_nlink = this->i_nlink;
    dInode.d_uid = this->i_uid;
    dInode.d_gid = this->i_gid;
    dInode.d_size = this->i_size;
    for (int i=0; i<10; i++) {
        dInode.d_addr[i] = this->i_addr[i];
    }
    if (this->i_flag & Inode::IACC) {
        dInode.d_atime = tme;
    }
    if (this->i_flag & Inode::IUPD) {
        dInode.d_mtime = tme;
    }

    // p 指向 DiskInode 的块内偏移位置
    char *p = pBuf->b_addr + (this->i_number % PARAMS::INODE_NUMBER_PER_SECTOR) * sizeof(DiskInode);
    DiskInode *pNode = &dInode;

    // 写进外存
    Utility::DWordCopy((int*)pNode, (int*)p, sizeof(DiskInode)/sizeof(int));

    bufMgr.RlsBlk(pBuf);
}



void Inode::ITrunc() {
    BufferManager& bm = Kernel::Instance().GetBufferManager();
    FileSystem& filesys = Kernel::Instance().GetFileSystem();


    // FILO 方式释放，尽量使 SuperBlock 中空闲盘块号连续
    for (int i=9; i>=0; i--) {
        if (this->i_addr[i] == 0) {
            continue;
        }

        if (i>=6 && i<=9) {
            Buf *pFirstBuf = bm.GetBlk(this->i_dev, this->i_addr[i]);
            int *pFirst = (int*)(pFirstBuf->b_addr);

            for (int j=127; j>=0; j--) {
                if (pFirst[j] != 0) {
                    if (i >= 8 && i<=9) {
                        Buf *pSecondBuf = bm.GetBlk(this->i_dev, pFirst[j]);
                        int *pSecond = (int*)(pSecondBuf->b_addr);

                        for (int k=127; k>=0; k--) {
                            if (pSecond[k] != 0) {
                                filesys.Free(this->i_dev, pSecond[k]);
                            }
                        }
                        bm.RlsBlk(pSecondBuf);
                    }
                    filesys.Free(this->i_dev, pFirst[j]);
                }
            }

            bm.RlsBlk(pFirstBuf);
        }

        filesys.Free(this->i_dev, this->i_addr[i]);

        this->i_addr[i] = 0;
    }

    this->i_size = 0;
    this->i_flag |= Inode::IUPD;
    this->i_mode &= ~(Inode::ILARG & Inode::IRWXU & Inode::IRWXG & Inode::IRWXO);
    this->i_nlink = 1;
}


bool Inode::IsLocked() {
    return (i_flag & Inode::ILOCK)!=0;
}


void Inode::Lock() {
    while (i_flag & Inode::ILOCK) {
        i_flag |= Inode::IWANT;
        Utility::Sleep(1);
    }
    i_flag |= Inode::ILOCK;
}


void Inode::Unlock() {
    i_flag &= ~Inode::ILOCK;
}


void Inode::Clean() {
    //this->i_flag = 0;
    this->i_mode = 0;
    this->i_count = 0;
    this->i_nlink = 0;
    //this->i_dev = -1;
    //this->i_number = -1;
    this->i_uid = -1;
    this->i_gid = -1;
    this->i_size = 0;
    for(int i = 0; i < 10; i++)
    {
        this->i_addr[i] = 0;
    }
}


void Inode::ICopy(Buf *bp, int inumber)
{
    DiskInode dInode;
    DiskInode* pNode = &dInode;

    /* 将p指向缓存区中编号为inumber外存Inode的偏移位置 */
    char* p = bp->b_addr + (inumber % PARAMS::INODE_NUMBER_PER_SECTOR) * sizeof(DiskInode);
    /* 将缓存中外存Inode数据拷贝到临时变量dInode中，按4字节拷贝 */
    Utility::DWordCopy( (int *)p, (int *)pNode, sizeof(DiskInode)/sizeof(int) );

    /* 将外存Inode变量dInode中信息复制到内存Inode中 */
    this->i_mode = dInode.d_mode;
    this->i_nlink = dInode.d_nlink;
    this->i_uid = dInode.d_uid;
    this->i_gid = dInode.d_gid;
    this->i_size = dInode.d_size;
    for(int i = 0; i < 10; i++)
    {
        this->i_addr[i] = dInode.d_addr[i];
    }
}











DiskInode::DiskInode()
{
    /* 
     * 如果DiskInode没有构造函数，会发生如下较难察觉的错误：
     * DiskInode作为局部变量占据函数Stack Frame中的内存空间，但是
     * 这段空间没有被正确初始化，仍旧保留着先前栈内容，由于并不是
     * DiskInode所有字段都会被更新，将DiskInode写回到磁盘上时，可能
     * 将先前栈内容一同写回，导致写回结果出现莫名其妙的数据。
     */
    this->d_mode = 0;
    this->d_nlink = 0;
    this->d_uid = -1;
    this->d_gid = -1;
    this->d_size = 0;
    for(int i = 0; i < 10; i++)
    {
        this->d_addr[i] = 0;
    }
    this->d_atime = 0;
    this->d_mtime = 0;
}