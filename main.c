#include<stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "shell.h"


int main() {
    char prompt[BUFSIZ];
    char *line;
    welcome();
    init_lastdir();
    history_setup();
    while (1) {
        set_prompt(prompt);
        line = readline(prompt);
        //空命令
        if (strlen(line) == 0)
            continue;
        //exit
        if (strcmp(line, "exit") == 0) {
            break;
        }

        if (*line)
            add_history(line);
        strcpy(command, line);
        analyse_command();  //分析命令
        if (BUILTIN_COMMAND) {  //内部命令
            builtin_command();
        } else {    //其他命令
            do_command();
        }

        initial();
    }
    history_finish();
    bye();  //退出提示

    return 0;
}
