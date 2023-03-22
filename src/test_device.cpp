#include "DeviceManager.h"
#include "BufferManager.h"
#include "Buf.h"
#include "Kernel.h"

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

void create_img(const char *fname, int size) {
    int fd = open(fname, O_RDWR|O_CREAT, 00777);
    lseek(fd, size-1, SEEK_SET);
    write(fd, "\0", 1);
    close(fd);
}

void test_mmap()
{
    int fd = open("1.txt", O_RDWR, 00777);
    //lseek(fd, 1024, SEEK_SET);
    //write(fd, "123456", 5);
    char *p = (char*)mmap(NULL, 512, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 4096);
    puts(".");
    memcpy(p, "#####", 5);
    puts("..");
    munmap(p, 512);
    puts("...");
    close(fd);
}

void test()
{
    if (IS_ROOT_PROG) {
        Kernel::Instance().Initialize();
    }
    else {
        Kernel::Instance().ConnectInitialize("disk.img");
    }


    string op;
    string str;
    Buf *pBuf;

    while (1) {
        cout <<"\n> ";
        fflush(stdout);

        if (!IS_ROOT_PROG) Kernel::Instance().GetDeviceManager().ReOpen();

        cin >>op;
        if (op == "exit") {
            break;
        }
        if (op == "fork") {
            int pid = fork();
            if (pid == 0) {
                
                for (int i=1; i<=20; i++) {
                    pBuf = Kernel::Instance().GetBufferManager().GetBlk(1, 1);
                    cout <<"[son dev1 blk1] pBuf = " <<pBuf <<endl;
                    if (!pBuf) {
                        sleep(5);
                        continue;
                    }
                    cout <<"[son]???" <<endl;
                    cout <<pBuf->b_addr[0] <<endl;
                    //memcpy(strbuf, pBuf->b_addr, PARAMS::BUFFER_SIZE);
                    cout <<"[son]..." <<endl;
                    //strbuf[512] = 0;
                    //cout <<"[son]contents:\n=================\n" <<strbuf <<"\n======================" <<endl;

                    Kernel::Instance().GetBufferManager().RlsBlk(pBuf);
                    cout <<"[Son] sleep 5" <<endl;
                    sleep(5);
                }
                exit(0);
            }

            cout <<"[parent]..." <<endl;
            
        }
        if (op == "NewFile") {
            int size;
            cin >>strbuf >>size;
            create_img(strbuf, size);
        }
        if (op == "AddDev" && IS_ROOT_PROG) {
            string path;
            cin >>path;
            cout <<Kernel::Instance().GetDeviceManager().Add(path.c_str()) <<endl;
        }
        if (op == "Devs") {
            int ndev = Kernel::Instance().GetDeviceManager().GetNblkDev();
            auto &kn = Kernel::Instance();
            cout <<ndev <<endl;
            for (int i=0; i<ndev; i++) {
                cout <<"[" <<i <<"] " <<kn.GetDeviceManager().GetBlockDeviceName(i) <<endl;
            }
        }
        if (op == "Write") {
            int dev, blkno;
            string content;
            cin >>dev >>blkno >>strbuf;
            strbuf[511] = 0;

            pBuf = Kernel::Instance().GetBufferManager().GetBlk(dev, blkno);
            cout <<"pBuf = " <<pBuf <<endl;
            if (!pBuf) continue;

            memcpy(pBuf->b_addr, strbuf, strlen(strbuf));
            cout <<"copied." <<endl;

            Kernel::Instance().GetBufferManager().RlsBlk(pBuf);
        }
        if (op == "Read") {
            int dev, blkno;
            string content;
            cin >>dev >>blkno;

            pBuf = Kernel::Instance().GetBufferManager().GetBlk(dev, blkno);
            cout <<"pBuf = " <<pBuf <<endl;
            if (!pBuf) continue;

            memcpy(strbuf, pBuf->b_addr, PARAMS::BUFFER_SIZE);
            strbuf[512] = 0;
            cout <<"contents:\n=================\n" <<strbuf <<"\n======================" <<endl;

            Kernel::Instance().GetBufferManager().RlsBlk(pBuf);
        }
    }

}



int main(int argc, char **argv)
{
    IS_ROOT_PROG = (argc == 2 && argv[1][0]=='r');
    //IS_ROOT_PROG = 1;


    cout <<"sizeof(DeviceManager)  " <<sizeof(DeviceManager) <<endl;
    cout <<"sizeof(BufferManager)  " <<sizeof(BufferManager) <<endl;
    cout <<"sizeof(Inode)          " <<sizeof(Inode) <<endl;
    //test_mmap();
    test();
}