#include "ux_message_delay_queue.h"

static void _test_printf(ux_delay_msg_queue *msg_queue)
{
    ux_delay_msg *msg;
    UX_LOG_D("length : %d", msg_queue->parent.length);
    UX_LOG_D("*******************************************************");
    msg = (ux_delay_msg *)msg_queue->parent.top;
    while (msg) {
        UX_LOG_D("test printf time:%d, group:%d id:%d", msg->msg_act_time, msg->basic_msg.msg_group, msg->basic_msg.msg_id);
        msg = (ux_delay_msg *)msg->basic_msg.next_msg;
    }
    UX_LOG_D("*******************************************************");
}

/*a > b*/
static bool ux_message_is_time_larger(uint32_t a, uint32_t b)
{
    return ((a > b) && (a - b < (1 << 31))) || ((b > a) && (b - a > (1 << 31)));
}

static void ux_message_push_delay_msg(ux_delay_msg_queue *msg_queue, ux_delay_msg *input_msg)
{
    ux_delay_msg *msg;
    ux_delay_msg *current_msg = NULL, *last_msg = NULL;
    uint32_t sync_mask;

    if (input_msg == NULL) {
        return;
    }
    UX_LOG_D("push delay message %x-%x, act_time : %d, length : %d\n", 
              input_msg->basic_msg.msg_group, input_msg->basic_msg.msg_id, input_msg->msg_act_time, msg_queue->parent.length);

    msg = (ux_delay_msg *)ux_ports_malloc(sizeof(ux_delay_msg));
    if (msg == NULL) {
        UX_LOG_E("push dalay message failed, malloc msg is null");
        return;
    }

    memcpy(msg, input_msg, sizeof(ux_delay_msg));
    msg->basic_msg.next_msg = NULL;

    ux_ports_synchronized(&sync_mask);

    if (msg_queue->parent.top == NULL) {
        msg_queue->parent.top = &msg->basic_msg;
    } else {
        current_msg = (ux_delay_msg *)msg_queue->parent.top;
        while(1) {
            if (current_msg == NULL || ux_message_is_time_larger(current_msg->msg_act_time, msg->msg_act_time)) {
                msg->basic_msg.next_msg = &current_msg->basic_msg;
                if (last_msg) {
                    last_msg->basic_msg.next_msg = &msg->basic_msg;
                } else {
                    msg_queue->parent.top = &msg->basic_msg;
                }
            } else {
                last_msg  = current_msg;
                current_msg = (ux_delay_msg *)current_msg->basic_msg.next_msg;
            }
        }
    }
    msg_queue->parent.length++;
    ux_ports_synchronized_end(sync_mask);

    _test_printf(msg_queue);
}

static ux_delay_msg *ux_message_remove_timeout_msg(ux_delay_msg_queue *msg_queue)
{
    ux_delay_msg *temp, *last_temp = NULL, *ret_msgs = NULL;
    uint32_t sync_mask, timeout_msg_count = 0;
    uint32_t current = ux_ports_get_currents_ms();

    UX_LOG_D("top message : %d, current_time = %d\n",
              msg_queue->parent.top ? msg_queue->parent.top->msg_group : -1,current);

    ux_ports_synchronized(&sync_mask);

    for (temp = (ux_delay_msg *)msg_queue->parent.top; temp; temp = (ux_delay_msg *)temp->basic_msg.next_msg) {
        if (ux_message_is_time_larger(current, temp->msg_act_time)) {
            timeout_msg_count++;
            last_temp = temp;
        } else {
            break;
        }
    }

    if (timeout_msg_count > 0) {
        msg_queue->parent.length -= timeout_msg_count;
        ret_msgs = (ux_delay_msg *)msg_queue->parent.top;
        msg_queue->parent.top = &temp->basic_msg;
        if (last_temp) {
            last_temp->basic_msg.next_msg = NULL;
        }
    }

    ux_ports_synchronized_end(sync_mask);

    _test_printf(msg_queue);
    return ret_msgs;
}

static uint32_t ux_message_get_next_timeout(ux_delay_msg_queue *msg_queue)
{
    uint32_t sync_mask, ret = 0;
    uint32_t current = ux_ports_get_currents_ms();

    ux_ports_synchronized(&sync_mask);

    if (msg_queue->parent.top) {
        if (ux_message_is_time_larger(current, ((ux_delay_msg *)(msg_queue->parent.top))->msg_act_time)) {
            ret = 0;
        } else {
            ret = ((ux_delay_msg *)(msg_queue->parent.top))->msg_act_time - current;
        }
    } else {
        ret = UX_PORT_MAX_DELAY;
    }

    ux_ports_synchronized_end(sync_mask);
    return ret;
}

static void ux_message_delay_queue_init(ux_delay_msg_queue *msg_queue)
{
    ux_message_queue_init(&msg_queue->parent);
    msg_queue->push_delay_msg = ux_message_push_delay_msg;
    msg_queue->remove_timeout_msg = ux_message_remove_timeout_msg;
    msg_queue->get_next_timeout = ux_message_get_next_timeout;
}

ux_delay_msg_queue *ux_message_delay_queue_new(void)
{
    ux_delay_msg_queue *msg_queue = (ux_delay_msg_queue *)ux_ports_malloc(sizeof(ux_delay_msg_queue));
    if (msg_queue) {
        memset(msg_queue, 0x00, sizeof(ux_delay_msg_queue));
        ux_message_delay_queue_init(msg_queue);
    } else {
        UX_LOG_E("message delay queue new failed, malloc msg is null");
    }
    return msg_queue;
}

void ux_message_delay_queue_input_msg(ux_delay_msg *input_msg, uint8_t priority, uint32_t group, uint32_t id,
                                      void *data, uint32_t data_len, void *data2, uint32_t data_len2,
                                      void (*special_free_extra_func)(void *extra_data), uint32_t delay_ms)
{
    if (input_msg == NULL) {
        return ;
    }

    ux_message_queue_input_msg(&input_msg->basic_msg, priority, group, id, data, data_len, data2, data_len2, special_free_extra_func);
    input_msg->msg_act_time = ux_ports_get_currents_ms() + delay_ms;
}
