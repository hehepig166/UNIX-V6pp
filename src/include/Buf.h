#ifndef HEHEPIG_BUF_H
#define HEHEPIG_BUF_H


// 若mmap了，则b_addr非零，
// unmmap 之后必须置零，用这个判断是否正在映射
// 不做读写权限判断，想让 FileManager 负责

struct Buf {
    int     b_count;            // 引用次数

    int     b_dev;
    int     b_blkno;
    char    *b_addr;

    Buf(): b_count(0), b_dev(-1), b_blkno(0), b_addr(0) {}
    void init(int dev=-1, int blkno=0) {b_count=0; b_dev=dev; b_blkno=blkno; b_addr=0;}

    bool is_connected() {return b_addr != 0;}
    bool is_devblk(int dev, int blkno) {return is_connected() && b_dev==dev && b_blkno==blkno;}
};


#endif