


class Utility {

public:
    static int GetTime();
    static void Panic(const char *content);
    static void LogError(const char *content);
    static void Sleep(int secs);
    static void ByteCopy(const char *src, char *dst, int count);
    static void DWordCopy(const int *src, int *dst, int count);
    static int Min(const int x, const int y);
};