#include "FileManager.h"
#include "User.h"
#include "Inode.h"
#include "BufferManager.h"
#include "FileSystem.h"
#include "InodeTable.h"
#include "OpenFile.h"
#include "Kernel.h"
#include "Utility.h"

#include <vector>
#include <string>
#include <cstring>

static FileSystem      *m_FileSystem;
static InodeTable      *m_InodeTable;
static OpenFileTable   *m_OpenFileTable;
static Inode           *rootDirInode;

void FileManager::Initialize() {
    m_FileSystem = &Kernel::Instance().GetFileSystem();
    m_InodeTable = &Kernel::Instance().GetInodeTable();
    m_OpenFileTable = &Kernel::Instance().GetOpenFileTable();

    rootDirInode = m_InodeTable->IGet(PARAMS::ROOTDEV, PARAMS::ROOTINO);
    rootDirInode->Unlock();
}

void FileManager::ShutDown() {
    m_InodeTable->IPut(rootDirInode);
}






int FileManager::Open(const char *path, int mode) {
    Inode *pInode;

    pInode = NameI(path);

    if (pInode == NULL) {
        return -1;
    }

    return Open1(pInode, mode, 0);
}



int FileManager::Create(const char *path, int mode) {
    Inode *pInode;
    int newACCMode = mode & (Inode::IRWXU|Inode::IRWXG|Inode::IRWXO);

    auto parentPath = Utility::GetParentPath(path);
    auto lastPath = Utility::GetLastPath(path);

    // 此处会上锁
    pInode = NameI(parentPath.c_str());
    
    if (pInode == NULL) {
        Utility::LogError("no parent dir");
        return -1;
    }

    auto dirlist = GetDirList(pInode);
    for (auto &[ent, offset]: dirlist) {
        if (lastPath == ent.m_name) {
            // 同名文件存在，则重置该文件
            m_InodeTable->IPut(pInode);
            return Open1(pInode, File::FWRITE, 1);
        }
    }

    // 同名文件不存在，创建文件
   
    Inode *newInode = MakNode(pInode, lastPath.c_str(), newACCMode, dirlist[0].second);
    m_InodeTable->IPut(pInode);
    if (newInode == NULL) {
        return -1;
    }
    return Open1(newInode, File::FWRITE, 2);
}

int FileManager::FileManager::Open1(Inode *pInode, int mode, int trf) {
    User &u = Kernel::Instance().GetUser();

    if (trf != 2) {
        if (mode & File::FREAD) {
            Access(pInode, Inode::IREAD);
        }
        if (mode & File::FWRITE) {
            Access(pInode, Inode::IWRITE);
            if ((pInode->i_mode & Inode::IFMT) == Inode::IFDIR) {
                u.u_error = 1;
                Utility::LogError("trying to modify dir file!");
            }
        }
    }

    if (u.u_error) {
        m_InodeTable->IPut(pInode);
    }

    // create 时搜索到同名文件，会释放该文件所占据的所有盘块
    if (trf == 1) {
        pInode->ITrunc();
    }

    // 这里可以解锁了（来自 NameI 的锁），因为后面没有大量的文件操作了
    pInode->Unlock();

    File *pFile = m_OpenFileTable->FAlloc();
    int fd = u.u_tmp;
    if (pFile == NULL) {
        m_InodeTable->IPut(pInode);
        return -1;
    }

    pFile->f_flag = mode & (File::FREAD | File::FWRITE);
    pFile->f_ino = Kernel::Instance().GetInodeTable().GetIndex(pInode);

    // 本进程 fd 在 FAlloc 中存在了 u.u_tmp 中
    return fd;
}


int FileManager::Close(int fd) {
    User &u = Kernel::Instance().GetUser();
    File *pFile = u.u_ofiles.GetF(fd);
    if (pFile == NULL) {
        return -1;
    }

    u.u_ofiles.SetF(fd, NULL);
    m_OpenFileTable->CloseF(pFile);
    return 0;
}








std::vector<std::pair<DirectoryEntry, int>> FileManager::GetDirList(Inode *pInode) {
    int tmp_offset = 0;
    int tmp_rest = pInode->i_size / sizeof(DirectoryEntry);
    BufferManager &bufMgr = Kernel::Instance().GetBufferManager();
    Buf *pBuf = NULL;
    DirectoryEntry tmp_dentry;
    std::vector<std::pair<DirectoryEntry, int>> ret;

    // verify
    if (    pInode == NULL ||
            (pInode->i_mode & Inode::IFMT) != Inode::IFDIR ||
            Access(pInode, Inode::IEXEC) != 0
    ) return ret;


    ret.push_back({DirectoryEntry(), tmp_rest*sizeof(DirectoryEntry)});

    // 遍历目录文件
    while (tmp_rest > 0) {

        // 需要读入下一数据块
        if (tmp_offset%PARAMS::BLOCK_SIZE == 0) {
            if (pBuf) {
                bufMgr.RlsBlk(pBuf);
            }
            int phyBlkno = pInode->Bmap(tmp_offset/PARAMS::BLOCK_SIZE);
            pBuf = bufMgr.GetBlk(pInode->i_dev, phyBlkno);
        }

        // 读出目录项
        Utility::ByteCopy(
            (char*)(pBuf->b_addr + (tmp_offset % PARAMS::BLOCK_SIZE)),
            (char*)&tmp_dentry,
            sizeof(DirectoryEntry)
        );

        // 空闲目录项
        if (tmp_dentry.m_ino == 0 && ret[0].second < 0) {
            ret[0].second = tmp_offset;
            continue;
        }

        // 非空闲目录项，加入 ret
        ret.push_back({tmp_dentry, tmp_offset});

        tmp_offset += sizeof(DirectoryEntry);
        tmp_rest --;
    }

    if (pBuf) {
        bufMgr.RlsBlk(pBuf);
    }

    return ret;
}




Inode* FileManager::MakNode(Inode *parInode, const char *name, int mode, int offset) {
    Inode *pInode;
    User &u = Kernel::Instance().GetUser();

    // 此处返回的是上锁的
    pInode = m_FileSystem->IAlloc(parInode->i_dev);
    if (pInode == NULL) {
        return NULL;
    }
    


    DirectoryEntry tmpdent;
    Utility::StrCopy(name, tmpdent.m_name, DirectoryEntry::DIRSIZ);
    tmpdent.m_ino = pInode->i_number;

    pInode->i_flag |= (Inode::IACC | Inode::IUPD);
    pInode->i_mode = mode | Inode::IALLOC;
    pInode->i_nlink = 1;
    pInode->i_uid = u.u_uid;
    pInode->i_gid = u.u_gid;

    u.u_IOParam.m_Count = sizeof(DirectoryEntry);
    u.u_IOParam.m_Offset = offset;
    u.u_IOParam.m_Base = (char*)&tmpdent;

    parInode->WriteI();
    return pInode;
}



Inode* FileManager::NameI(const char *path) {
    auto splited_path = Utility::SplitPath(path);
    User &u = Kernel::Instance().GetUser();
    Inode *pInode;

    if (splited_path[0] == "/") {
        pInode = rootDirInode;
    }
    else if (splited_path[0] == ".") {
        pInode = u.u_cdir;
    }
    else {
        Utility::Panic("err in NameI!");
    }

    // 上锁，加引用
    m_InodeTable->IGet(pInode->i_dev, pInode->i_number);

    for (int i=1, mi=splited_path.size(); i<mi; i++) {
        const auto &fname = splited_path[i];

        if (fname == ".") { // 原地 tp
            continue;
        }

        auto dirlist = GetDirList(pInode);

        if (dirlist.empty()) goto out;

        for (const auto &[dent, offset]: dirlist) {
            if (fname == dent.m_name) {
                int dev = pInode->i_dev;
                m_InodeTable->IPut(pInode);
                pInode = m_InodeTable->IGet(dev, dent.m_ino);
                if (pInode == NULL) {
                    return NULL;    // 这里不能 goto out
                }
            }
        }

        goto out;   // 没找到
    }

    return pInode;  // 走到这说明找到了最终节点，不解锁返回

out:    // 统一的错误出口，解锁 pInode ，引用减一
    m_InodeTable->IPut(pInode);
    return NULL;

}





int FileManager::Access(Inode *pInode, int mode) {
    User &u = Kernel::Instance().GetUser();
    u.u_tmp = 0;
    return 0;
}