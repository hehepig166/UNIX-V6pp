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
    memset(strbuf, 0, sizeof(strbuf));
    for (int i=0; i<blknum+10; i++) {
        lseek(fd, i*blksize, SEEK_SET);
        write(fd, strbuf, blksize);
    }

    close(fd);
//=================================================

    Kernel::Instance().Initialize();

    for (int i=blknum-1; i>=PARAMS::DATA_ZONE_START_SECTOR; i--) {
        Kernel::Instance().GetFileSystem().Free(0, i);
    }

    cout <<"Blocks format done." <<endl;

    Buf *pBuf;

    pBuf = Kernel::Instance().GetFileSystem().Alloc(0);
    memset(strbuf, '1', sizeof(strbuf));
    Utility::ByteCopy(strbuf, pBuf->b_addr, 512);
    Kernel::Instance().GetBufferManager().RlsBlk(pBuf);
    cout <<"fill 1 in block " <<(pBuf->b_blkno) <<endl;

    pBuf = Kernel::Instance().GetFileSystem().Alloc(0);
    memset(strbuf, '2', sizeof(strbuf));
    Utility::ByteCopy(strbuf, pBuf->b_addr, 512);
    Kernel::Instance().GetBufferManager().RlsBlk(pBuf);
    cout <<"fill 2 in block " <<(pBuf->b_blkno) <<endl;

    cout <<"free blk 1024 " <<endl;
    Kernel::Instance().GetFileSystem().Free(0, 1024);

    pBuf = Kernel::Instance().GetFileSystem().Alloc(0);
    memset(strbuf, '#', sizeof(strbuf));
    Utility::ByteCopy(strbuf, pBuf->b_addr, 128);
    Kernel::Instance().GetBufferManager().RlsBlk(pBuf);
    cout <<"write 128 # in block " <<(pBuf->b_blkno) <<endl;


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

    test_format(argv[1]);

    return 0;
}