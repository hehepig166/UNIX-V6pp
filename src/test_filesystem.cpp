#include "DeviceManager.h"
#include "BufferManager.h"
#include "Buf.h"
#include "Kernel.h"
#include "Utility.h"

#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>

#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

using namespace std;

bool IS_ROOT_PROG;

char strbuf[1024];


void test()
{

    if (IS_ROOT_PROG) {
        Kernel::Instance().Initialize("disk.img");
    }
    else {
        Kernel::Instance().ConnectInitialize("disk.img");
    }

    auto fs = Kernel::Instance().GetFileSystem();

    string op;
    string str;
    Buf *pBuf;

    while (1) {
        cout <<"\n> ";
        fflush(stdout);

        if (!IS_ROOT_PROG) Kernel::Instance().GetDeviceManager().ReOpen();

        cin >>op;

        
    }


}



int main(int argc, char **argv)
{
    IS_ROOT_PROG = (argc == 2 && argv[1][0]=='r');
    //IS_ROOT_PROG = 1;


    cout <<"sizeof(DeviceManager)   " <<sizeof(DeviceManager) <<endl;
    cout <<"sizeof(BufferManager)   " <<sizeof(BufferManager) <<endl;
    cout <<"sizeof(Inode)           " <<sizeof(Inode) <<endl;
    cout <<"sizeof(SuperBlock)      " <<sizeof(SuperBlock) <<endl;
    test();
}