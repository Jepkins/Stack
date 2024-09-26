#include <stdio.h>
#include <io.h>
#include <time.h>
#include "stack.h"

typedef double elm_t;

void set_stderr (const char* file_name);

class timer_cl
{
    long int mark = 0;
    bool started = false;
    public:
        void start()
        {
            mark = clock();
            started = true;
        }
        void end()
        {
            if (started)
            {
                printf("TIME = %ld\n", clock() - mark);
                started = false;
            }
        }
};

int main()
{
    timer_cl timer;
    timer.start();

    set_stderr("err_log.txt");

    stack_t stk = {};
    stack_ctor(&stk, sizeof(elm_t), 10);

    const size_t n_elms = 1e5;
    elm_t x = 0;

    for (; x < n_elms; x++)
    {
        stack_push(&stk, &x);
        stack_assert(&stk, "main:!1"); // !1
    }

    for (size_t j = 0; j < n_elms; j++)
    {
        stack_pop(&stk, &x);
        stack_assert(&stk, "main:!2"); // !2
    }

    stack_assert(&stk, "main:!3"); // !3
    stack_dtor(&stk);

    timer.end();

    return 0;
}

void set_stderr (const char* file_name)
{
    FILE* err_file = fopen(file_name, "w");
    dup2(fileno(err_file), STDERR_FILENO);
    fclose(err_file);
}
