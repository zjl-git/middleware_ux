#include "ux_ports.h"

void *ux_ports_malloc(uint32_t num)
{
    return NULL;
}

void ux_ports_free(void *memory)
{

}

void ux_ports_synchronized(uint32_t *mask)
{

}

void ux_ports_synchronized_end(uint32_t mask)
{

}

void *ux_ports_create_mute(void)
{

}

void *ux_ports_create_semaphore(void)
{

}

uint32_t ux_ports_semaphore_give(void *semaphore, uint8_t from_isr)
{
    return 0;
}

uint32_t ux_ports_semaphore_take(void *semaphore, uint32_t block_time)
{
    return 0;
}


uint32_t ux_ports_get_currents_ms(void)
{
    return 0;
}

void ux_ports_create_task(ux_ports_task_handler task_func, char *task_name, 
                          uint16_t task_depth, void *task_parameters, uint32_t task_priority)
{

}
