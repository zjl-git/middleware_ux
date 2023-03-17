#include "ux_ports.h"
#include "ux_message_queue.h"

static void _test_printf(ux_msg_queue *msg_queue)
{
    ux_msg *msg;
    
    UX_LOG_D("--------------------------------------------------------");
    UX_LOG_D("queue length : %d", msg_queue->length);
    msg = msg_queue->top;
    while (msg) {
        UX_LOG_D("queue test printf prio:%d, group:%d id:%d, data_len:%d", msg->msg_priority, msg->msg_group, msg->msg_id, msg->msg_data_len);
        msg = msg->next_msg;
    }
    UX_LOG_D("--------------------------------------------------------");
}

static void push_msg(ux_msg_queue *msg_queue, bool from_isr, ux_msg *input_msg)
{
    ux_msg *msg;
    uint32_t i = 0;
    uint32_t sync_mask;

    if (input_msg == NULL || msg_queue == NULL) {
        return;
    }
    UX_LOG_D("push message %x-%x, priority:%x", input_msg->msg_group, input_msg->msg_id, input_msg->msg_priority);
    
    msg = (ux_msg *)ux_ports_malloc(sizeof(ux_msg));
    if (msg == NULL) {
        UX_LOG_E("push message failed, malloc msg is null");
        return;
    }

    memcpy(msg, input_msg, sizeof(ux_msg));
    msg->next_msg = NULL;

    ux_ports_synchronized(&sync_mask);

    for (i = msg->msg_priority; i <= UX_ACTIVITY_EVENT_PRIORITY_SYSTEM; i++) {
        if (msg_queue->last_of_priority[i] != NULL) {    
            break;
        }
    }

    if (i <= UX_ACTIVITY_EVENT_PRIORITY_SYSTEM) {
        msg->next_msg = msg_queue->last_of_priority[i]->next_msg;
        msg_queue->last_of_priority[i]->next_msg = msg;
    } else {
        msg->next_msg = msg_queue->top;
        msg_queue->top = msg;
    }

    msg_queue->last_of_priority[msg->msg_priority] = msg;
    msg_queue->length++;

    ux_ports_synchronized_end(sync_mask);

    _test_printf(msg_queue);
}

static void remove_msg(ux_msg_queue *msg_queue, uint32_t group, uint32_t id)
{
    uint32_t sync_mask;
    ux_msg *current_msg = NULL, *last_msg = NULL;
    if (msg_queue == NULL) {
        return ;
    }

    UX_LOG_D("remove message %x-%x", group, id);

    ux_ports_synchronized(&sync_mask);
    
    msg_queue->remove_cursor = &msg_queue->top;
    while (true) {
        if (msg_queue->remove_cursor == NULL) {
            break;
        }

        current_msg = *msg_queue->remove_cursor;
        if (current_msg == NULL) {
            msg_queue->remove_cursor = NULL;
            break;
        }

        if (current_msg->msg_group != group || current_msg->msg_id != id) {
            msg_queue->remove_cursor = &current_msg->next_msg;
            continue;
        }

        *msg_queue->remove_cursor = current_msg->next_msg;
        if (current_msg == msg_queue->last_of_priority[current_msg->msg_priority]) {
            if (msg_queue->remove_cursor == &msg_queue->top) {
                msg_queue->last_of_priority[current_msg->msg_priority] = NULL;
            } else {
                last_msg = (ux_msg *)((uint8_t *)msg_queue->remove_cursor - (size_t) & ((ux_msg *)0)->next_msg);
                if (last_msg->msg_priority == current_msg->msg_priority) {
                    msg_queue->last_of_priority[current_msg->msg_priority] = last_msg;
                } else {
                    msg_queue->last_of_priority[current_msg->msg_priority] = NULL;
                }
            }
        }
        msg_queue->length--;

        if (current_msg->msg_data && current_msg->msg_data_len) {
            if (current_msg->special_free_extra_func) {
                current_msg->special_free_extra_func(current_msg->msg_data);
            }
            ux_ports_free(current_msg->msg_data);
        }

        if (current_msg->msg_data2 && current_msg->msg_data_len2) {
            ux_ports_free(current_msg->msg_data2);
        }

        ux_ports_free(current_msg);
    }

    ux_ports_synchronized_end(sync_mask);

    _test_printf(msg_queue);
}

static ux_msg *remove_top_msg(ux_msg_queue *msg_queue)
{
    uint32_t sync_mask;
    ux_msg *temp = NULL;
    if (msg_queue == NULL) {
        return temp;
    }

    UX_LOG_D("top message : %d", msg_queue->top ? msg_queue->top->msg_group : -1);
    ux_ports_synchronized(&sync_mask);

    temp = msg_queue->top;
    if (msg_queue->top != NULL) {
        if (msg_queue->last_of_priority[msg_queue->top->msg_priority] == msg_queue->top) {
            msg_queue->last_of_priority[msg_queue->top->msg_priority] = NULL;
        }
        msg_queue->top = msg_queue->top->next_msg;
        msg_queue->length--;
    }

    ux_ports_synchronized_end(sync_mask);
    return temp;
}

void ux_message_queue_init(ux_msg_queue *msg_queue)
{
    msg_queue->push_msg = push_msg;
    msg_queue->remove_msg = remove_msg;
    msg_queue->remove_top_msg = remove_top_msg;

    msg_queue->mutex = ux_ports_create_mute();
}

ux_msg_queue *ux_message_queue_new(void)
{
    ux_msg_queue *msg_queue = (ux_msg_queue *)ux_ports_malloc(sizeof(ux_msg_queue));
    if (msg_queue) {
        memset(msg_queue, 0x00, sizeof(ux_msg_queue));
        ux_message_queue_init(msg_queue);
    } else {
        UX_LOG_E("message queue new failed, malloc msg is null");
    }
    return msg_queue;
}

void ux_message_queue_input_msg(ux_msg *input_msg, uint8_t priority, uint32_t group, uint32_t id,
                                void *data, uint32_t data_len, void *data2, uint32_t data_len2,
                                void (*special_free_extra_func)(void *extra_data))
{
    if (input_msg == NULL) {
        return ;
    }

    input_msg->msg_priority = priority;
    input_msg->msg_group = group;
    input_msg->msg_id = id;
    input_msg->msg_data = data;
    input_msg->msg_data_len = data_len;
    input_msg->msg_data2 = data2;
    input_msg->msg_data_len2 = data_len2;
    input_msg->special_free_extra_func = special_free_extra_func;
    input_msg->next_msg = NULL;
}