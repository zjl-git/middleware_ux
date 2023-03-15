#ifndef __UX_MESSAGE_QUEUE_H__
#define __UX_MESSAGE_QUEUE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "ux_ports.h"
#include "ux_activity.h"

typedef struct _ux_msg {
    uint8_t msg_priority;
    uint32_t msg_group;
    uint32_t msg_id;

    void *msg_data;
    uint32_t msg_data_len;

    void *msg_data2;
    uint32_t msg_data_len2;
    void (*special_free_extra_func)(void *extra_data);

    struct _ux_msg *next_msg;
} ux_msg;


typedef struct _ux_msg_queue {
    ux_msg *top;
    ux_msg **remove_cursor;
    ux_msg *last_of_priority[UX_ACTIVITY_EVENT_PRIORITY_SYSTEM + 1];

    uint32_t length;
    void *mutex;

    void (*push_msg)(struct _ux_msg_queue *msg_queue, bool from_isr, ux_msg *input_msg);
    ux_msg *(*get_top_msg)(struct _ux_msg_queue *msg_queue);
    uint8_t (*get_length)(struct _ux_msg_queue *msg_queue);
    ux_msg *(*remove_top_msg)(struct _ux_msg_queue *msg_queue);
    void (*output_msg)(struct _ux_msg_queue *msg_queue);
    void (*remove_msg)(struct _ux_msg_queue *msg_queue, uint32_t group, uint32_t id);
} ux_msg_queue;

void ux_message_queue_init(ux_msg_queue *msg_queue);

ux_msg_queue *ux_message_queue_new(void);

void ux_message_queue_input_msg(ux_msg *input_msg, uint8_t priority, uint32_t group, uint32_t id,
                                void *data, uint32_t data_len, void *data2, uint32_t data_len2,
                                void (*special_free_extra_func)(void *extra_data));

#ifdef __cplusplus
}
#endif

#endif 
