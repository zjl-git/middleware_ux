#include "ux_activity.h"

#define UX_ACTIVITY_MAX_COUNT           40

typedef struct _ux_activity_item {
    ux_activity_internal activity;
    uint16_t index_in_array;
} ux_activity_item;

typedef struct _ux_activity_temp_save {
    ux_activity_internal *acti;
    struct _ux_activity_temp_save *next;
} ux_activity_temp_save;

typedef struct _ux_activity_waiting_allowed {
    ux_activity_temp_save *waiting_acti_list;
    ux_activity_internal *request_activity;
    uint32_t request_id;
    struct _ux_activity_waiting_allowed *next;
} ux_activity_waiting_allowed;

static ux_activity_item g_ux_activity_array[UX_ACTIVITY_MAX_COUNT];
static ux_activity_waiting_allowed *g_waiting_allowed_list = NULL;
static ux_activity_internal *g_pre_proc_acti = NULL;
static ux_activity_internal *g_acti_list = NULL;
static bool g_already_started = false;

ux_activity_internal *ux_activity_get_preproc_item(void)
{
    memset(&g_ux_activity_array[0], 0x00, sizeof(ux_activity_item));
    return &g_ux_activity_array[0].activity;
}

ux_activity_internal *ux_activity_get_free_item(void)
{
    uint16_t i = 0;
    for (i = UX_ACTIVITY_MAX_COUNT - 1; i > 0; i--) {
        if (g_ux_activity_array[i].index_in_array == 0) {
            memset(&g_ux_activity_array[i], 0x00, sizeof(ux_activity_item));
            g_ux_activity_array[i].index_in_array = 1;
            return &g_ux_activity_array[i].activity;
        }
    }
    return NULL;
}

void ux_activity_free_item(ux_activity_internal *activity)
{
    ux_activity_item *item = (ux_activity_item *)activity;
    item->index_in_array = 0;
}

bool ux_activity_add(ux_activity_internal *activity, void *data, size_t data_len)
{
    bool ret = false;
    ux_activity_internal **acti;

    if (activity->activity_type == UX_ACTIVITY_PRE_PROC) {
        if (g_pre_proc_acti) {
            UX_LOG_E("Cannot add 2 pre_proc_activities");
        } else if (g_already_started) {
            UX_LOG_E("Must add pre_proc activity before start");
        } else {
            g_pre_proc_acti = activity;
            ret = true;
        }
        return ret;
    }

    /*which have higher priority than current activity*/
    for (acti = &g_acti_list; *acti != NULL; acti = &(*acti)->next) {
        if ((*acti)->priority <= activity->priority) {
            /*acti will point to the activity which have lower priority than current*/
            break;
        }
    }

    if (g_already_started) {
        if (*acti == g_acti_list && g_acti_list) {  /*this activity is next foreground activity*/
            /*pause last foreground activity*/
            g_acti_list->proc_event_func(&(g_acti_list->external_activity), UX_ACTIVITY_SYSTEM, UX_ACTIVITY_SYSTEM_EVENT_ID_FINISH_ACTI, NULL, 0);
        }

        ret = activity->proc_event_func(&(activity->external_activity), UX_ACTIVITY_SYSTEM, UX_ACTIVITY_SYSTEM_EVENT_ID_SYSTEM_START, data, data_len);
        if (ret) {
            /*if priority of *acti is lower than current, new activity is next foreground activity*/
            if (*acti == g_acti_list) {
                activity->proc_event_func(&(activity->external_activity), UX_ACTIVITY_SYSTEM, UX_ACTIVITY_SYSTEM_EVENT_ID_START_ACTI, NULL, 0);
            }
        } else {
            UX_LOG_E("On create failed in activity = %x", 1, activity->proc_event_func);
            if (*acti == g_acti_list && g_acti_list) {  /*resume from pause status*/
                g_acti_list->proc_event_func(&(g_acti_list->external_activity), UX_ACTIVITY_SYSTEM, UX_ACTIVITY_SYSTEM_EVENT_ID_START_ACTI, NULL, 0);
                g_acti_list->proc_event_func(&(g_acti_list->external_activity), UX_ACTIVITY_SYSTEM, UX_ACTIVITY_SYSTEM_EVENT_ID_TRIGGER_REFRESH, NULL, 0);
            }
        }
    } else {
        ret = true;
    }

    if (ret) {
        activity->next = (*acti);
        *acti = activity;
    } else {
        ux_activity_free_item(activity);
    }

    return ret;
}

void ux_activity_remove(ux_activity_internal *activity)
{
    bool ret = false;
    ux_activity_internal **acti = &g_acti_list;
    ux_activity_internal *temp_acti = g_acti_list;

    if (activity == NULL) {
        UX_LOG_E("activity is null");
        return;
    }

    if (activity->activity_type != UX_ACTIVITY_TRANSIENT) {
        UX_LOG_E("activity type is transient");
        return ;
    }

    /*To avoid duplicate remove message for the same activity, check the activity is valid*/
    for (temp_acti = g_acti_list; temp_acti != NULL && temp_acti->activity_type == UX_ACTIVITY_TRANSIENT; temp_acti = temp_acti->next) {
        if (temp_acti == activity) {
            if (activity == g_acti_list) {
                activity->proc_event_func(&(activity->external_activity), UX_ACTIVITY_SYSTEM, UX_ACTIVITY_SYSTEM_EVENT_ID_FINISH_ACTI, NULL, 0);
            }
            ret = activity->proc_event_func(&(activity->external_activity), UX_ACTIVITY_SYSTEM, UX_ACTIVITY_SYSTEM_EVENT_ID_SYSTEM_STOP, NULL, 0);
            break;
        }
    }

    if (ret) {
        for (acti = &g_acti_list; *acti != NULL; acti = &(*acti)->next) {
            if ((*acti)->started_by == activity) {
                (*acti)->started_by = NULL;
                UX_LOG_D("activity %x(%x)'s branch %x(%x) is detroied", (uint32_t)(*acti), (uint32_t)(*acti)->proc_event_func, 
                         (uint32_t)activity, (uint32_t)activity->proc_event_func);
            }

            if (*acti == activity) {
                *acti = activity->next;
            }
        }

        /*activity has been removed from list*/
        if (activity->next == g_acti_list && g_acti_list) {
            g_acti_list->proc_event_func(&(g_acti_list->external_activity), UX_ACTIVITY_EVENT_PRIORITY_SYSTEM,
                                      UX_ACTIVITY_SYSTEM_EVENT_ID_START_ACTI, NULL, 0);
            g_acti_list->proc_event_func(&(g_acti_list->external_activity), UX_ACTIVITY_EVENT_PRIORITY_SYSTEM,
                                      UX_ACTIVITY_SYSTEM_EVENT_ID_TRIGGER_REFRESH, NULL, 0);
        }
        ux_activity_free_item(activity);
    } else {
        UX_LOG_I("activity refused remove %x(%x)", (uint32_t)activity, (uint32_t)activity->proc_event_func);
    }
}

void ux_activity_start(void)
{
    ux_activity_internal *temp_acti = NULL;
    if (g_already_started) {
        UX_LOG_E("ui_shell_activity_stack_start have been called before");
        return;
    }

    if (g_pre_proc_acti) {
        g_pre_proc_acti->proc_event_func(&g_pre_proc_acti->external_activity, UX_ACTIVITY_SYSTEM, UX_ACTIVITY_SYSTEM_EVENT_ID_SYSTEM_START, NULL, 0);
    }

    for (temp_acti = g_acti_list; temp_acti != NULL; temp_acti = temp_acti->next) {
        temp_acti->proc_event_func(&temp_acti->external_activity, UX_ACTIVITY_SYSTEM, UX_ACTIVITY_SYSTEM_EVENT_ID_SYSTEM_START, NULL, 0);
    }

    /*refresh the first idel activity*/
    if (g_acti_list) {
        g_acti_list->proc_event_func(&g_acti_list->external_activity, UX_ACTIVITY_SYSTEM, UX_ACTIVITY_SYSTEM_EVENT_ID_TRIGGER_REFRESH, NULL, 0);
    }

    g_already_started = true;
}

void ux_activity_set_result(ux_activity_internal *target_acti, void *data, uint32_t data_len)
{
    ux_activity_internal *temp_acti = g_acti_list;

    for (temp_acti = g_acti_list; temp_acti != NULL; temp_acti = temp_acti->next) {
        if (temp_acti == target_acti) {
            target_acti->proc_event_func(&target_acti->external_activity, UX_ACTIVITY_SYSTEM, UX_ACTIVITY_SYSTEM_EVENT_ID_SET_RESULT, data, data_len);
        }
    }
}

void ux_activity_refresh_activity(ux_activity_internal *target_acti)
{
    if (g_acti_list != target_acti) {
        UX_LOG_I("target activity is not the top activity, so ignore the refresh request");
        return;
    }

    target_acti->proc_event_func(&target_acti->external_activity, UX_ACTIVITY_SYSTEM, UX_ACTIVITY_SYSTEM_EVENT_ID_TRIGGER_REFRESH, NULL, 0);
}

void ux_activity_request_allowance(uint32_t request_id, ux_activity_internal *requester)
{
    ux_activity_internal *temp_acti = g_acti_list;
    ux_activity_temp_save *acti_need_saved = NULL;
    ux_activity_waiting_allowed *waiting_data = NULL;
    bool ret;

    for (; temp_acti != NULL; temp_acti = temp_acti->next) {
        ret = temp_acti->proc_event_func(&temp_acti->external_activity, UX_ACTIVITY_SYSTEM, UX_ACTIVITY_SYSTEM_EVENT_ID_REQUEST_ALLOWANCE, (void *)request_id, 0);
        if (!ret) {
            UX_LOG_I("Not allowed the request : %d", request_id);
            if (waiting_data == NULL) {
                waiting_data = ux_ports_malloc(sizeof(ux_activity_waiting_allowed));
            }

            if (waiting_data) {
                acti_need_saved = ux_ports_malloc(sizeof(ux_activity_temp_save));
                if (acti_need_saved) {
                    acti_need_saved->acti = temp_acti;
                    acti_need_saved->next = waiting_data->waiting_acti_list;
                    waiting_data->waiting_acti_list = acti_need_saved;
                } else {
                    UX_LOG_E("malloc acti_need_saved failed");
                }
            } else {
                UX_LOG_E("malloc waiting_data failed");
            }
        }
    }

    if (waiting_data) {
        waiting_data->request_activity = requester;
        waiting_data->request_id = request_id;
        waiting_data->next = g_waiting_allowed_list;
        g_waiting_allowed_list = waiting_data;
    } else {
        requester->proc_event_func(&requester->external_activity, UX_ACTIVITY_SYSTEM, UX_ACTIVITY_SYSTEM_EVENT_ID_ON_ALLOWANCE, (void *)request_id, 0);
    }
}

ux_activity_internal *ux_activity_allow(ux_activity_internal *target_acti, uint32_t request_id)
{
    ux_activity_waiting_allowed **wait_data = NULL;
    ux_activity_temp_save **temp_saved_acti = NULL;
    ux_activity_internal *acti_need_notify = NULL;

    for (wait_data = &g_waiting_allowed_list; wait_data && *wait_data; wait_data = &(*wait_data)->next) {
        if (request_id != (*wait_data)->request_id) {
            continue;
        }

        for (temp_saved_acti = &(*wait_data)->waiting_acti_list; temp_saved_acti && *temp_saved_acti; temp_saved_acti = &(*temp_saved_acti)->next) {
            if ((*temp_saved_acti)->acti == target_acti) {
                ux_activity_temp_save *temp_activity_need_free = *temp_saved_acti;
                *temp_saved_acti = (*temp_saved_acti)->next;
                ux_ports_free(temp_activity_need_free);
            }
        }

        if ((*wait_data)->waiting_acti_list == NULL) {
            acti_need_notify = (*wait_data)->request_activity;
            ux_activity_waiting_allowed *wait_data_need_free = *wait_data;
            *wait_data = (*wait_data)->next;
            ux_ports_free(wait_data_need_free);
        } else {
            wait_data = &(*wait_data)->next;
        }
        break;
    }
    return acti_need_notify;
}

void ux_activity_allowed(uint32_t request_id, ux_activity_internal *requester)
{
    if (requester) {
        requester->proc_event_func(&requester->external_activity, UX_ACTIVITY_SYSTEM, UX_ACTIVITY_SYSTEM_EVENT_ID_ON_ALLOWANCE, (void *)request_id, 0);
    }
}

void ux_activity_traverse(uint32_t event_group, uint32_t event_id, void *data, uint32_t data_len)
{
    bool ret = false;
    ux_activity_internal *temp_acti = NULL;

    if (g_pre_proc_acti) {
        ret = g_pre_proc_acti->proc_event_func(&g_pre_proc_acti->external_activity, event_group, event_id, data, data_len);
    }

    if (ret != true) {
        for (temp_acti = g_acti_list; temp_acti != NULL; temp_acti = temp_acti->next) {
            ret = temp_acti->proc_event_func(&temp_acti->external_activity, event_group, event_id, data, data_len);
            if (ret == true) {
                UX_LOG_I("traverse_stack return true 0x%x-0x%x by function 0x%x", event_group, event_id, temp_acti->proc_event_func);
                break;
            }
        }
    }
}