/*
 Executable "bak":
 It is a bak-tool with simple backup operation.
 Version 2.0.1 GPL License
 by: liuchonghui@139.com
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <utime.h>

#define DEBUG (0)
#define DEBUGA (0)
#define BAK_HAVE_TAIL
#undef  BAK_HAVE_TAIL
#define PATHLEN 256
/* global variables */
const char version[]={"2.0.5\0"};
const char tip=',';//'\\';//'.';
static unsigned char need_backup=0;
static unsigned char is_copy_regfile_to_baktype=0;
/* check argc numbers, if no input, bak return */
static int require_input_argv(int argc);
#if DEBUG
/* DEBUG: show argv[argc] */
static int DEBUG_argv_list(int argc, char** argv);
#endif
/* "bak -h" or "bak --help" notes */
static int inline	show_help_notes();
/* "bak -version" notes */
static int inline	show_version_notes();
/* invalid input option warning */
static int inline invalid_option_warning(char** argv);
/* recursive option: only work for directory */
static int do_recursive_option(int argc, char** argv);
/* bak regular files and bak .bak-format.bak files */
static int do_bak(int argc, char** argv);

/* In do_bak, check it's a .c .h .java or .bak file */
static char* get_file_type(char* args);
/* In do_bak, run if comes a bak file */
static char* do_bak_bakfile(char* buf, char* tail, char** argv, int argc);
/* In do_bak->do_bak_bakfile, get .*.bak name */
static char* get_src_name(char* buf);
/* In do_bak->do_bak_bakfile, convert srcname to path route */
static char* convert_src_name_to_path(char* buf);
/* In do_bak, run if comes a regular file */
static char* do_bak_regfile(char* buf, char* tail, char** argv, int argc);
/* In do_bak->do_bak_regfile, use for strcat(buf, argv[argc]) */
static char* strcat_buf_and_argv(char* buf, char** argv, int argc);
/* In do_bak->do_bak_regfile, use for ../../../ type convert */
static char* adjust_buf_to_ordinary(char* buf);
/* In do_bak->do_bak_regfile->adjust_buf_to_ordinary, use for check /../ mode */
static int check_splash_dot_dot_splash(char* buf);
/* In do_bak->do_bak_regfile, in buf, change splash to dot and add path information */
static char* change_splash_to_dot_and_strcat_path_and_buf(char* path, char* buf, char* tail);
/* In do_bak, cp args buf, to get target */
static int do_copy(char** argv, int argc, char* buf);
/* In do_copy, check input file is a format bak file */
static int check_any_bak_file(char* buf);


int main(int argc, char** argv)
{
    if(require_input_argv(argc)) return -1;
#if DEBUG
    DEBUG_argv_list(argc, argv);
#endif
    int flag=0;
    if(!strcmp(argv[1], "-h")||!strcmp(argv[1], "--help")) 	flag = 1;
    else if(!strcmp(argv[1], "-R")) 							flag = 2;
    else if(!strcmp(argv[1], "--version")) 					flag = 3;
    else if(!strcmp(argv[1], "-b")) {need_backup = 1;		flag = 0;}
    else if(*argv[1] == '-') 									flag = -1;
    switch(flag)
    {
        case 0: do_bak(argc, argv);
            break;
        case 1: show_help_notes();
            break;
        case 2: do_recursive_option(argc, argv);
            break;
        case 3: show_version_notes();
            break;
        default: invalid_option_warning(argv);
            break;
    };
    return 0;//main
}

static int check_any_bak_file(char* buf)
{
    int ret=0;
    //	if(*buf != '.')
    //	{
    //		ret = 1;
    //		return ret;
    //	}
    char* cp = buf + strlen(buf) - 4;
#if DEBUG
    fprintf(stderr, ">>>> DEBUG: check_format_file. cp is '%s'\n", cp);
#endif
    if(!strcmp(cp, ".bak"))
    {
        //		while(cp != buf)
        //		{
        //			if(*cp == tip)
        //			{
        ret = 0;
        //				return ret;
        //			}
        //			if(cp != buf) cp = cp - 1;
        //		}
    }
    else ret = 1;
    //	ret = 1;
    return ret;
}

static int do_recursive_option(int argc, char** argv)// -r -R -recursive
{
#if DEBUG
    fprintf(stderr, "> WARNING. calling do_recursive_option.\n");
#endif
    struct stat statbuf1;
    char buf[PATHLEN];
    while(argc-- > 2)
    {
        if(stat(argv[argc], &statbuf1) < 0)
        {
            perror(argv[argc]);
            continue;
        }
        memset(buf, 0, PATHLEN);
        getcwd(buf, sizeof(buf) - 1);
        if(S_ISDIR(statbuf1.st_mode))//dir type
        {
            fprintf(stderr, "bak: '%s' it's a dir, we do recursive option.\n", argv[argc]);
        }
        else
        {
            fprintf(stderr, "bak: -R option. '%s' is not a directory.\n", argv[argc]);
            continue;
        }
    }
    return 0;
}

static int do_bak(int argc, char** argv)// do bak
{
    struct stat statbuf1;
    char buf[PATHLEN];
    int base_name=1;
    if(need_backup) base_name = 2;
    while(argc-- > base_name)//1)   0927
    {
        if(stat(argv[argc], &statbuf1) < 0)
        {
            perror(argv[argc]);
            continue;
        }
        memset(buf, 0, PATHLEN);
        getcwd(buf, sizeof(buf) - 1);
#if DEBUG
        fprintf(stderr, "> DEBUG. do_bak: buf is '%s', argv[%d] is '%s'\n", buf, argc, argv[argc]);
#endif
        if(S_ISREG(statbuf1.st_mode))
        {
#if DEBUG
            fprintf(stderr, "> NOTICE. do_bak: It's a regular file, we like it.\n");
#endif
            char* cp = buf;
            char* bp = get_file_type(argv[argc]);
#if DEBUG
            fprintf(stderr, "> NOTICE. do_bak: get file path is '%s'\n", cp);
            fprintf(stderr, "> NOTICE. do_bak: get file type is '%s'\n", bp);
#endif
#if DEBUG
            fprintf(stderr, "> DEBUG. calling do_bak_xxxfile: argv[%d] is '%s', cp is '%s', argv[%d]=%x, cp=%x\n", argc, argv[argc], cp, argc, (unsigned int)argv[argc], (unsigned int)cp);
#endif
            if(!strcmp(bp, ".bak"))//cp .*.bak -> *
            {
                is_copy_regfile_to_baktype = 0;
                cp = do_bak_bakfile(cp, bp, argv, argc);//return cp: /home/guest/project/a.c
            }
            else//cp * -> .*.bak
            {
                is_copy_regfile_to_baktype = 1;
                cp = do_bak_regfile(cp, bp, argv, argc);//return cp: /home/gest/project/.home.guest.project.a.c.c.bak
            }/* 0923 */
#if DEBUG
            fprintf(stderr, "> DEBUG. calling do_copy: argv[%d] is '%s', cp is '%s', argv[%d]=%x, cp=%x\n", argc, argv[argc], cp, argc, (unsigned int)argv[argc], (unsigned int)cp);
#endif
            do_copy(argv, argc, cp);
        }
        else if(S_ISDIR(statbuf1.st_mode))//dir type
        {
            fprintf(stderr, "Directory: %s, bak do not support S_ISDIR\n", argv[argc]);
        }
        else if(S_ISBLK(statbuf1.st_mode))//block type
        {
            fprintf(stderr, "Block: %s, bak do not support S_ISBLK\n", argv[argc]);
        }
        else if(S_ISCHR(statbuf1.st_mode))//char type
        {
            fprintf(stderr, "Char: %s, bak do not support S_ISCHR\n", argv[argc]);
        }
        else if(S_ISFIFO(statbuf1.st_mode))//fifo type
        {
            fprintf(stderr, "Fifo: %s, bak do not support S_ISFIFO\n", argv[argc]);
        }
        else if(S_ISLNK(statbuf1.st_mode))//link type
        {
            fprintf(stderr, "Link: %s, bak do not support S_ISLNK\n", argv[argc]);
        }
        else continue;
    }//end of while
    return 0;
}

static char* do_bak_bakfile(char* buf, char* tail, char** argv, int argc)
{
    char* path = 0;
    path = malloc(sizeof(buf));
    strcpy(path, buf);
#if DEBUG
    fprintf(stderr, ">> DEBUG. do_bak_bakfile: buf is '%s', argv[%d] is '%s', buf=%x, argv[%d]=%x\n", buf, argc, argv[argc], (unsigned int)buf, argc, (unsigned int)argv[argc]);
    fprintf(stderr, ">> DEBUG. do_bak_bakfile: path is '%s'\n", path);
#endif
    buf = strcat_buf_and_argv(buf, argv, argc);
#if DEBUG
    fprintf(stderr, ">> DEBUG. do_bak_bakfile: called strcat_buf_and_argv: buf is '%s', argv[%d] is '%s', buf=%x, argv[%d]=%x\n", buf, argc, argv[argc], (unsigned int)buf, argc, (unsigned int)argv[argc]);
#endif
    buf = adjust_buf_to_ordinary(buf);
#if DEBUG
    fprintf(stderr, ">> DEBUG. do_bak_bakfile: called adjust_buf_to_ordinary: buf is '%s', argv[%d] is '%s', buf=%x, argv[%d]=%x\n", buf, argc, argv[argc], (unsigned int)buf, argc, (unsigned int)argv[argc]);
#endif
    buf = get_src_name(buf);
#if DEBUG
    fprintf(stderr, ">> DEBUG. do_bak_bakfile: called get_src_name: buf is '%s', argv[%d] is '%s', buf=%x, argv[%d]=%x\n", buf, argc, argv[argc], (unsigned int)buf, argc, (unsigned int)argv[argc]);
#endif
    buf = convert_src_name_to_path(buf);
#if DEBUG
    fprintf(stderr, ">> DEBUG. do_bak_bakfile: called convert_src_name_to_path: buf is '%s', argv[%d] is '%s'\n", buf, argc, argv[argc]);
#endif
    return buf;
}


static char* adjust_buf_to_ordinary(char* buf)
{
#if DEBUG
    fprintf(stderr, ">>> DEBUG. adjust_buf_to_ordinary: buf is '%s'\n", buf);
#endif
    /* 0923 */
    char* ap = buf + 1;
    char* bp;
    char* cp;
    char* dp;
    while(check_splash_dot_dot_splash(buf))
    {
#if DEBUG
        fprintf(stderr, ">>> DEBUG. adjust_buf_to_ordinary: buf is '%s'\n", buf);
        fprintf(stderr, "string have ../   getchar()\n");
#if DEBUGA
        getchar();
#endif
#endif
        while(*ap != '/') ap = ap + 1;
#if DEBUG
        fprintf(stderr, "ap is '%s'   getchar()\n", ap);
#if DEBUGA
        getchar();
#endif
#endif
        bp = ap - 1;cp = bp - 1;dp = cp - 1;
        if(dp == buf)
        {
            strcpy(buf, ap);
            ap  = buf + 1;
        }
        else
        {
            
            if((*bp == '.')&&(*cp == '.')&&(*dp == '/'))
            {
#if DEBUG
                fprintf(stderr, "we got a ../   getchar()\n");
#if DEBUGA
                getchar();
#endif
#endif
                dp = dp - 1;
                while(*dp != '/')	dp = dp - 1;
                if(dp != buf)
                {
                    *dp = '\0';
                    strcat(buf, ap);
                    ap = buf + 1;
                }
                else
                {
                    strcpy(buf, ap);
                    ap = buf + 1;
                }
#if DEBUG
                fprintf(stderr, "we got a ../ , buf is '%s'\n", buf);
#endif
            }
            else
            {
#if DEBUG
                fprintf(stderr, "we find a xxx/   getchar()\n");
#if DEBUGA
                getchar();
#endif
#endif
                ap = ap + 1;
            }
        }
    }
#if DEBUG
    fprintf(stderr, ">>> !. adjust_buf_to_ordinary complete.\n");
#endif
    return buf;
}

static int check_splash_dot_dot_splash(char* buf)
{
    unsigned char ret=0;
    char* ap = buf + strlen(buf) - 1;
    char* bp = ap - 1;
    char* cp = bp - 1;
    char* dp = cp - 1;
    while((dp != buf)&&((*ap != '/')||(*bp != '.')||(*cp != '.')||(*dp != '/')))
    {
        if(dp != buf)
        {
            dp = dp - 1;
            cp = cp - 1;
            bp = bp - 1;
            ap = ap - 1;
        }
    }
    if((*ap == '/')&&(*bp == '.')&&(*cp == '.')&&(*dp == '/')) ret = 1;
    return ret;
}

static char* convert_src_name_to_path(char* buf)
{
#if DEBUG
    fprintf(stderr, ">>> DEBUG. calling convert_src_name_to_path: buf is '%s'\n", buf);
#endif
    char* cp = buf;
    if(*cp != '.')
    {
        fprintf(stderr, "bak: '%s' is not a format .bak file, we do nothing yet..\n", buf);
        return NULL;
    }
    cp = buf + strlen(buf) - 5;
#if DEBUG
    fprintf(stderr, "cp is %s\n", cp);
#endif
    char* bp = cp;
    while(*bp != '.')//can remove '.' to anything?
    {
        bp = bp - 1;
    }
#if DEBUG
    fprintf(stderr, "bp is %s\n", bp);
#endif
    unsigned char length = cp - bp + 1;
    unsigned char j=0;
    //char* temp = 0;
    //temp = malloc(length + 1);
    //memset(temp, '\0', length + 1);
    
    // 03.26
    char temp[length];
    
    while(bp != (cp+1))
    {
        *(temp + j) = *bp;
        bp = bp + 1;
        j = j + 1;
    }
#if DEBUG
    fprintf(stderr, "temp is '%s', bp is'%s', cp is '%s'\n", temp, bp, cp);
#endif
    bp = cp - 2*length + 1;cp = cp - length;j = 0;
//    char* temp2 = 0;
//    temp2 = malloc(length + 1);
//    memset(temp2, '\0', length + 1);
   
    // 03.26
    char temp2[length];
    
    while(bp != (cp+1))
    {
        *(temp2 + j) = *bp;
        bp = bp + 1;
        j = j + 1;
    }
#if DEBUG
    fprintf(stderr, "temp is '%s',temp2 is '%s', bp is'%s', cp is '%s'\n", temp, temp2, bp, cp);
#endif
    if(strcmp(temp, temp2)) // a type
    {
#if DEBUG
        fprintf(stderr, "It's a no dot file: a type.\n");
        fprintf(stderr, "It's a dot file: '%s' type.\n", temp);
        fprintf(stderr, "now buf is '%s'\n", buf);
        fprintf(stderr, "now cp is '%s'\n", cp);
        fprintf(stderr, "now bp is '%s'\n", bp);
#endif
        bp = bp + length;
#if DEBUG
        fprintf(stderr, "!!. bp is '%s'\n", bp);
#endif
        *bp = '\0';
        cp = buf + strlen(buf) -1;
#if DEBUG
        fprintf(stderr, "!!. cp is '%s'\n", cp);
#endif
        while(cp != buf)
        {
            //if(*cp == '.') *cp = '/';//0927
            if(*cp == tip) *cp = '/';
            cp = cp - 1;
        }
        *cp = '/';
#if DEBUG
        fprintf(stderr, "> after change, cp is '%s'\n", cp);
        fprintf(stderr, "> after change, buf is '%s'\n", buf);
#endif
    }
    else// a.c type
    {
#if DEBUG
        fprintf(stderr, "It's a dot file: '%s' type.\n", temp);
        fprintf(stderr, "now buf is '%s'\n", buf);
        fprintf(stderr, "now cp is '%s'\n", cp);
        fprintf(stderr, "now bp is '%s'\n", bp);
#endif
        *bp = '\0';
        cp = buf + strlen(buf) -1;
        cp = cp -length;
        while(cp != buf)
        {
            //if(*cp == '.') *cp = '/';//0927
            if(*cp == tip) *cp = '/';
            cp = cp - 1;
        }
        *cp = '/';
#if DEBUG
        fprintf(stderr, "> after change, cp is '%s'\n", cp);
        fprintf(stderr, "> after change, buf is '%s'\n", buf);
#endif
    }
#if DEBUG
    fprintf(stderr, ">>> DEBUG. called convert_src_name_to_path: buf is '%s'\n", buf);
#endif
    return buf;
}

static char* get_src_name(char* buf)
{
#if DEBUG
    fprintf(stderr, ">>> DEBUG. calling get_src_name: buf is '%s'\n", buf);
#endif
    char* cp = buf + strlen(buf) -1;
    while(*cp != '/')
    {
        cp = cp - 1;
    }
    cp = cp + 1;
//    strcpy(buf, cp);
    // 03.26
    char buff[strlen(cp)];
    strcpy(buff, cp);
    strcpy(buf, buff);
    
#if DEBUG
    fprintf(stderr, ">>> DEBUG. called get_src_name: buf is '%s'\n", buf);
#endif
    return buf;
}

static char* do_bak_regfile(char* buf, char* tail, char** argv, int argc)
{
    //	char* path = 0;
    //	path = malloc(sizeof(buf));
    //	strcpy(path, buf);
    buf = strcat_buf_and_argv(buf, argv, argc);
#if DEBUG
    fprintf(stderr, ">> DEBUG. do_bak_regfile: called strcat_buf_and_argv: buf is '%s', argv[%d] is '%s'\n", buf, argc, argv[argc]);
#endif
    buf = adjust_buf_to_ordinary(buf);
#if DEBUG
    fprintf(stderr, ">> DEBUG. do_bak_regfile: called adjust_buf_to_ordinary: buf is '%s', argv[%d] is '%s'\n", buf, argc, argv[argc]);
#endif
    /* 0923 */
    char* path = 0;
    path = malloc(sizeof(buf));
    strcpy(path, buf);
    char* cp = path + strlen(path) - 1;
    while(*cp != '/') cp = cp - 1;
    *cp = '\0';
#if DEBUG
    fprintf(stderr, ">> DEBUG. do_bak_regfile: path is '%s'\n", path);
#endif
    buf = change_splash_to_dot_and_strcat_path_and_buf(path, buf, tail);
    return buf;
}

static char* change_splash_to_dot_and_strcat_path_and_buf(char* path,char* buf, char* tail)
{
#if DEBUG
    fprintf(stderr, ">>> DEBUG. change_splash_to_dot_and_strcat_path_and_buf: buf is '%s'\n", buf);
    fprintf(stderr, ">>> DEBUG. change_splash_to_dot_and_strcat_path_and_buf: tail is '%s'\n", tail);
    fprintf(stderr, ">>> DEBUG. change_splash_to_dot_and_strcat_path_and_buf: path is '%s'\n", path);
#endif
    char* cp = buf;
    *cp = '.';//change '/' to '.'
    while(*cp != '\0')
    {
        //if(*cp == '/') *cp = '.';//0927
        if(*cp == '/') *cp = tip;//0927
        cp = cp + 1;
    }
#ifdef BAK_HAVE_TAIL
    if(*tail == '.') strcat(cp, tail);
#endif
    strcat(path, "/");
    strcat(path, buf);
    strcat(path, ".bak");
#if DEBUG
    fprintf(stderr, ">>> NOTICE. change_splash_to_dot_and_strcat_path_and_buf: buf is '%s'\n", buf);
    fprintf(stderr, ">>> NOTICE. change_splash_to_dot_and_strcat_path_and_buf: tail is '%s'\n", tail);
    fprintf(stderr, ">>> NOTICE. change_splash_to_dot_and_strcat_path_and_buf: path is '%s'\n", path);
#endif
    return path;
}


static char* get_file_type(char* args)
{
    char* cp;
    cp = args + strlen(args) - 1;
    while((cp != args)&&(*cp != '.'))
    {
        if(cp != args) cp = cp - 1;
    }
    return cp;
}

static char* strcat_buf_and_argv(char* buf,char** argv,int argc)
{
#if DEBUG
    fprintf(stderr, ">>> DEBUG. strcat_buf_and_argv: buf is '%s', argv[%d] is '%s'\n", buf, argc, argv[argc]);
#endif
    strcat(buf, "/");
    if((argv[argc][0] == '.')&&(argv[argc][1] == '.')&&(argv[argc][2] == '/'))
    {
        strcat(buf, argv[argc]);
    }
    else if(argv[argc][0] == '/')
    {
        //		return argv[argc];//error: argv[argc] will equal with buf!
        memset(buf, '\0', strlen(buf));
        strcpy(buf, argv[argc]);
    }
    else if((argv[argc][0] == '.')&&(argv[argc][1] == '/'))
    {
        char* cp = *(argv+argc);
        cp = cp + 2;
        strcat(buf, cp);
    }
    else
    {
        strcat(buf, argv[argc]);
    }
    return buf;
}


static int inline	show_help_notes()// --help -h
{
    fprintf(stderr, "Usage: bak [option] source...\n");
    fprintf(stderr, "    -b backup src file before being covered.\n");
    fprintf(stderr, "    -R recursive directory.\n");
    fprintf(stderr, "    -h, --help show this notice and quit.\n");
    fprintf(stderr, "    --version show version informations and quit.\n\n");
    fprintf(stderr, "Usage 1: \"bak /home/guest/project/main.cpp\" or \"cd /home/guest/project;bak main.cpp\"\n");
#ifdef BAK_HAVE_TAIL
    fprintf(stderr, "Create .home%cguest%cproject%cmain.cpp.cpp.bak In directory: /home/guest/project/\n", tip, tip, tip);
#else
    fprintf(stderr, "Create .home%cguest%cproject%cmain.cpp.bak In directory: /home/guest/project/\n", tip, tip, tip);
#endif
#ifdef BAK_HAVE_TAIL
    fprintf(stderr, "Usage 2: \"bak ~anywhere/.home%cguest%cproject%cmain.cpp.cpp.bak\"\n", tip, tip, tip);
    fprintf(stderr, "Copy .home%cguest%cproject%cmain.cpp.cpp.bak to /home/guest/project/main.cpp\n\n", tip, tip, tip);
#else
    fprintf(stderr, "Usage 2: \"bak ~anywhere/.home%cguest%cproject%cmain.cpp.bak\"\n", tip, tip, tip);
    fprintf(stderr, "Copy .home%cguest%cproject%cmain.cpp.bak to /home/guest/project/main.cpp\n\n", tip, tip, tip);
#endif
    fprintf(stderr, "Report bak bugs to liuchonghui@139.com\n");
    return 0;
}

static int inline show_version_notes()// -v --version
{
    fprintf(stderr, "bak %s\n", version);
    return 0;
}

static int inline invalid_option_warning(char** argv)// invalid option -*
{
    fprintf(stderr, "bak: invalid option %s\nType \"bak --help\" or \"bak -h\" for more informations.\n", argv[1]);
    return 0;
}

#if DEBUG
static int DEBUG_argv_list(int argc, char** argv)
{
    unsigned char i=0;
    unsigned char j=0;
    if(argc > 1) while(argc--) fprintf(stderr, "> DEBUG. argv[%d] = '%s'\n", i++, argv[j++]);
    return 0;
}
#endif

static int require_input_argv(int argc)
{
    unsigned char ret=0;
    if(argc < 2)
    {
        fprintf(stderr, "bak: require input file(s)\n");
        ret = 1;
    }
   	return ret;
}

static int do_copy(char** argv,int argc, char* buf)
{
#if DEBUG
    fprintf(stderr, ">> NOTICE. do_copy: argv[%d] is '%s'\n", argc, argv[argc]);
    fprintf(stderr, ">> NOTICE. do_copy: buf is '%s'\n", buf);
#endif
    if(buf == NULL)
    {
#if DEBUG
        fprintf(stderr, ">> WARNING. do_copy: buf is NULL, maybe is a *.bak file.\n");
#endif
        return 0;
    }
    struct utimbuf times;
    struct stat statbuf;
    struct stat statbuf2;
    if(stat(argv[argc], &statbuf) < 0)
    {
#if DEBUG
        fprintf(stderr, ">> ERR. do_copy: argv[argc] is '%s',check no such file.\n", argv[argc]);
#endif
        perror(argv[argc]);
        return -1;
    }
    if(need_backup)//target file exist, need backup to filename.bak before .*.bak cover it
    {
        if(stat(buf, &statbuf) == 0)
        {
#if DEBUG
            fprintf(stderr, ">> WARNING. do_copy: '%s' already exist.\n", buf);
#endif
            if(check_any_bak_file(buf))
            {
                int len = strlen(buf) + 4;
                char buff[len];
                strcpy(buff, buf);
                strcat(buff, ".bak");
                do_copy(&buf, 0, buff);
            }
        }
    }
    int rfd = open(argv[argc], 0);
    int rcc, wcc;
    if(rfd < 0) 
    {
#if DEBUG
        fprintf(stderr, ">> ERR. do_copy: open(argv[argc] is '%s', 0) failed.\n", argv[argc]);
#endif
        perror(argv[argc]);
        return -1;
    }
    if(!need_backup) unlink(buf);///20101028
    int wfd = open(buf, O_WRONLY|O_CREAT|O_TRUNC, statbuf2.st_mode);
    if (wfd < 0)
    {
#if DEBUG
        fprintf(stderr, ">> ERR. do_copy: open(buf is '%s', 0) failed.\n", buf);
#endif
        //perror(buf);//导致堆栈出错
        fprintf(stderr, "%s: No such file or directory\n", buf);
        close(rfd);
        return -1;
    }
    int len = 8192-16;
    char buffer[len];
    while ((rcc = read(rfd, buffer, len)) > 0) 
    {
        char* bp = buffer;
        while(rcc > 0) 
        {
            wcc = write(wfd, bp, rcc);
            if (wcc < 0) 
            {
#if DEBUG
                fprintf(stderr, ">> ERR. do_copy: write(wfd, bp, rcc) failed.\n");
#endif
                perror(buf);
                return -1;
            }
            bp += wcc;
            rcc -= wcc;
        }
    }
    if(rcc < 0) 
    {
        perror(buf);
        return -1;
    }
    close(rfd);
    if(close(wfd) < 0) 
    {
#if DEBUG
        fprintf(stderr, ">> ERR. do_copy: close(wfd) failed.\n");
#endif
        perror(buf);
        return -1;
    }
    if(1) 
    {
        (void)chmod(buf, statbuf.st_mode);
        (void)chown(buf, statbuf.st_uid, statbuf.st_gid);
        times.actime = statbuf.st_atime;
        times.modtime = statbuf.st_mtime;
        if(is_copy_regfile_to_baktype)
        {
            is_copy_regfile_to_baktype = 0;
            (void)utime(buf, &times);///20101029 "timestamp same with orig-file"
        }
    }
#if DEBUG
    fprintf(stderr, ">> WARNING. do_copy: done.\n");
#endif
    return 0;
}

