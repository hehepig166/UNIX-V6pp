
#include <vector>
#include <string>


class Utility {

public:
    static int GetTime();
    static void Panic(const char *content);
    static void LogError(const char *content);
    static void Sleep(int secs);
    static void ByteCopy(const char *src, char *dst, int count);
    static void DWordCopy(const int *src, int *dst, int count);
    static int Min(const int x, const int y);
    static int StrCopy(const char *src, char *dst, const int mxlen=2147483647, const char endchar='\0');
    static void DWordMemset(void *s, int v, int c);

    static std::vector<std::string> SplitPath(const char *path);    // 若是绝对路径，ret[0] = "/"
    static std::string GetParentPath(const char *path); // 除去末端分量
    static std::string GetLastPath(const char *path);   // 最末端的分量
    static std::string SimplifyAbsPath(const char *path);  // 化简（./.. 之类的）
};