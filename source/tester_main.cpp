#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include "stack.h"


typedef double elm_t;

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
    stack_ctor(&stk, sizeof(elm_t));

    const size_t n_elms = 1e4;
    elm_t x = -16;

    for (size_t i = 0; i < n_elms; i++, x+=16)
    {
        if (i == 102)
            memset(stk.data, 1, 100);
        stack_push(&stk, &x);
    }

    for (size_t j = 0; j < n_elms - 20; j++)
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
