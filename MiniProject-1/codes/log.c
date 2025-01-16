#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include "log.h"
#include "hop.h"
#include "reveal.h"
#include "proclore.h"
#include "seek.h"
#include "echo.h"

char last_cmd[6000];

void load(char log[LOG_SIZE][6000], int *log_count)
{
    FILE *fp = fopen(LOG_FILE, "r");
    if (fp)
    {
        while (fgets(log[*log_count], 6000, fp) != NULL && *log_count < LOG_SIZE)
        {
            log[*log_count][strcspn(log[*log_count], "\n")] = '\0';
            (*log_count)++;
        }
        fclose(fp);
    }
    else
    {
        printf("Error in reading the file\n");
    }
}

void saveentry(char log[LOG_SIZE][6000], int log_count, char *currentdir, char *home)
{
    FILE *fp = fopen(LOG_FILE, "w");
    if (fp)
    {
        for (int i = 0; i < log_count; i++)
        {
            fprintf(fp, "%s\n", log[i]);
        }
        fclose(fp);
    }
}

void printentries(char log[LOG_SIZE][6000], int log_count)
{
    for (int i = 0; i < log_count; i++)
    {
        printf("%s\n", log[i]);
    }
}

void addentries(char log[LOG_SIZE][6000], int *log_count, const char *command, char *currentdir, char *home)
{

    if (*log_count > 0 && strcmp(log[*log_count - 1], command) == 0)
    {
        return;
    }

    if (*log_count == LOG_SIZE)
    {
        for (int i = 1; i < LOG_SIZE; i++)
        {
            strcpy(log[i - 1], log[i]);
        }
        strcpy(log[LOG_SIZE - 1], command);
    }
    else
    {
        strcpy(log[*log_count], command);
        (*log_count)++;
    }
    saveentry(log, *log_count, currentdir, home);
}

void purge_log(char log[LOG_SIZE][6000], int *log_count, char *currentdir, char *home)
{
    *log_count = 0;
    saveentry(log, *log_count, currentdir, home);
}

int logexec(char log[LOG_SIZE][6000], int log_count, int index, char *home, char *prevdir, char *currentdir)
{
    if (index < 1 || index > log_count)
    {
        printf("ERROR: Invalid log index\n");
        return 0;
    }
   
    char command[4096];
    strcpy(command, log[log_count - index]);
    char cmod[6000];
    strcpy(cmod,command);
    int last_cmd_time = 0;
    printf("Executing: %s\n", command);

    char *save1;
    char *t2 = __strtok_r(command, ";", &save1);
    char curr[6000];
    strcpy(curr,currentdir);
    while (t2 != NULL)
    {
        getcwd(curr, sizeof(curr) - 1);
        //  printf("a\n");
        while (*t2 == ' ')
        {
            t2++;
        }

        if (strlen(t2) == 0)
        {
            t2 = __strtok_r(NULL, ";", &save1);
            continue;
        }

        char *bg_command = NULL;
        char *fg_command = NULL;
        char *ampersand = strchr(t2, '&');

        while (ampersand != NULL)
        {
            *ampersand = '\0';
            bg_command = t2;

            while (*bg_command == ' ')
            {
                bg_command++;
            }

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
                                printf("ERROR: Command '%s' failed to execute\n", bg_command);
                            }
                            exit(1);
                        }
                        else
                        {
                            wait(NULL);
                            printf("background process ended with pid=%d\n",pid2);
                            exit(1);
                        }
                    }
                    else
                    {

                        usleep(1000);
                    }
            }

            t2 = ampersand + 1;
            while (*t2 == ' ')
            {
                t2++;
            }

            ampersand = strchr(t2, '&');
        }

        fg_command = t2;
        if (fg_command && strlen(fg_command) > 0)
        {
            time_t start_time = time(NULL);

            if (strncmp(fg_command, "hop", 3) == 0)
            {
                char *x = fg_command + 3;
                while (*x == ' ')
                    x++;
                hopcmd(x, home, prevdir);
                strncpy(prevdir, curr, sizeof(prevdir));
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
                seek(arg_count, (char **)args, home, curr);
            }
            else if (strncmp(fg_command, "proclore", 8) == 0)
            {
                char *pid_str = fg_command + 8;
                while (*pid_str == ' ')
                    pid_str++;
                int pid = (*pid_str == '\0') ? getpid() : atoi(pid_str);
                proclore(pid);
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

        t2 = __strtok_r(NULL, ";", &save1);
    }
    addentries(log, &log_count, cmod, currentdir, home);
    return last_cmd_time;
}
