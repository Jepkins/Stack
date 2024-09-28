#include <stdio.h>
#include <io.h>
#include <time.h>
#include "stack.h"


typedef int elm_t;

void set_stderr (const char* file_name);

class timer_cl
{
    private:
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
    stack_ctor(&stk, sizeof(elm_t), 3);

    const size_t n_elms = 1e7;
    elm_t x = -5;

    for (size_t i = 0; i < n_elms; i++, x++)
    {
        stack_push(&stk, &x);
    }

    for (size_t j = 0; j < n_elms; j++)
    {
        stack_pop(&stk, &x);
    }
    timer.end();

    _STACK_ASSERT_(&stk)
    stack_dump(&stk, _POS_);
    stack_dtor(&stk);

    return 0;
}

void set_stderr (const char* file_name)
{
    FILE* err_file = fopen(file_name, "w");
    dup2(fileno(err_file), STDERR_FILENO);
    fclose(err_file);
}
