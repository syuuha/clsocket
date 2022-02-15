#ifdef linux
    #include <stdio.h>
    #include <stdlib.h>
    #include <stdarg.h>
    #include <string.h>
    #include <unistd.h>
    #include <fcntl.h>
    #define  MAX_PATH 260
#else
    #include <Windows.h>
    #include <stdio.h>
    #include <string.h>
    #pragma warning(disable:4996)
#endif


#define CONF_FILE_PATH	"conf.ini"
#define MAX_BUF 1024


// //获取当前程序目录
int getCurrentPath(const char *pFileName, char buf[]);
//从配置文件读取字符串类型数据
int readConfigFile_String(const char *filename, const char *title, const char *key, char *value);
//从配置文件读取整类型数据
int readConfigFile_Int(const char *filename, const char *title, const char *key);
