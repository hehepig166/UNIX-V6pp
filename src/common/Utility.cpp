#include "Utility.h"

#include <string.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>

#include <vector>
#include <string>

int Utility::GetTime() {
    return (int)time(NULL);
}

void Utility::Panic(const char *content) {
    printf("%s\n", content);
    exit(-1);
}

void Utility::LogError(const char *content) {
    printf("%s\n", content);
}

void Utility::Sleep(int secs) {
    Utility::LogError("Sleep");
    sleep(secs);
    //getchar();
}

void Utility::ByteCopy(const char *src, char *dst, int count) {
    memcpy(dst, src, count);
}

void Utility::DWordCopy(const int *src, int *dst, int count) {
    while (count--) {
        *dst++ = *src++;
    }
    return;
}

int Utility::Min(const int x, const int y) {
    return x<y ? x : y;
}

int Utility::StrCopy(const char *src, char *dst, const int mxlen, const char endchar) {
    int i;
    for (i=0; i<mxlen-1 && src[i] && src[i]!=endchar; i++)
        dst[i] = src[i];
    dst[i] = '\0';
    return i;
}

void Utility::DWordMemset(void *s, int v, int c) {
    int *p = (int*)s;
    while (c--) {
        *p++ = v;
    }
}



std::vector<std::string> Utility::SplitPath(const char *path) {
    std::vector<std::string> ret;
    std::string curstr;
    const char *p = path;

    // 若以 '/' 开头，说明是绝对路径，记下 '/'
    if (*p == '/') {
        ret.push_back("/");
        p++;
    }
    else {
        ret.push_back(".");     // 默认本地路径
    }

    while (*p) {
        while (*p && *p=='/') p++;  // 过滤多余的 '/'
        if (!*p) break;
        // 读取名字直到 '/' 或结束
        while (*p && *p!='/') curstr+=*(p++);
        if (curstr.empty()) continue;
        ret.push_back(curstr);
        curstr.clear();
    }

    return ret;
}


std::string Utility::GetParentPath(const char *path) {
    auto dlist = SplitPath(path);
    std::string ret;
    for (int i=0, mi=dlist.size()-1; i<mi; i++) {
        ret += dlist[i];
        ret += "/";
    }
    return ret;
}


std::string Utility::GetLastPath(const char *path) {
    auto dlist = SplitPath(path);
    return dlist.back();
}