#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <ctype.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>
#include "hop.h"
#include "reveal.h"
#include "proclore.h"
#include "seek.h"
#include "echo.h"
#include "log.h"

int main(int argc, char const *argv[])
{
    char user[6000];
    char lapname[6000];
    char currentdir[6000];
    char prevdir[6000];
    char home[6000];
    char log[LOG_SIZE][6000];
    int log_count = 0;

    int last_cmd_time = 0;

    load(log, &log_count);
    strncpy(user, getenv("USER"), sizeof(user) - 1);
    gethostname(lapname, sizeof(lapname) - 1);
    getcwd(home, sizeof(home));

    while (1)
    {
        
        getcwd(currentdir, sizeof(currentdir) - 1);

        if (strncmp(currentdir, home, strlen(home)) == 0)
        {
            if (last_cmd_time > 2)
            {
                printf("<%s@%s:~%s %s : %ds> ", user, lapname, currentdir + strlen(home), last_cmd, last_cmd_time);
            }
            else
            {
                printf("<%s@%s:~%s> ", user, lapname, currentdir + strlen(home));
            }
        }
        else
        {
            if (last_cmd_time > 2)
            {
                printf("<%s@%s:%s %s : %ds> ", user, lapname, currentdir, last_cmd, last_cmd_time);
            }
            else
            {
                printf("<%s@%s:%s> ", user, lapname, currentdir);
            }
        }

        last_cmd[0] = '\0';
        last_cmd_time = 0;

        char cmd[6000];
        fgets(cmd, sizeof(cmd), stdin);
        char *newline = strchr(cmd, '\n');
        if (newline)
        {
            *newline = '\0';
        }

        char cmod[6000];
        strcpy(cmod, cmd);

        char *save1;
        char *t2 = strtok_r(cmd, ";", &save1);
        int flag = 1;

        while (t2 != NULL)
        {
            getcwd(currentdir, sizeof(currentdir) - 1);
            while (*t2 == ' ')
                t2++;
            if (strlen(t2) == 0)
            {
                t2 = strtok_r(NULL, ";", &save1);
                continue;
            }

            char *bg_command = NULL;
            char *ampersand = strchr(t2, '&');
            if (ampersand != NULL)
            {
                *ampersand = '\0';
                bg_command = t2;
                while (*bg_command == ' ')
                    bg_command++;
                if (strlen(bg_command) > 0)
                {
                    pid_t pid = fork();
                    if (pid == 0)
                    {
                         setpgid(0, 0);
                         
                        pid_t pid2 = fork();
                        if (pid2 == 0)
                        {
                            printf("[%d]\n",getpid());
                            char *args[100];
                            char *arg = strtok(bg_command, " ");
                            int i = 0;
                            while (arg != NULL)
                            {
                                args[i++] = arg;
                                arg = strtok(NULL, " ");
                            }
                            args[i] = NULL;
                            if (execvp(args[0], args) < 0)
                            {
                                printf("background process ended abnormally with PID [%d]\n", getpid());
                                exit(EXIT_FAILURE);
                            }
                            exit(1);
                        }
                        else
                        {
                            wait(NULL);
                            printf("PID of the terminated child process: %d\n", pid2);
                            exit(EXIT_SUCCESS);
                        }
                    }
                    else
                    {

                        usleep(1000);
                    }
                }
                t2 = ampersand + 1;
                while (*t2 == ' ')
                    t2++;
                continue;
            }

            char *fg_command = t2;
            if (fg_command && strlen(fg_command) > 0)
            {
                time_t start_time = time(NULL);

                if (strncmp(fg_command, "hop", 3) == 0)
                {
                    char *x = fg_command + 3;
                    while (*x == ' ')
                        x++;
                    hopcmd(x, home, prevdir);
                    strncpy(prevdir, currentdir, sizeof(prevdir));
                }
                else if (strncmp(fg_command, "echo", 4) == 0)
                {
                    char *echo_cmd = strstr(fg_command, "echo");
                    if (echo_cmd)
                    {
                        ecko(echo_cmd + 4);
                    }
                }
                else if (strncmp(fg_command, "reveal", 6) == 0)
                {
                    char *x1 = fg_command + 6;
                    char flags[10] = "";
                    char *p = ".";
                    int ind = 0;
                    while (*x1 == ' ')
                        x1++;
                    while (*x1)
                    {
                        if (*x1 == '-' && (*(x1 + 1) == 'a' || *(x1 + 1) == 'l'))
                        {
                            x1++;
                            while (*x1 && *x1 != ' ')
                                flags[ind++] = *x1++;
                        }
                        else
                        {
                            p = x1;
                            break;
                        }
                        while (*x1 == ' ')
                            x1++;
                    }
                    flags[ind] = '\0';
                    reveal(flags, p, home, prevdir);
                }
                else if (strncmp(fg_command, "seek", 4) == 0)
                {
                    char *args[100];
                    int arg_count = 0;
                    char *arg = strtok(fg_command, " ");
                    while (arg != NULL)
                    {
                        args[arg_count++] = arg;
                        arg = strtok(NULL, " ");
                    }
                    seek(arg_count, (char **)args, home, currentdir);
                }
                else if (strncmp(fg_command, "proclore", 8) == 0)
                {
                    char *pid_str = fg_command + 8;
                    while (*pid_str == ' ')
                        pid_str++;
                    int pid = (*pid_str == '\0') ? getpid() : atoi(pid_str);
                    proclore(pid);
                }

                else if (strncmp(fg_command, "exit", 4) == 0)
                {
                    exit(0);
                }
                else if (strncmp(fg_command, "log purge", 9) == 0)
                {
                    flag = 0;
                    purge_log(log, &log_count, currentdir, home);
                }
                else if (strncmp(fg_command, "log execute", 11) == 0)
                {
                    flag = 0;
                    int index = atoi(fg_command + 12);
                    last_cmd_time=logexec(log, log_count, index, home, prevdir, currentdir);
                }
                else if (strncmp(fg_command, "log", 3) == 0)
                {
                    flag = 0;
                    printentries(log, log_count);
                }
                else
                {
                    pid_t pid = fork();
                    if (pid == 0)
                    {
                        char *args[100];
                        char *arg = strtok(fg_command, " ");
                        int i = 0;
                        while (arg != NULL)
                        {
                            args[i++] = arg;
                            arg = strtok(NULL, " ");
                        }
                        args[i] = NULL;
                        if (execvp(args[0], args) < 0)
                        {
                            printf("ERROR: Command '%s' failed to execute\n", fg_command);
                        }

                        exit(0);
                    }
                    else
                    {
                        int status;
                        waitpid(pid, &status, 0);
                        time_t end_time = time(NULL);

                        int time_taken = (int)(end_time - start_time);
                        last_cmd_time = time_taken;
                        if (time_taken > 2)
                        {
                            strncpy(last_cmd, fg_command, sizeof(last_cmd) - 1);
                        }
                    }
                }
            }

            t2 = strtok_r(NULL, ";", &save1);
        }
        if (flag == 1)
        {
            addentries(log, &log_count, cmod, currentdir, home);
        }
    }
    return 0;
}
