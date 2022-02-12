#define _XOPEN_SOURCE
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>

struct Alarmclock
{
    pid_t pid;
    time_t alarm_time;
    int active;
};

int MAX_ALARMS = 60;

int main()
{
    struct Alarmclock alarms[MAX_ALARMS]; // Alarm storage
    int head = 0;   // Index for the array
    char op;    // Stores users requested operation for each loop
    time_t ct;  // Current time

    time(&ct);
    printf("Welcome to the alarm clock! It is currently %s", ctime(&ct));

    while (1)
    {
        printf("Please enter \"s\" (schedule), \"l\" (list), \"c\" (cancel), \"x\" (exit)\n> ");

        scanf(" %c", &op);
        
        // Remove inactive alarms (zombies)
        int status;
        for (int i = 0; i < head; i++) 
        {
            waitpid(alarms[i].pid, &status, WNOHANG);
            if (!status)
            {
                alarms[i].active = 0;
            }
        }
        
        switch (op)
        {
        case 's':
        {
            if (head >= MAX_ALARMS)
            {
                printf("Maximum capacity reached!\n");
                break;
            }

            struct Alarmclock alarm;
            char time_str[19];
            struct tm time_tm;

            printf("Schedule alarm at which date and time?\n> ");
            scanf(" %18c", time_str);

            strptime(time_str, "%Y-%m-%d %H:%M:%S", &time_tm);
            time_tm.tm_isdst = -1;

            alarm.alarm_time = mktime(&time_tm);
            alarm.active = 1;
            alarm.pid = fork();

            if (alarm.pid > 0)
            {
                alarms[head] = alarm;
                head++;
            }
            else if (alarm.pid == 0)
            {
                int sound_pid, sound_status;

                time(&ct);
                sleep(alarm.alarm_time - ct);
                if (sound_pid > 0) {
                    waitpid(sound_pid, &sound_status, 0);
                } else if (sound_pid == 0) {
                    execlp("mpg123","mpg123", "-q", "pling.mp3", NULL);
                    exit(0);
                } else {
                    printf("Fork failed!");
                }
                exit(0);
            }
            else
            {
                printf("Fork failed!\n");
            }
            break;
        }
        case 'l':
        {
            int count = 0;
            for (int i = 0; i < head; i++)
            {
                if (alarms[i].active) 
                {
                    count++;
                    printf("Alarm %d: %s", count, ctime(&(alarms[i].alarm_time)));
                }
            }
            break;
        }
        case 'c':
        {
            int alarm_no;

            printf("Cancel which alarm?\n> ");
            scanf("%d", &alarm_no);

            if (alarm_no > 0 && alarm_no < head + 1)
            {
                kill(alarms[alarm_no - 1].pid, SIGKILL);
                alarms[alarm_no - 1].active = 0;
            }
            else
            {
                printf("Invalid alarm number\n");
            }
            break;
        }
        case 'x':
        {
            printf("Goodbye!\n");
            exit(0);
        }
        default:
        {

            printf("Invalid operation %c\n", op);
        }
        }
        printf("--------------------------------\n");
    }
    return 0;
}
