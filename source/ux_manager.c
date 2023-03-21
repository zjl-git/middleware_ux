#include "ux_manager.h"
#include "ux_activity.h"
#include "ux_message_queue.h"

ux_manager_handler *g_manager_handler;

void ux_start(void)
{
    ux_msg msg;
    if (g_manager_handler != NULL) {
        UX_LOG_E("already started a message handler");
        return;
    }

    g_manager_handler = (ux_manager_handler *)ux_message_handler_new();
    if (g_manager_handler == NULL) {
        UX_LOG_E("manager handler new fail");
        return;
    }

    ux_message_queue_input_msg(&msg, UX_ACTIVITY_EVENT_PRIORITY_SYSTEM, UX_ACTIVITY_SYSTEM, 
                               UX_ACTIVITY_SYSTEM_EVENT_ID_SYSTEM_START, NULL, 0, NULL, 0, NULL);
    g_manager_handler->handler.send_msg(&(g_manager_handler->handler), false, &msg);
}

void ux_send_event(bool from_isr, ux_activity_event_priority priority, uint32_t event_group, uint32_t event_id, 
                   void *data, uint32_t data_len, void(*special_free_extra_func)(void), uint32_t delay_ms)
{
    if (g_manager_handler == NULL) {
        UX_LOG_E("should not call send event before ux running");
        return;
    }

    if (priority > UX_ACTIVITY_EVENT_PRIORITY_SYSTEM) {
        UX_LOG_E("priority error");
        return ;
    }

    UX_LOG_D("send event: 0x%x, 0x%x, delay:%d", event_group, event_id, delay_ms);
    if (delay_ms != 0) {
        ux_delay_msg msg;
        ux_message_delay_queue_input_msg(&msg, priority, event_group, event_id, data, data_len, NULL, 0, NULL, delay_ms);
        g_manager_handler->handler.send_delay_msg(&(g_manager_handler->handler), from_isr, &msg);
    } else {
        ux_msg msg;
        ux_message_queue_input_msg(&msg, priority, event_group, event_id, data, data_len, NULL, 0, NULL);
        g_manager_handler->handler.send_msg(&(g_manager_handler->handler), from_isr, &msg);
    }
}

void ux_remove_event(uint32_t event_group, uint32_t event_id)
{
    if (g_manager_handler == NULL) {
        UX_LOG_E("should not call send event before ux running");
        return;
    }

    UX_LOG_I("remove event: 0x%x, 0x%x",event_group, event_id);
    g_manager_handler->handler.remove_msg(&(g_manager_handler->handler), event_group, event_id);
}

void ux_set_pre_proc_func(ux_proc_event_func_t proc_func)
{
    if (g_manager_handler) {
        UX_LOG_E("ux shell has already started, cannot call ui_shell_set_pre_proc_func");
        return ;
    }

    ux_activity_internal *activity = ux_activity_get_preproc_item();
    if (activity == NULL) {
        UX_LOG_E("new preproc item failed");
        return;
    }

    activity->activity_type = UX_ACTIVITY_PRE_PROC;
    activity->proc_event_func = proc_func;
    activity->priority = UX_ACTIVITY_PRIORITY_HIGHEST;
    ux_activity_add(activity, NULL, 0);
}

void ux_start_activity(ux_activity_context *self, ux_proc_event_func_t proc_func, 
                       ux_activity_priority priority, void *extra_data, uint32_t data_len)
{
    ux_activity_internal *activity = NULL;
    ux_msg msg;

    if (g_manager_handler == NULL && priority > UX_ACTIVITY_PRIORITY_IDLE_TOP) {
        UX_LOG_E("ux not started, priority > top");
        return ;
    }

    if (proc_func == NULL || priority > UX_ACTIVITY_PRIORITY_HIGHEST) {
        UX_LOG_E("func is null or priority > highest");
        return ;
    }

    activity = ux_activity_get_free_item();
    if (activity == NULL) {
        UX_LOG_E("new free item failed");
        return ;
    }

    if (priority <= UX_ACTIVITY_PRIORITY_IDLE_TOP) {
        activity->activity_type = UX_ACTIVITY_IDLE;
    } else {
        activity->activity_type = UX_ACTIVITY_TRANSIENT;
    }
    activity->proc_event_func = proc_func;
    activity->priority = priority;
    activity->started_by = (ux_activity_internal *)self;

    if (g_manager_handler == NULL) {
        ux_activity_add(activity, NULL, 0);
    } else {
        ux_message_queue_input_msg(&msg, UX_ACTIVITY_EVENT_PRIORITY_SYSTEM, UX_ACTIVITY_SYSTEM, 
                                    UX_ACTIVITY_SYSTEM_EVENT_ID_START_ACTI, extra_data, data_len, activity, 0, NULL);
        g_manager_handler->handler.send_msg(&(g_manager_handler->handler), false, &msg);
    }
}

void ux_finish_activity(ux_activity_context *self, ux_activity_context *target_activity)
{
    ux_msg msg;
    if (g_manager_handler == NULL) {
        UX_LOG_E("should not call finish activity before ui shell running");
        return;
    }

    if (target_activity == NULL) {
        UX_LOG_E("target activity is null");
        return;
    }

    ux_message_queue_input_msg(&msg, UX_ACTIVITY_EVENT_PRIORITY_SYSTEM, UX_ACTIVITY_SYSTEM, 
                                UX_ACTIVITY_SYSTEM_EVENT_ID_FINISH_ACTI, NULL, 0, target_activity, 0, NULL);
    g_manager_handler->handler.send_msg(&(g_manager_handler->handler), false, &msg);
}

void ux_set_result(ux_activity_context *self, void *data, uint32_t data_len)
{
    ux_msg msg;
    ux_activity_internal *internal_self = (ux_activity_internal *)self;

    if (g_manager_handler == NULL) {
        UX_LOG_E("should not call set_result before ui shell running");
        return;
    }

    if (internal_self == NULL || internal_self->started_by == NULL) {
        UX_LOG_E("this activity have no trunk activity");
        return;
    }

    ux_message_queue_input_msg(&msg, UX_ACTIVITY_EVENT_PRIORITY_SYSTEM, UX_ACTIVITY_SYSTEM, 
                                UX_ACTIVITY_SYSTEM_EVENT_ID_SET_RESULT, data, data_len, internal_self->started_by, 0, NULL);
    g_manager_handler->handler.send_msg(&(g_manager_handler->handler), false, &msg);
}

void ux_request_allowance(ux_activity_context *self, uint32_t request_id)
{
    ux_msg msg;
    if (g_manager_handler == NULL) {
        UX_LOG_E("should not call request allowed before ui shell running");
        return;
    }

    if (self == NULL) {
        UX_LOG_E("self is null");
        return;
    }

    ux_message_queue_input_msg(&msg, UX_ACTIVITY_EVENT_PRIORITY_SYSTEM, UX_ACTIVITY_SYSTEM, 
                                UX_ACTIVITY_SYSTEM_EVENT_ID_REQUEST_ALLOWANCE, (void *)request_id, 0, (void *)self, 0, NULL);
    g_manager_handler->handler.send_msg(&(g_manager_handler->handler), false, &msg);
}

void ux_grant_allowance(ux_activity_context *self, uint32_t request_id)
{
    ux_msg msg;
    ux_activity_internal *acti_need_notify = NULL;
    if (g_manager_handler == NULL) {
        UX_LOG_E("should not call grant allowed before ui shell running");
        return;
    }

    if (self == NULL) {
        UX_LOG_E("self is null");
        return;
    }

    acti_need_notify = ux_activity_allow((ux_activity_internal *)self, request_id);
    if (acti_need_notify) {
       ux_message_queue_input_msg(&msg, UX_ACTIVITY_EVENT_PRIORITY_SYSTEM, UX_ACTIVITY_SYSTEM, 
                                  UX_ACTIVITY_SYSTEM_EVENT_ID_ON_ALLOWANCE, (void *)request_id, 0, (void *)acti_need_notify, 0, NULL);
        g_manager_handler->handler.send_msg(&(g_manager_handler->handler), false, &msg); 
    }
}

void ux_refresh_activity(ux_activity_context *self, ux_activity_context *target)
{
    ux_msg msg;
    if (g_manager_handler == NULL) {
        UX_LOG_E("should not call set_result before ui shell running");
        return;
    }

    if (target == NULL) {
        UX_LOG_E("target is NULL");
        return;
    }

    ux_message_queue_input_msg(&msg, UX_ACTIVITY_EVENT_PRIORITY_SYSTEM, UX_ACTIVITY_SYSTEM, 
                                UX_ACTIVITY_SYSTEM_EVENT_ID_TRIGGER_REFRESH, NULL, 0, target, 0, NULL);
    g_manager_handler->handler.send_msg(&(g_manager_handler->handler), false, &msg);
}