#ifndef __UX_MESSAGE_DELAY_QUEUE_H__
#define __UX_MESSAGE_DELAY_QUEUE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "ux_ports.h"
#include "ux_activity.h"
#include "ux_message_queue.h"

typedef struct _ux_delay_msg {
    ux_msg  basic_msg;
    uint32_t msg_act_time;
} ux_delay_msg;

typedef struct _ux_delay_msg_queue {
    ux_msg_queue parent;
    void (*push_delay_msg)(struct _ux_delay_msg_queue *msg_queue, ux_delay_msg *input_msg);
    ux_delay_msg *(*remove_timeout_msg)(struct _ux_delay_msg_queue *msg_queue);
    uint32_t (*get_next_timeout)(struct _ux_delay_msg_queue *msg_queue);
} ux_delay_msg_queue;

ux_delay_msg_queue *ux_message_delay_queue_new(void);

void ux_message_delay_queue_input_msg(ux_delay_msg *input_msg, uint8_t priority, uint32_t group, uint32_t id,
                                      void *data, uint32_t data_len, void *data2, uint32_t data_len2,
                                      void (*special_free_extra_func)(void *extra_data), uint32_t delay_ms);

#ifdef __cplusplus
}
#endif

#endif 
