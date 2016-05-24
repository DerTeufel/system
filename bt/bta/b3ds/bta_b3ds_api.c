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

/******************************************************************************
 *
 *  This is the implementation of the API for B3DS subsystem of BTA,
 *  Bluetooth application layer for mobile phones.
 *
 ******************************************************************************/

#include "bt_target.h"

#include "bta_api.h"
#include "bta_sys.h"
#include "gki.h"
#include "bta_b3ds_api.h"
#include "bta_b3ds_int.h"
#include <string.h>
#include "bt_utils.h"

#if defined(MTK_B3DS_INCLUDED) && (MTK_B3DS_INCLUDED == TRUE)

static const tBTA_SYS_REG bta_b3ds_reg =
{
    bta_b3ds_hdl_event,
    BTA_B3dsDisable
};

void BTA_B3dsEnable(tBTA_B3DS_CBACK p_cback, UINT8 enable_legacy)
{
    tBTA_B3DS_API_ENABLE  *p_buf;
    APPL_TRACE_DEBUG("BTA_B3dsEnable");

    /* register with BTA system manager */
    bta_sys_register(BTA_ID_B3DS, &bta_b3ds_reg);

    if ((p_buf = (tBTA_B3DS_API_ENABLE *) GKI_getbuf(sizeof(tBTA_B3DS_API_ENABLE))) != NULL)
    {
        p_buf->hdr.event = BTA_B3DS_API_ENABLE_EVT;
        p_buf->p_cback = p_cback;
        p_buf->enable_legacy = enable_legacy;

        bta_sys_sendmsg(p_buf);
    }
}

void BTA_B3dsDisable(void)
{
    BT_HDR  *p_buf;
    APPL_TRACE_DEBUG("BTA_B3dsDisable");

    bta_sys_deregister(BTA_ID_B3DS);
    if ((p_buf = (BT_HDR *) GKI_getbuf(sizeof(BT_HDR))) != NULL)
    {
        p_buf->event = BTA_B3DS_API_DISABLE_EVT;
        bta_sys_sendmsg(p_buf);
    }
}

void BTA_B3dsSetLegacy(UINT8 enable_legacy)
{
    tBTA_B3DS_API_SET_LEGACY  *p_buf;
    APPL_TRACE_DEBUG("BTA_B3dsSetLegacy");

    if ((p_buf = (tBTA_B3DS_API_SET_LEGACY *) GKI_getbuf(sizeof(tBTA_B3DS_API_SET_LEGACY))) != NULL)
    {
        p_buf->hdr.event = BTA_B3DS_API_SET_LEGACY_EVT;
        p_buf->enable_legacy = enable_legacy;

        bta_sys_sendmsg(p_buf);
    }
}

void BTA_B3dsSetBroadcast(tBTA_B3DS_VEDIO_MODE vedio_mode, tBTA_B3DS_PERIOD period_mode, UINT32 panel_delay)
{
    tBTA_B3DS_API_SET_BROADCAST  *p_buf;
    APPL_TRACE_DEBUG("BTA_B3dsSetBroadcast");

    if ((p_buf = (tBTA_B3DS_API_SET_BROADCAST *) GKI_getbuf(sizeof(tBTA_B3DS_API_SET_BROADCAST))) != NULL)
    {
        p_buf->hdr.event = BTA_B3DS_API_SET_BROADCAST_EVT;
        p_buf->vedio_mode = vedio_mode;
        p_buf->period_mode = period_mode;
        p_buf->panel_delay = panel_delay;

        bta_sys_sendmsg(p_buf);
    }
}

void BTA_B3dsStartBroadcast(void)
{
    BT_HDR  *p_buf;
    APPL_TRACE_DEBUG("BTA_B3dsStartBroadcast");

    if ((p_buf = (BT_HDR *) GKI_getbuf(sizeof(BT_HDR))) != NULL)
    {
        p_buf->event = BTA_B3DS_API_START_BROADCAST_EVT;
        bta_sys_sendmsg(p_buf);
    }
}

void BTA_B3dsStopBroadcast(void)
{
    BT_HDR  *p_buf;
    APPL_TRACE_DEBUG("BTA_B3dsStopBroadcast");

    if ((p_buf = (BT_HDR *) GKI_getbuf(sizeof(BT_HDR))) != NULL)
    {
        p_buf->event = BTA_B3DS_API_STOP_BROADCAST_EVT;
        bta_sys_sendmsg(p_buf);
    }
}

void BTA_B3dsStartSyncTrain(UINT32 sync_train_to)
{
    tBTA_B3DS_API_START_SYNC_TRAIN  *p_buf;
    APPL_TRACE_DEBUG("BTA_B3dsStartSyncTrain");

    if ((p_buf = (tBTA_B3DS_API_START_SYNC_TRAIN *) GKI_getbuf(sizeof(tBTA_B3DS_API_START_SYNC_TRAIN))) != NULL)
    {
        p_buf->hdr.event = BTA_B3DS_API_START_SYNC_TRAIN_EVT;
        p_buf->sync_train_to = sync_train_to;

        bta_sys_sendmsg(p_buf);
    }
}

void BTA_B3dsStopSyncTrain(void)
{
    BT_HDR  *p_buf;
    APPL_TRACE_DEBUG("BTA_B3dsStopSyncTrain");

    if ((p_buf = (BT_HDR *) GKI_getbuf(sizeof(BT_HDR))) != NULL)
    {
        p_buf->event = BTA_B3DS_API_STOP_SYNC_TRAIN_EVT;
        bta_sys_sendmsg(p_buf);
    }
}
#endif

