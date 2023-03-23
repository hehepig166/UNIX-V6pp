#include "DeviceManager.h"
#include "BufferManager.h"
#include "Buf.h"
#include "FileSystem.h"
#include "Parameters.h"
#include "Kernel.h"
#include "Utility.h"

#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

using namespace std;

const int blksize   = 512;
const int blknum    = PARAMS::DATA_ZONE_END_SECTOR+1;
const int fsize     = blknum * blksize;
const int inodenum  = PARAMS::INODE_ZONE_SIZE * PARAMS::INODE_NUMBER_PER_SECTOR;

char strbuf[1024];
int fd;

SuperBlock  sb;
DiskInode   inodes[inodenum];




void add_block(int blkno);


void test_format(const char *fpath)
{

//=================================================
    fd = open(fpath, O_RDWR|O_CREAT, 00777);
    if (fd < 0) {
        cout <<"failed..." <<endl;
        return;
    }

    // 清零
    // memset(strbuf, 0, sizeof(strbuf));
    // for (int i=0; i<blknum+10; i++) {
    //     lseek(fd, i*blksize, SEEK_SET);
    //     write(fd, strbuf, blksize);
    // }
    lseek(fd, (blknum+5)*blksize-1, SEEK_SET);
    write(fd, "\0", 1);

    close(fd);

    cout <<"file " <<fpath <<" created.." <<endl;
//=================================================
// init
    Kernel::Instance().Initialize(fpath);

    auto &u = Kernel::Instance().GetUser();
    auto &fs = Kernel::Instance().GetFileSystem();
    auto &bm = Kernel::Instance().GetBufferManager();
    auto &itb = Kernel::Instance().GetInodeTable();
    auto &fm = Kernel::Instance().GetFileManager();
    auto *psb = fs.GetFS(0);

    psb->s_isize = inodenum;
    psb->s_fsize = PARAMS::DATA_ZONE_SIZE;

    Buf     *pBuf = NULL;
    Inode   *pnode = NULL;
    int     t1, t2;

    for (int i=blknum-1; i>=PARAMS::DATA_ZONE_START_SECTOR; i--) {
        Kernel::Instance().GetFileSystem().Free(0, i);
    }
    cout <<"Blocks format done." <<endl;

    pnode = itb.IGet(0, PARAMS::ROOTINO);
    pnode->i_mode = Inode::IFDIR | Inode::IRWXU | Inode::IRWXG | Inode::IRWXO;
    pBuf = fs.Alloc(0);
    t1 = pBuf->b_blkno;
    bm.RlsBlk(pBuf);
    pnode->i_addr[0] = t1;
    pnode->i_size = 0;

    DirectoryEntry tmpEnt[2];
    tmpEnt[0] = DirectoryEntry(pnode->i_number, ".");
    tmpEnt[1] = DirectoryEntry(pnode->i_number, "..");
    u.u_IOParam.m_Base = (char*)tmpEnt;
    u.u_IOParam.m_Offset = 0;
    u.u_IOParam.m_Count = sizeof(tmpEnt);
    pnode->WriteI();

    itb.IPut(pnode);
    cout <<"rootdir created." <<endl;

    cout <<"mkdir /home  " <<"return " <<fm.MkDir("/home") <<endl;
    cout <<"mkdir /dev   " <<"return " <<fm.MkDir("/dev") <<endl;
    cout <<"mkdir /etc   " <<"return " <<fm.MkDir("/etc") <<endl;
    cout <<"mkdir /root  " <<"return " <<fm.MkDir("/root") <<endl;

/*
    t1 = fm.Create("/1.txt", FileManager::CREATE);
    cout <<"create file /1.txt [" <<t1 <<"]" <<endl;

    t2 = fm.Create("/2.txt", FileManager::CREATE);
    cout <<"create file /2.txt [" <<t2 <<"]" <<endl;

    cout <<"close [" <<t1 <<"] return " <<fm.Close(t1) <<endl;

    t1 = fm.Create("/3.txt", FileManager::CREATE);
    cout <<"create file /3.txt [" <<t1 <<"]" <<endl;

    cout <<"close [" <<t1 <<"] return " <<fm.Close(t1) <<endl;
    cout <<"close [" <<t2 <<"] return " <<fm.Close(t2) <<endl;
*/

    return;

//==================================================
// test block alloc/free

    cout <<"\n=================test alloc/free=================" <<endl;

    pBuf = fs.Alloc(0);
    memset(strbuf, '1', sizeof(strbuf));
    Utility::ByteCopy(strbuf, pBuf->b_addr, 512);
    bm.RlsBlk(pBuf);
    cout <<"alloc and fill 1 in block " <<(t1 = pBuf->b_blkno) <<endl;

    pBuf = fs.Alloc(0);
    memset(strbuf, '2', sizeof(strbuf));
    Utility::ByteCopy(strbuf, pBuf->b_addr, 512);
    bm.RlsBlk(pBuf);
    cout <<"alloc and fill 2 in block " <<(t2 = pBuf->b_blkno) <<endl;

    fs.Free(0, t1);
    cout <<"free blk " <<t1 <<endl;

    pBuf = Kernel::Instance().GetFileSystem().Alloc(0);
    memset(strbuf, '#', sizeof(strbuf));
    Utility::ByteCopy(strbuf, pBuf->b_addr, 128);
    bm.RlsBlk(pBuf);
    cout <<"alloc and write 128 # in block " <<(t1 = pBuf->b_blkno) <<endl;

    fs.Free(0, t1);
    cout <<"free blk " <<t1 <<endl;

    fs.Free(0, t2);
    cout <<"free blk " <<t2 <<endl;


//===================================================
// test inode alloc/free

    cout <<"\n=================test ialloc/ifree=================" <<endl;

    pnode = fs.IAlloc(0);
    t1 = pnode->i_number;
    itb.IPut(pnode);
    cout <<"ialloc " <<t1 <<endl;

    pnode = fs.IAlloc(0);
    t2 = pnode->i_number;
    itb.IPut(pnode);
    cout <<"ialloc " <<t2 <<endl;

    fs.IFree(0, t1);
    cout <<"ifree " <<t1 <<endl;

    pnode = fs.IAlloc(0);
    t1 = pnode->i_number;
    itb.IPut(pnode);
    cout <<"ialloc " <<t1 <<endl;

    fs.IFree(0, t1);
    cout <<"ifree " <<t1 <<endl;

    fs.IFree(0, t2);
    cout <<"ifree " <<t2 <<endl;

}






void add_block(int blkno) {

}






int main(int argc, char **argv)
{
    if (argc != 2) {
        cout <<"usage: " <<argv[0] <<" disk.img" <<endl;
        return 0;
    }

    cout <<"sizeof(DeviceManager)   " <<sizeof(DeviceManager) <<endl;
    cout <<"sizeof(BufferManager)   " <<sizeof(BufferManager) <<endl;
    cout <<"sizeof(Inode)           " <<sizeof(Inode) <<endl;
    cout <<"sizeof(SuperBlock)      " <<sizeof(SuperBlock) <<endl;
    cout <<"===================================\n" <<endl;

    test_format(argv[1]);

    Kernel::Instance().Shutdown();

    return 0;
}