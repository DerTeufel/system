/* Copyright Statement:
 * *
 * * This software/firmware and related documentation ("MediaTek Software") are
 * * protected under relevant copyright laws. The information contained herein
 * * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * * Without the prior written permission of MediaTek inc. and/or its licensors,
 * * any reproduction, modification, use or disclosure of MediaTek Software,
 * * and information contained herein, in whole or in part, shall be strictly prohibited.
 * *
 * * MediaTek Inc. (C) 2010. All rights reserved.
 * *
 * * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 * *
 * * The following software/firmware and/or related documentation ("MediaTek Software")
 * * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * * applicable license agreements with MediaTek Inc.
 * */

/************************************************************************************
 *
 *  Filename:      btif_b3ds.c
 *
 *  Description:   Bluetooth 3d Synchronization Interface
 *
 *
 ***********************************************************************************/
#include "bt_target.h"

#if defined(MTK_B3DS_INCLUDED) && (MTK_B3DS_INCLUDED == TRUE)

#include <hardware/bluetooth.h>
#include <string.h>
#include "btif_b3ds.h"

#define LOG_TAG "BTIF_B3DS"
#include "btif_common.h"
#include "btif_util.h"
#include "btm_api.h"

#include "bta_api.h"
#include "bta_b3ds_api.h"

#define BTB3DS_ROLE_NONE    0
#define BTB3DS_ROLE_3DD     1
#define BTB3DS_ROLE_3DG     2

#define BTB3DS_LOCAL_ROLE   BTB3DS_ROLE_3DD

typedef struct
{
    int enabled;
    UINT8 enable_legacy;
} btb3ds_cb_t;

btb3ds_cb_t btb3ds_cb;
static int jni_initialized = FALSE, stack_initialized = FALSE;

static bt_status_t btb3ds_jni_init(const btb3ds_callbacks_t *callbacks, UINT8 enable_legacy_reference_protocol);
static void btb3ds_jni_cleanup(void);
static bt_status_t btb3ds_set_broadcast(btb3ds_vedio_mode_t vedio_mode, btb3ds_frame_sync_period_t frame_sync_period, UINT32 panel_delay);
static bt_status_t btb3ds_start_broadcast(void);
static bt_status_t btb3ds_stop_broadcast(void);
static bt_status_t btb3ds_start_sync_train(UINT32 synchronization_train_to);
static bt_status_t btb3ds_stop_sync_train(void);
static void bta_b3ds_callback(tBTA_B3DS_EVT event, tBTA_B3DS *p_data);

static btb3ds_interface_t b3ds_if = {
    sizeof(b3ds_if),
    btb3ds_jni_init,
    btb3ds_set_broadcast,
    btb3ds_start_broadcast,
    btb3ds_stop_broadcast,
    btb3ds_start_sync_train,
    btb3ds_stop_sync_train,
    btb3ds_jni_cleanup
};

btb3ds_interface_t *btif_b3ds_get_interface()
{
    BTIF_TRACE_DEBUG("btif_b3ds_get_interface");

    return &b3ds_if;
}

static void b3ds_disable()
{
    BTIF_TRACE_DEBUG("b3ds_disable");

    if(btb3ds_cb.enabled)
    {
        btb3ds_cb.enabled = 0;
        BTA_B3dsDisable();
    }
}

void btif_b3ds_cleanup()
{
    BTIF_TRACE_DEBUG("btif_b3ds_cleanup");

    if(stack_initialized)
    {
        b3ds_disable();
    }
    stack_initialized = FALSE;
}

void btif_b3ds_init(UINT8 enable_legacy_reference_protocol)
{
    BTIF_TRACE_DEBUG("btif_b3ds_init: jni_initialized = %d, btb3ds_cb.enabled:%d", jni_initialized, btb3ds_cb.enabled);

    stack_initialized = TRUE;

    if (BTB3DS_LOCAL_ROLE != BTB3DS_ROLE_3DD)
        BTIF_TRACE_ERROR("btif_b3ds_init: only support 3DD role");

#if defined(MTK_B3DS_DEMO) && (MTK_B3DS_DEMO == TRUE)
    jni_initialized = TRUE;
#endif

    if (jni_initialized && !btb3ds_cb.enabled)
    {
        BTIF_TRACE_DEBUG("Enabling B3DS....");
        memset(&btb3ds_cb, 0, sizeof(btb3ds_cb));
        btb3ds_cb.enable_legacy = enable_legacy_reference_protocol;

        BTA_B3dsEnable(bta_b3ds_callback, enable_legacy_reference_protocol);
        btb3ds_cb.enabled = 1;
    }
}

static btb3ds_callbacks_t callback;
static bt_status_t btb3ds_jni_init(const btb3ds_callbacks_t *callbacks, UINT8 enable_legacy_reference_protocol)
{
    BTIF_TRACE_DEBUG("btb3ds_jni_init: stack_initialized = %d, btb3ds_cb.enabled:%d", stack_initialized, btb3ds_cb.enabled);

    jni_initialized = TRUE;
    callback = *callbacks;

    if(stack_initialized && !btb3ds_cb.enabled)
        btif_b3ds_init(enable_legacy_reference_protocol);
    else if (enable_legacy_reference_protocol != btb3ds_cb.enable_legacy)
    {
        BTA_B3dsSetLegacy(enable_legacy_reference_protocol);
        btb3ds_cb.enable_legacy = enable_legacy_reference_protocol;
    }

    return BT_STATUS_SUCCESS;
}

static void btb3ds_jni_cleanup()
{
    BTIF_TRACE_DEBUG("btb3ds_jni_cleanup");

    b3ds_disable();
    jni_initialized = FALSE;
}

static bt_status_t btb3ds_set_broadcast(btb3ds_vedio_mode_t vedio_mode, btb3ds_frame_sync_period_t frame_sync_period, UINT32 panel_delay)
{
    BTIF_TRACE_DEBUG("btb3ds_set_broadcast");

    BTA_B3dsSetBroadcast(vedio_mode, frame_sync_period, panel_delay);

    return BT_STATUS_SUCCESS;
}

static bt_status_t btb3ds_start_broadcast()
{
    BTIF_TRACE_DEBUG("btb3ds_start_broadcast");

    BTA_B3dsStartBroadcast();

    return BT_STATUS_SUCCESS;
}

static bt_status_t btb3ds_stop_broadcast()
{
    BTIF_TRACE_DEBUG("btb3ds_stop_broadcast");

    BTA_B3dsStopBroadcast();

    return BT_STATUS_SUCCESS;
}

static bt_status_t btb3ds_start_sync_train(UINT32 synchronization_train_to)
{
    BTIF_TRACE_DEBUG("btb3ds_start_sync_train");

    BTA_B3dsStartSyncTrain(synchronization_train_to);

    return BT_STATUS_SUCCESS;
}

static bt_status_t btb3ds_stop_sync_train()
{
    BTIF_TRACE_DEBUG("btb3ds_stop_sync_train");

    BTA_B3dsStopSyncTrain();

    return BT_STATUS_SUCCESS;
}

static void bta_b3ds_callback_transfer(UINT16 event, char *p_param)
{
    tBTA_B3DS *p_data = (tBTA_B3DS *)p_param;

    BTIF_TRACE_DEBUG("bta_b3ds_callback_transfer");
    switch(event)
    {
        case BTA_B3DS_ENABLE_EVT:
            BTIF_TRACE_DEBUG("bta_b3ds_callback_transfer: BTA_B3DS_ENABLE_EVT");
            if(callback.bcst_state_cb)
                callback.bcst_state_cb(BTB3DS_BCST_STATE_NOT_BROADCASTING, p_data->status == BTA_B3DS_SUCCESS ? BT_STATUS_SUCCESS : BT_STATUS_FAIL);
#if defined(MTK_B3DS_DEMO) && (MTK_B3DS_DEMO == TRUE)
            BTA_B3dsSetBroadcast(BTA_B3DS_VEDIO_MODE_3D, BTA_B3DS_PERIOD_DYNAMIC_CALCULATED,0);
#endif
            break;

         case BTA_B3DS_SET_BROADCAST_EVT:
            BTIF_TRACE_DEBUG("bta_b3ds_callback_transfer: BTA_B3DS_SET_BROADCAST_EVT");
#if defined(MTK_B3DS_DEMO) && (MTK_B3DS_DEMO == TRUE)
            BTA_B3dsStartBroadcast();
#endif
            break;

         case BTA_B3DS_START_BROADCAST_EVT:
            BTIF_TRACE_DEBUG("bta_b3ds_callback_transfer: BTA_B3DS_START_BROADCAST_EVT");
            if(callback.bcst_state_cb)
                callback.bcst_state_cb(BTB3DS_BCST_STATE_BROADCASTING, p_data->status == BTA_B3DS_SUCCESS ? BT_STATUS_SUCCESS : BT_STATUS_FAIL);
#if defined(MTK_B3DS_DEMO) && (MTK_B3DS_DEMO == TRUE)
            BTA_B3dsStartSyncTrain(0x0002EE00);
#endif
            break;

         case BTA_B3DS_STOP_BROADCAST_EVT:
            BTIF_TRACE_DEBUG("bta_b3ds_callback_transfer: BTA_B3DS_STOP_BROADCAST_EVT");
            if(callback.bcst_state_cb)
                callback.bcst_state_cb(BTB3DS_BCST_STATE_NOT_BROADCASTING, p_data->status == BTA_B3DS_SUCCESS ? BT_STATUS_SUCCESS : BT_STATUS_FAIL);
            break;

         case BTA_B3DS_START_SYNC_TRAIN_EVT:
            BTIF_TRACE_DEBUG("bta_b3ds_callback_transfer: BTA_B3DS_START_SYNC_TRAIN_EVT");
            if(callback.sync_state_cb)
                callback.sync_state_cb(BTB3DS_SYNC_STATE_SYNCHRONIZABLE, p_data->status == BTA_B3DS_SUCCESS ? BT_STATUS_SUCCESS : BT_STATUS_FAIL);
            break;

         case BTA_B3DS_STOP_SYNC_TRAIN_EVT:
            BTIF_TRACE_DEBUG("bta_b3ds_callback_transfer: BTA_B3DS_STOP_SYNC_TRAIN_EVT");
            if(callback.sync_state_cb)
                callback.sync_state_cb(BTB3DS_SYNC_STATE_NON_SYNCHRONIZABLE, p_data->status == BTA_B3DS_SUCCESS ? BT_STATUS_SUCCESS : BT_STATUS_FAIL);
            break;

         case BTA_B3DS_CONNECT_ANNOUNCE_MSG_EVT:
            BTIF_TRACE_DEBUG("bta_b3ds_callback_transfer: BTA_B3DS_CONNECT_ANNOUNCE_MSG_EVT");
            if(callback.connect_announce_cb)
                callback.connect_announce_cb((const bt_bdaddr_t*) p_data->con_anno.bd_addr,
                        p_data->con_anno.option & BTA_B3DS_CON_ANNO_MSG_ASSOCIATION_NOTIFICATION, p_data->con_anno.battery_level);
            break;

         case BTA_B3DS_LEGACY_ASSOC_NOTIFY_EVT:
            BTIF_TRACE_DEBUG("bta_b3ds_callback_transfer: BTA_B3DS_LEGACY_ASSOC_NOTIFY_EVT");
            if(callback.legacy_connect_announce_cb)
                callback.legacy_connect_announce_cb();
            break;

         default:
            BTIF_TRACE_WARNING("Unknown b3ds event %d", event);
            break;
    }
}

static void bta_b3ds_callback(tBTA_B3DS_EVT event, tBTA_B3DS *p_data)
{
    BTIF_TRACE_DEBUG("bta_b3ds_callback");
    btif_transfer_context(bta_b3ds_callback_transfer, event, (char*)p_data, sizeof(tBTA_B3DS), NULL);
}

#endif

