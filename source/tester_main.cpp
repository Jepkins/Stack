#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include "stack.h"


typedef int elm_t;

void set_stderr (const char* file_name);

// !do not push/pop values with byte size not equal to stk->elm_width
int main()
{
    set_stderr("err_log.txt");

    stack_t stk = {};
    stack_ctor(&stk, sizeof(elm_t));

    

    stack_dtor(&stk);
    return 0;
}

void set_stderr (const char* file_name)
{
    FILE* err_file = fopen(file_name, "w");
    dup2(fileno(err_file), STDERR_FILENO);
    fclose(err_file);
}
