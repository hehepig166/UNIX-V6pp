#ifndef HEHEPIG_PARAMETERS_H
#define HEHEPIG_PARAMETERS_H

#ifndef NULL
    #define NULL (0)
#endif

#define KERNEL_IMG_PATH "kernel"
//#define DEFAULT_DEV "disk.img"

namespace PARAMS {

    // system
    const int PAGE_SIZE = 4096;
    const int BUF_PER_PAGE = 4096/512;

    // buf
    const int BUFFER_SIZE = 512;
    const int NBUF = 20;


    // device
    const int MAX_DEVICE_NUM = 5;
    const int NODEV = -1;
    const int ROOTDEV = 0;
    const int MAXLEN_DEVICE_NAME = 63;

    // filesystem
    static const int NMOUNT = 5;                            // 最大挂载数

    static const int SUPER_BLOCK_START_SECTOR = 200;        // SuperBlock 起始扇区
    static const int SUPER_BLOCK_SECTOR_NUMBER = 2;         // SuperBlock 占两个扇区（200，201）

    static const int ROOTINO = 1;                           // 文件系统根目录外存 Inode 编号

    static const int INODE_NUMBER_PER_SECTOR = 512/64;      // 每个磁盘块存放的 Inode 数
    static const int INODE_ZONE_START_SECTOR = 202;         // 外存 Inode 区的起始扇区号
    static const int INODE_ZONE_SIZE = 1024 - 202;          // 磁盘上外存 Inode 区占据的扇区数

    static const int DATA_ZONE_START_SECTOR = 1024;         // 数据区起始扇区号
    static const int DATA_ZONE_END_SECTOR = 200000 - 1;      // 数据区结束扇区号
    static const int DATA_ZONE_SIZE = 200000 - DATA_ZONE_START_SECTOR;   // 数据区扇区数



    // Inode
    static const int BLOCK_SIZE = 512;      // 文件逻辑块大小：512 B
    static const int ADDRESS_PER_INDEX_BLOCK = BLOCK_SIZE / sizeof(int);    // 128 每个间接索引块包含的物理盘块号个数

    static const int SMALL_FILE_BLOCK = 6;                              // 小型文件：直接索引表最多可寻址的逻辑块号
    static const int LARGE_FILE_BLOCK = 128 * 2 + 6;                    // 大型文件：经过一次间接索引表最多可寻址的逻辑块号
    static const int HUGE_FILE_BLOCK = 128 * 128 * 2 + 128 * 2 + 6;     // 巨型文件：经过两次简介索引表最多可寻址的逻辑块号

};





#endif