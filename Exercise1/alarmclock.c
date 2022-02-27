#define _XOPEN_SOURCE
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>

// Structure of each alarmclock
struct Alarmclock
{
    pid_t pid;         // Keeps track of the childs process id
    time_t alarm_time; // When the alarm should ring
    int active;        // If the alarm is active (haven't rung yet)
};

int MAX_ALARMS = 60;

// Plays the ringing sound
void ring()
{
    // Executes mpg123 then terminates
    execlp("mpg123", "mpg123", "-q", "pling.mp3", NULL);
    printf("RING\n");
    exit(0);
}

// Sets up a new alarm at the given time
void set_alarm(time_t alarm_time, time_t *current_time)
{
    int sound_pid, sound_status; // Variables for the ring child process

    time(current_time);                // Update current time
    sleep(alarm_time - *current_time); // Wait for the alarm time

    sound_pid = fork(); // Create the ring child process
    if (sound_pid > 0)
    {                                         // If in parent
        waitpid(sound_pid, &sound_status, 0); // Block until ringing is done
    }
    else if (sound_pid == 0)
    {           // If in child
        ring(); // Make ring sound
    }
    else
    { // If something goes wrong
        printf("Fork failed!");
    }
    exit(0); // Terminate process
}

int main()
{
    struct Alarmclock alarms[MAX_ALARMS]; // Alarm storage
    int head = 0;                         // Index for the array
    char op;                              // Stores users requested operation for each loop
    time_t ct;                            // Current time

    time(&ct); // Update current time
    printf("Welcome to the alarm clock! It is currently %s", ctime(&ct));

    while (1)
    {
        printf("Please enter \"s\" (schedule), \"l\" (list), \"c\" (cancel), \"x\" (exit)\n> ");

        scanf(" %c", &op); // Get operation to perform from user

        // Remove inactive alarms (zombies)
        int status;
        for (int i = 0; i < head; i++)
        {
            if (alarms[i].active)
            {
                waitpid(alarms[i].pid, &status, WNOHANG); // Gets rid of zombies
                if (!status)
                {
                    alarms[i].active = 0; // Marks the alarm as inactive
                }
            }
        }

        switch (op)
        {
        case 's': // Schedule
        {
            // Check if there is still space for new alarms
            if (head >= MAX_ALARMS)
            {
                printf("Maximum capacity reached!\n");
                break;
            }

            struct Alarmclock alarm; // Stores the newly created alarm
            char time_str[19];       // User input variable
            struct tm time_tm;       // User input parsed to tm type

            printf("Schedule alarm at which date and time? (YYYY-MM-DD hh:mm:ss)\n> ");
            scanf(" %19c", time_str); // Get user input

            strptime(time_str, "%Y-%m-%d%t%T", &time_tm); // Converts string to tm type
            time_tm.tm_isdst = 0;                         // Ignore daylight saving time

            alarm.alarm_time = mktime(&time_tm); // Converts tm to time_t type
            alarm.active = 1;                    // Sets the new alarm as active

            alarm.pid = fork(); // Create child process for the alarm
            if (alarm.pid > 0)
            { // If in parent
                // Add the alarm to the array
                alarms[head] = alarm;
                head++;
            }
            else if (alarm.pid == 0)
            {                                     // If in child
                set_alarm(alarm.alarm_time, &ct); // Execute the alarm logic
            }
            else
            { // If something went wrong
                printf("Fork failed!\n");
            }
            break;
        }
        case 'l': // List
        {
            // List all active alarms
            for (int i = 0; i < head; i++)
            {
                if (alarms[i].active)
                {
                    printf("Alarm %d: %s", i + 1, ctime(&(alarms[i].alarm_time)));
                }
            }
            break;
        }
        case 'c': // Cancel
        {
            int alarm_no; // User input variable

            printf("Cancel which alarm?\n> ");
            scanf("%d", &alarm_no); // Get user input

            if (alarm_no > 0 && alarm_no < head + 1) // Check alarm number validity
            {
                // Kill the child process and mark the alarm as inactive
                kill(alarms[alarm_no - 1].pid, SIGKILL);
                alarms[alarm_no - 1].active = 0;
            }
            else
            {
                printf("Invalid alarm number\n");
            }
            break;
        }
        case 'x': // Exit program
        {
            printf("Goodbye!\n");
            exit(0); // Terminate alarmclock program
        }
        default: // Invalid operation
        {

            printf("Invalid operation %c\n", op);
        }
        }
        printf("--------------------------------\n");
    }
    return 0;
}
