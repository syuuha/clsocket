#include "readConfigFile.h"

//获取当前程序目录
int getCurrentPath(const char *pFileName, char buf[])
{
#ifdef linux
    char pidfile[64] = {0};
    int bytes = 0;
    int fd = 0;
    sprintf(pidfile, "/proc/%d/cmdline", getpid());
    fd = open(pidfile, O_RDONLY, 0);
    bytes = read(fd, buf, 256);
    close(fd);
    buf[MAX_PATH] = '\0';
#else
    GetModuleFileName(NULL, buf, MAX_PATH);
#endif
    char * p = &buf[strlen(buf)];
    do
    {
        *p = '\0';
        p--;
#ifdef linux
    } while( '/' != *p );
#else
    } while( '\\' != *p );
#endif
    p++;
    //配置文件目录
    memcpy(p, pFileName, strlen(pFileName));
    return 0;
}

//从配置文件读取字符串类型数据
int readConfigFile_String(const char *filename, const char *title, const char *key, char *value)
{
    FILE *fp = NULL;
    char szLine[MAX_BUF] = {0};
    static char tmpstr[MAX_BUF] = {0};
    int rtnval = 0;
    int i = 0;
    int flag = 0;
    char *tmp = NULL;
    bool isFirst = true;  //是否为第一次扫描配置文件
    bool isEnd = false;    //是否扫描完配置文件
    if((fp = fopen(filename, "r")) == NULL)
    { 
        printf("no found conf.ini : %s\n",filename);
        return -1;
    }
    while(!feof(fp))
    {
        if (isEnd) break;  //扫描配置文件结束
        rtnval = fgetc(fp);
        if(rtnval == EOF)
        {
            isEnd = true;
        }
        else
        {
            szLine[i++] = rtnval;
        }
        if(rtnval == '\n' || isFirst == true || isEnd == true)  //第一次扫描配置文件 第一行不用是\n
        {
            if (isFirst == false && isEnd != true) //是否为第一次扫描文件
            {
                szLine[--i] = '\0';
                i = 0;
            }
            tmp = strchr(szLine, '=');
            if(( tmp != NULL )&&(flag == 1))
            {
                if(strstr(szLine, key) != NULL)
                {
                    //注释行
                    if ('#' == szLine[0])  // "#"comment, eg,#ip=0.0.0.0
                    {
                    }
                    else if ( '/' == szLine[0] && '/' == szLine[1] ) // "//"comment, eg,//port=25
                    {
                    }
                    else
                    {
                        //找到key对应变量
                        strcpy(value, tmp+1);
                        fclose(fp);
                        return 0; 
                    }
                }
                else
                {
                    memset(szLine, 0, MAX_BUF);
                }
            }
            else
            {
                strcpy(tmpstr, "[");
                strcat(tmpstr, title);
                strcat(tmpstr, "]");
                if( strncmp(tmpstr, szLine, strlen(tmpstr)) == 0 )
                {
                    //找到title
                    flag = 1;
                }
            }
            isFirst=false;
        }
    }
    fclose(fp);
    return -1;
}

//从配置文件读取整类型数据
int readConfigFile_Int(const char *filename, const char *title, const char *key)
{
    char value_string[MAX_BUF] = {0};

    if(0 == readConfigFile_String(filename, title, key, value_string))  //成功
    {
        return atoi(value_string); //atol
    }
    else //失败
    {
        return -1;
    }
}

// int main(int argc, char* argv[])
// {
//     char buf[MAX_PATH] = {0};
//     memset(buf, 0, sizeof(buf));
//     getCurrentPath(buf, CONF_FILE_PATH);
//     strcpy(g_szConfigPath, buf);
//     printf("path:%s\n", g_szConfigPath);
//     char ip[32] = {0};
//     int port = -1;
//     if (0 != readConfigFile_String(g_szConfigPath, "INICnet", "ip", ip))
//     {
//         printf("no ip key\r\n");
//     }
//     printf("ip:%s\n", ip); fflush(stdout); fflush(stdout);
//     port = readConfigFile_Int(g_szConfigPath, "INICnet", "port");
//     if (-1 == port)
//     {
//         printf("no port key\r\n"); fflush(stdout); fflush(stdout);
//     }
//     printf("port:%d\n", port);

// #ifdef WIN32
//     system("pause");
// #endif // WIN32
//     return 0;
// }
