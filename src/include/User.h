#ifndef HEHEPIG_USER_H
#define HEHEPIG_USER_H

#include "IOParameter.h"
#include "OpenFile.h"
#include "Inode.h"
#include "FileManager.h"


class User {
    
public:
    IOParameter     u_IOParam;
    OpenFiles       u_ofiles;

    int             u_tmp;
    int             u_error;

    Inode           *u_pdir;        // 指向父目录文件（已经在根目录则还是根目录）
    Inode           *u_cdir;        // 指向当前目录文件

    char            u_curdirstr[128];  // 当前工作目录完整路径

    //DirectoryEntry  u_dent;         // 临时目录项

    short           u_uid;
    short           u_gid;

public:
    void Initialize();
    void Shutdown();
    

};



#endif