#include "User.h"
#include "OpenFile.h"
#include "Kernel.h"
#include "Parameters.h"
#include "Utility.h"


void User::Initialize() {
    u_ofiles = OpenFiles();

    u_error = 0;

    u_cdir = Kernel::Instance().GetInodeTable().IGet(0, PARAMS::ROOTINO);
    u_cdir->Unlock();
    u_pdir = Kernel::Instance().GetInodeTable().IGet(0, PARAMS::ROOTINO);
    u_pdir->Unlock();

    u_uid = 0;
    u_gid = 0;

    Utility::StrCopy("/", u_curdirstr);
}


void User::Shutdown() {
    Kernel::Instance().GetInodeTable().IPut(u_cdir);
    Kernel::Instance().GetInodeTable().IPut(u_pdir);
    u_cdir = u_pdir = NULL;
}