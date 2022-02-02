#include <stdio.h>
#include <time.h>

int main()
{
    char input;
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    printf("Welcome to the alarm clock! The time now is: %d-%02d-%02d %02d:%02d:%02d \n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    printf("Please enter 's'(schedule), 'l'(list), 'c'(cancel), 'x'(exit) ");
    scanf("%c", &input);
    printf("You entered %c \n", input);
    return 0;
}