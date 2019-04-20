//
// Created by jacka on 2019/4/19.
//

#ifndef MYSHELL_SHELL_H
#define MYSHELL_SHELL_H


#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<dirent.h>
#include<pwd.h>
#include<wait.h>
#include<sys/types.h>
#include <readline/readline.h>
#include <readline/history.h>


char lastdir[100];
char command[BUFSIZ];
char argv[100][100];
char **argvtmp1;
char **argvtmp2;
char argv_redirect[100];
int argc;
int BUILTIN_COMMAND = 0;  //标志是否是内部命令
int PIPE_COMMAND = 0;     //标志是否是管道命令
int REDIRECT_COMMAND = 0;  //标志是否是重定向命令

//设置提示
void set_prompt(char *prompt);

//分析命令
void analyse_command();

// 内部命令
void builtin_command();

// 外部命令
void do_command();

//一些辅助函数
void welcome();

void help();

void initial();

void init_lastdir();

void history_setup();

void history_finish();

void bye();


//set the prompt
void set_prompt(char *prompt) {
    char hostname[100];
    char cwd[100];
    char super = '#';
    char delims[] = "/";
    struct passwd *pwp;

    // gethostname 获取主机名
    // getcwd 获取当前目录
    // getpwuid 获取用户id信息
    // getuid 获取用户id
    if (gethostname(hostname, sizeof(hostname)) == -1) {
        //获取主机失败
        strcpy(hostname, "unknown");
    }

    pwp = getpwuid(getuid());

    if (!(getcwd(cwd, sizeof(cwd)))) {
        //获取当前目录失败
        strcpy(cwd, "unknown");
    }

    // 当前路径在home,则设置提示符为~
    char cwdcopy[100];
    strcpy(cwdcopy, cwd);
    char *first = strtok(cwdcopy, delims);
    if (strcmp(first, "home") == 0) {
        strcpy(cwd, "~");
    }


//    printf("%s \n", cwd);
    if (getuid() != 0)//判断用户是root还是普通用户
        super = '$';

    //使用字符串格式化函数sprintf来将命令提示符中的几部分（目录，主机名）合并到一个字符串中，
    //readline.h头文件中readline函数读入一行字符串，它包含一个char *prompt参数，传入的prompt
    //将作为命令提示符来处理，要使用这个库，链接的时候要链接动态库ncurses
    sprintf(prompt, "%s@%s:%s%c", pwp->pw_name, hostname, cwd, super);

}

//分析用户输入的命令
void analyse_command() {
    char *p;
    char delims[] = " ";
    argc = 1;

    // 由readline函数获得输入的字符串，使用strtok函数进行分割
    // 根据" "来分割字符串，将分割后的字符串保存到argv中
    // argv[0]保存命令， argv[1]开始保存参数
    strcpy(argv[0], strtok(command, delims));
    p = strtok(NULL, delims);
    int k = 1;
    while (p) {
        strcpy(argv[k++], p);
        argc++;
        p = strtok(NULL, delims);
    }

    //判断是否为内部命令
    if (!(strcmp(argv[0], "help")) || !(strcmp(argv[0], "cd"))) {
        BUILTIN_COMMAND = 1;
    }

    //判断是否是管道命令
    int pipe_location = 0;
    for (int j = 0; j < argc; j++) {
        //如果遇到'|'则认为是管道命令命令
        if (strcmp(argv[j], "|") == 0) {
            PIPE_COMMAND = 1;
            pipe_location = j;
            break;
        }
    }

    //判断是否是重定向命令
    int redirect_location = 0;
    for (int j = 0; j < argc; j++) {
        //扫描到'>'
        //输出重定向
        if (strcmp(argv[j], ">") == 0) {
            REDIRECT_COMMAND = 1;
            //标记重定向开始的位置
            redirect_location = j;
            break;
        }
        //输入重定向
        if (strcmp(argv[j], "<") == 0) {
            REDIRECT_COMMAND = 2;
            redirect_location = j;
            break;
        }
    }

    // 管道命令不能当成一条命令来执行，所以在命令分析阶段通过'|'将命令分割为两部分
    if (PIPE_COMMAND) {
        //command 1
        argvtmp1 = malloc(sizeof(char *) * pipe_location + 1);
        for (int i = 0; i < pipe_location + 1; i++) {
            argvtmp1[i] = malloc(sizeof(char) * 100);
            if (i <= pipe_location)
                strcpy(argvtmp1[i], argv[i]);
        }
        argvtmp1[pipe_location] = NULL;

        //command 2
        argvtmp2 = malloc(sizeof(char *) * (argc - pipe_location));
        for (int j = 0; j < argc - pipe_location; j++) {
            argvtmp2[j] = malloc(sizeof(char) * 100);
            if (j <= pipe_location)
                strcpy(argvtmp2[j], argv[pipe_location + 1 + j]);
        }
        argvtmp2[argc - pipe_location - 1] = NULL;

    } else if (REDIRECT_COMMAND) {  //重定向命令分析，将命令分解为两部分
        strcpy(argv_redirect, argv[redirect_location + 1]);
        argvtmp1 = malloc(sizeof(char *) * redirect_location + 1);

        for (int i = 0; i < redirect_location + 1; i++) {
            argvtmp1[i] = malloc(sizeof(char) * 100);
            if (i < redirect_location)
                strcpy(argvtmp1[i], argv[i]);
        }
        argvtmp1[redirect_location] = NULL;
    } else {    //使得命令满足execvp函数的参数要求
        argvtmp1 = malloc(sizeof(char *) * argc + 1);
        for (int i = 0; i < argc + 1; i++) {
            argvtmp1[i] = malloc(sizeof(char) * 100);
            if (i < argc)
                strcpy(argvtmp1[i], argv[i]);
        }
        argvtmp1[argc] = NULL;
    }


}

void builtin_command() {
    struct passwd *pwp;
    // 内部命令
    // exit在输入的时候已经处理
    if (strcmp(argv[0], "help") == 0) {  //quit
        help();
    } else if (strcmp(argv[0], "cd") == 0) {  //cd
        char cd_path[100];
        if ((strlen(argv[1])) == 0) {
            pwp = getpwuid(getuid());
            sprintf(cd_path, "/home/%s", pwp->pw_name);
            strcpy(argv[1], cd_path);
            argc++;
        } else if ((strcmp(argv[1], "~") == 0)) {
            pwp = getpwuid(getuid());
            sprintf(cd_path, "/home/%s", pwp->pw_name);
            strcpy(argv[1], cd_path);
        }

        if ((chdir(argv[1])) < 0) {
            printf("cd failed in builtin_command()\n");
        }
    }
}

void do_command() {
    //执行命令
    if (PIPE_COMMAND) {  //管道命令

        int fd[2];
        int status;
        int res = pipe(fd);

        if (res == -1)
            printf("pipe failed in do_command()\n");
        pid_t pid1 = fork();
        if (pid1 == -1) {
            printf("fork failed in do_command()\n");
        } else if (pid1 == 0) {
            dup2(fd[1], 1);//复制标准输出
            close(fd[0]);//关闭读边
            if (execvp(argvtmp1[0], argvtmp1) < 0) {
                printf("%s:command not found\n", argvtmp1[0]);
            }
        } else {
            waitpid(pid1, &status, 0);
            pid_t pid2 = fork();
            if (pid2 == -1) {
                printf("fork failed in do_command()\n");
            } else if (pid2 == 0) {
                close(fd[1]);//关闭写边
                dup2(fd[0], 0);//复制标准输入
                if (execvp(argvtmp2[0], argvtmp2) < 0) {
                    printf("%s:command not found\n", argvtmp2[0]);
                }
            } else {
                close(fd[0]);
                close(fd[1]);
                waitpid(pid2, &status, 0);
            }
        }
    } else if (REDIRECT_COMMAND) {  //重定向命令
        //重定向命令的关键函数为freopen
        //FILE *freopen (const char *__restrict __filename,
        //		      const char *__restrict __modes,
        //		      FILE *__restrict __stream)

        pid_t pid = fork();
        if (pid == -1) {
            printf("fork failed in do_command()\n");
        } else if (pid == 0) {
            int redirect_flag = 0;
            FILE *fstream;
            // 输出重定向，以写方式打开文件
            if (REDIRECT_COMMAND == 1) {
                fstream = fopen(argv_redirect, "w+");
                freopen(argv_redirect, "w", stdout);
                if (execvp(argvtmp1[0], argvtmp1) < 0) {
                    redirect_flag = 1;//execvp调用失败
                }
                fclose(stdout);
                fclose(fstream);
            } else {  //输入重定向，以读方式打开文件
                fstream = fopen(argv_redirect, "r+");
                freopen(argv_redirect, "r", stdin);
                if (execvp(argvtmp1[0], argvtmp1) < 0) {
                    redirect_flag = 1;//execvp调用失败
                }
                fclose(stdout);
                fclose(fstream);
            }

            if (redirect_flag) {
                printf("%s:command not found\n", argvtmp1[0]);
            }

        } else {
            int pidReturn = wait(NULL);
        }
    } else {  //普通外部命令
        pid_t pid = fork();
        if (pid == -1) {
            printf("fork failed in do_command()\n");
        } else if (pid == 0) {
            if (execvp(argvtmp1[0], argvtmp1) < 0) {
                printf("%s:command not found\n", argvtmp1[0]);
            }
        } else {
            int pidReturn = wait(NULL);
        }
    }

    free(argvtmp1);  //释放参数列表
    free(argvtmp2);
}

void welcome() {
    char message[50] = "Welcome to jack's shell!";
    printf("%s \n", message);
    printf("                   _ooOoo_\n"
           "                  o8888888o\n"
           "                  88\" . \"88\n"
           "                  (| -_- |)\n"
           "                  O\\  =  /O\n"
           "               ____/`---'\\____\n"
           "             .'  \\\\|     |//  `.\n"
           "            /  \\\\|||  :  |||//  \\\n"
           "           /  _||||| -:- |||||-  \\\n"
           "           |   | \\\\\\  -  /// |   |\n"
           "           | \\_|  ''\\---/''  |   |\n"
           "           \\  .-\\__  `-`  ___/-. /\n"
           "         ___`. .'  /--.--\\  `. . __\n"
           "      .\"\" '<  `.___\\_<|>_/___.'  >'\"\".\n"
           "     | | :  `- \\`.;`\\ _ /`;.`/ - ` : | |\n"
           "     \\  \\ `-.   \\_ __\\ /__ _/   .-` /  /\n"
           "======`-.____`-.___\\_____/___.-`____.-'======\n"
           "                   `=---='\n");
}

void help() {
    printf("jack's shell, version 0.1, release 1. \n"
           "These shell commands are defined internally.  Type `help' to see this list.\n"
           "Type `help name' to find out more about the function `name'.\n"
           "Use `info bash' to find out more about the shell in general.\n"
           "Use `man -k' or `info' to find out more about commands not in this list.\n");
}


void bye() {
    char message[50] = "bye jack's shell!";
    printf(
            "%s\n"
            "\t\t\\\n"
            "\t\t \\   \\_\\_    _/_/\n"
            "\t\t  \\      \\__/\n"
            "\t\t   \\     (oo)\\_______\n"
            "\t\t    \\    (__)\\       )\\/\\\n"
            "\t\t             ||----w |\n"
            "\t\t             ||     ||\n", message);
}

void initial() {   //初始化参数
    for (int i = 0; i < argc; i++) {
        strcpy(argv[i], "\0");
    }
    argc = 0;
    BUILTIN_COMMAND = 0;
    PIPE_COMMAND = 0;
    REDIRECT_COMMAND = 0;
}

void init_lastdir() {
    getcwd(lastdir, sizeof(lastdir));
}

void history_setup() {  //设置历史信息，只能查看50条命令
    using_history();
    stifle_history(50);
    read_history("/tmp/msh_history");
}

void history_finish() {  //退出时对历史命令清空
    append_history(history_length, "/tmp/msh_history");
    history_truncate_file("/tmp/msh_history", history_max_entries);
}


#endif //MYSHELL_SHELL_H
