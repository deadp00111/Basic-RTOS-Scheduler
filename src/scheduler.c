

#include "scheduler.h"

static task_control_block_t task_table[MAX_TASKS];
static uint8_t  task_count = 0;
static volatile uint32_t system_tick_ms = 0; // increments every 1ms 

void scheduler_init(void)
{
    for (uint8_t i = 0; i < MAX_TASKS; i++) {
        task_table[i].func      = NULL;
        task_table[i].state     = TASK_SUSPENDED;
        task_table[i].period_ms = 0;
        task_table[i].delay_ms  = 0;
        task_table[i].name      = "unused";
    }
    task_count    = 0;
    system_tick_ms = 0;
}

int scheduler_add_task(task_func_t func, uint32_t period_ms, const char *name)
{
    if (task_count >= MAX_TASKS || func == NULL) {
        return -1;
    }

    task_table[task_count].func      = func;
    task_table[task_count].state     = TASK_BLOCKED;
    task_table[task_count].period_ms = period_ms;
    task_table[task_count].delay_ms  = period_ms; // run after one period 
    task_table[task_count].name      = name;

    return task_count++;
}


void scheduler_tick_handler(void)
{
    system_tick_ms++;

    for (uint8_t i = 0; i < task_count; i++) {
        if (task_table[i].state == TASK_BLOCKED) {
            if (task_table[i].delay_ms > 0) {
                task_table[i].delay_ms--;
            }
            if (task_table[i].delay_ms == 0) {
                task_table[i].state = TASK_READY;
            }
        }
    }
}


void scheduler_run(void)
{
    for (uint8_t i = 0; i < task_count; i++) {
        if (task_table[i].state == TASK_READY) {
            task_table[i].state = TASK_RUNNING;

            task_table[i].func();   // function-pointer

            
            task_table[i].delay_ms = task_table[i].period_ms;
            task_table[i].state    = TASK_BLOCKED;
        }
    }
}

uint32_t scheduler_get_tick(void)
{
    return system_tick_ms;
}