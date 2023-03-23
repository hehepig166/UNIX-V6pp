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

static const int flag_debug = 0;

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
    if (flag_debug) Utility::LogError("FileManager::Open");
    Inode *pInode;

    pInode = NameI(path);

    if (pInode == NULL) {
        return -1;
    }

    return Open1(pInode, mode, 0);
}



int FileManager::Create(const char *path, int mode) {
    if (flag_debug) if (flag_debug) Utility::LogError("FileManager::Create");
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
            // 同名文件存在，且没有正在被使用，则重置该文件
            m_InodeTable->IPut(pInode);
            pInode = NameI(path);
            if (pInode->i_count == 0) {
                return Open1(pInode, File::FWRITE, 1);
            }
            else {
                m_InodeTable->IPut(pInode);
                Utility::LogError("the file is being used");
            }
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
    if (flag_debug) Utility::LogError("FileManager::Open1");
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
    //pInode->Unlock();

    File *pFile = m_OpenFileTable->FAlloc();
    int fd = u.u_tmp;
    if (pFile == NULL) {
        m_InodeTable->IPut(pInode);
        return -1;
    }

    pFile->f_flag = mode & (File::FREAD | File::FWRITE);
    pFile->f_ino = Kernel::Instance().GetInodeTable().GetIndex(pInode);

    pInode->Unlock();

    // 本进程 fd 在 FAlloc 中存在了 u.u_tmp 中
    return fd;
}


int FileManager::Close(int fd) {
    if (flag_debug) Utility::LogError("FileManager::Close");
    User &u = Kernel::Instance().GetUser();
    File *pFile = u.u_ofiles.GetF(fd);
    if (pFile == NULL) {
        return -1;
    }

    u.u_ofiles.SetF(fd, NULL);
    m_OpenFileTable->CloseF(pFile);
    return 0;
}



int FileManager::Read(int fd, void *src, int size) {
    return Rdwr(fd, src, size, File::FREAD);
}

int FileManager::Write(int fd, void *src, int size) {
    return Rdwr(fd, src, size, File::FWRITE);
}




int FileManager::Rdwr(int fd, const void *src, int size, int mode) {
    if (flag_debug) Utility::LogError("FileManager::Rdwr");
    File *pFile;
    User &u = Kernel::Instance().GetUser();

    if ((pFile = u.u_ofiles.GetF(fd)) == NULL) {
        Utility::LogError("fd does not exist.");
        return -1;
    }

    if ((pFile->f_flag & mode) == 0) {
        Utility::LogError("wrong access to the opened file.");
        return -1;
    }

    u.u_IOParam.m_Base = (char*)src;
    u.u_IOParam.m_Count = size;
    u.u_IOParam.m_Offset = pFile->f_offset;

    pFile->GetInode()->Lock();

    if (mode == File::FREAD) {
        pFile->GetInode()->ReadI();
    }
    else {
        pFile->GetInode()->WriteI();
    }

    pFile->f_offset += (size - u.u_IOParam.m_Count);
    pFile->GetInode()->Unlock();

    return size - u.u_IOParam.m_Count;
}



int FileManager::MkDir(const char *path) {
    User &u = Kernel::Instance().GetUser();
    Inode *pInode;

    int cIno = 0;   // 新目录 . ino
    int pIno = 0;   // 新目录 .. ino
    auto parentPath = Utility::GetParentPath(path);
    auto lastPath = Utility::GetLastPath(path);

    // 已存在
    if ((pInode = NameI(path)) != NULL) {
        Utility::LogError("path already exists.");
        m_InodeTable->IPut(pInode);
        return -1;
    }


    // 父目录不存在
    if ((pInode = NameI(parentPath.c_str())) == NULL) {
        Utility::LogError((parentPath+" doesn't exists.").c_str());
        m_InodeTable->IPut(pInode);
        return -1;
    }
    pIno = pInode->i_number;

    // 创建目录文件
    int ACCMode = Inode::IRWXU | Inode::IRWXG | Inode::IRWXO;
    Inode *newInode = MakNode(pInode, lastPath.c_str(), ACCMode, GetDirList(pInode)[0].second);
    m_InodeTable->IPut(pInode);
    if (newInode == NULL) {
        return -1;
    }
    cIno = newInode->i_number;

    // 设置目录属性
    newInode->i_mode &= ~Inode::IFMT;
    newInode->i_mode |= Inode::IFDIR;

    // 加入 . 和 ..
    DirectoryEntry tmpEnt[2];
    tmpEnt[0] = DirectoryEntry(cIno, ".");
    tmpEnt[1] = DirectoryEntry(pIno, "..");
    u.u_IOParam.m_Base = (char*)tmpEnt;
    u.u_IOParam.m_Offset = 0;
    u.u_IOParam.m_Count = sizeof(tmpEnt);
    newInode->WriteI();


    m_InodeTable->IPut(newInode);

    return 0;
}


int FileManager::SetCurDir(const char *path) {
    User &u = Kernel::Instance().GetUser();
    Inode *pInode;
    
    pInode = NameI(path);
    if (pInode == NULL) {
        return -1;
    }
    pInode->Unlock();       // 直接解锁（但保留引用）
    if ((pInode->i_mode & Inode::IFMT) != Inode::IFDIR) {
        // 检查是否是目录
        m_InodeTable->IPut(pInode);
        return -1;
    }

    // 切换 u.u_cidr
    m_InodeTable->IPut(u.u_cdir);
    u.u_cdir = pInode;

    // 父目录也改，这时肯定存在，所以不判 NULL 和 FDIR 了
    auto parpath = Utility::GetParentPath(path);
    pInode = NameI(parpath.c_str());
    pInode->Unlock();
    m_InodeTable->IPut(u.u_pdir);
    u.u_pdir = pInode;

    // 修改 u_curdirstr
    std::string newdirstr;
    if (path[0] == '/') {
        newdirstr = path;
    }
    else {
        newdirstr = u.u_curdirstr;
        newdirstr += "/";
        newdirstr += path;
    }
    newdirstr = Utility::SimplifyAbsPath(newdirstr.c_str());
    Utility::StrCopy(newdirstr.c_str(), u.u_curdirstr, sizeof(u.u_curdirstr));

    return 0;
}





std::vector<std::pair<DirectoryEntry, int>> FileManager::GetDirList(Inode *pInode) {
    if (flag_debug) Utility::LogError("FileManager::GetDirList");
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
    if (flag_debug) Utility::LogError("FileManager::MakNode");
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
    if (flag_debug) Utility::LogError("FileManager::NameI");
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

        bool flag_found = false;

        for (const auto &[dent, offset]: dirlist) {
            if (fname == dent.m_name) {
                int dev = pInode->i_dev;
                m_InodeTable->IPut(pInode);
                pInode = m_InodeTable->IGet(dev, dent.m_ino);
                if (pInode == NULL) {
                    return NULL;    // 这里不能 goto out
                }
                flag_found = true;
            }
        }
        
        if (!flag_found) goto out;   // 没找到
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
