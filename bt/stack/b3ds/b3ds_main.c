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

#include "bt_target.h"

#if defined(MTK_B3DS_INCLUDED) && (MTK_B3DS_INCLUDED == TRUE)

#include <string.h>
#include "gki.h"
#include "bt_types.h"
#include "bt_utils.h"
#include "b3ds_api.h"
#include "b3ds_int.h"
#include "sdp_api.h"
#include "sdpdefs.h"
#include "l2c_api.h"
#include "hcidefs.h"
#include "btm_api.h"
#include "bta_sys.h"

tB3DS_CB  b3ds_cb;

#define B3DS_FRAME_PERIOD_CALCULATE_FRAME 32
#define B3DS_FRAME_PERIOD_CALCULATE_CYCLE ( B3DS_FRAME_PERIOD_CALCULATE_FRAME * (B3DS_SET_CLK_NUM_CAP_TO_FILTER+1) )

void b3ds_set_csb_data(UINT32 clock, UINT16 offset);

UINT32 b3ds_register_with_sdp (UINT16 uuid, char *p_name)
{
    UINT32  sdp_handle;
    UINT16  browse_list = UUID_SERVCLASS_PUBLIC_BROWSE_GROUP;
    tSDP_PROTOCOL_ELEM  protoList;
    B3DS_TRACE_DEBUG("b3ds_register_with_sdp");

    /* Create a record */
    sdp_handle = SDP_CreateRecord ();

    if (sdp_handle == 0)
    {
        B3DS_TRACE_ERROR ("b3ds_register_with_sdp: could not create SDP record");
        return 0;
    }
    APPL_TRACE_DEBUG("b3ds_register_with_sdp handle: %x", sdp_handle);

    /* Service Class ID List */
    SDP_AddServiceClassIdList (sdp_handle, 1, &uuid);

    /* For Service Search */
    protoList.protocol_uuid = UUID_PROTOCOL_L2CAP;
    protoList.num_params = 0;
    SDP_AddProtocolList(sdp_handle, 1, &protoList);

    /* Service Name */
    SDP_AddAttribute (sdp_handle, ATTR_ID_SERVICE_NAME, TEXT_STR_DESC_TYPE,
                        (UINT8) (strlen(p_name) + 1), (UINT8 *)p_name);

    /* Profile descriptor list */
    SDP_AddProfileDescriptorList (sdp_handle, UUID_SERVCLASS_3D_SYNCHRONIZATION, B3DS_PROFILE_VERSION);

    /* Make the service browsable */
    SDP_AddUuidSequence (sdp_handle,  ATTR_ID_BROWSE_GROUP_LIST, 1, &browse_list);

    bta_sys_add_uuid(uuid);

    return sdp_handle;
}

void b3ds_proc_cls_data_cb(BD_ADDR bd_addr, BT_HDR *p_buf)
{
    UINT8   *p = (UINT8 *)(p_buf + 1) + p_buf->offset;

    B3DS_TRACE_DEBUG( "b3ds_proc_cls_data_cb: Received command with code 0x%02x", (UINT8) *p);

    switch ((UINT8) *p)
    {
        case B3DS_COMM_CHNL_OPCODE_CON_ANNO_MSG:
            B3DS_TRACE_DEBUG( "b3ds_proc_cls_data_cb: len %d, option %d, battery %d", p_buf->len, (UINT8) *(p+1), (UINT8) *(p+2));
            b3ds_cb.b3ds_con_anno_msg_cb(bd_addr, (UINT8) *(p+1), (UINT8) *(p+2));
            break;

        default:
            B3DS_TRACE_WARNING("b3ds_proc_cls_data_cb: Undefined Opcode: %0x02x\n", (UINT8) *p);
            break;
    }

    GKI_freebuf (p_buf);
}

void b3ds_proc_cls_discover_cb (BD_ADDR bd_addr, UINT8 type , UINT32 extend)
{
    B3DS_TRACE_DEBUG("b3ds_proc_cls_discover_cb");
}

void b3ds_proc_cls_congestion_status_cb(BD_ADDR bd_addr, BOOLEAN congested)
{
    B3DS_TRACE_DEBUG("b3ds_proc_cls_congestion_status_cb");
}

UINT32 b3ds_register_with_l2cap()
{
    tL2CAP_UCD_CB_INFO cb_info;

    if ( !BTM_SetSecurityLevel(FALSE, MTK_B3DS_SERVICE_NAME, BTM_SEC_SERVICE_3D_SYNCHRONIZATION, MTK_B3DS_SECURITY_LEVEL, BT_PSM_B3DS, 0, 0))
    {
        B3DS_TRACE_ERROR("b3ds_register_with_l2cap: security register failed");
        return FALSE;
    }

    cb_info.pL2CA_UCD_Discover_Cb = b3ds_proc_cls_discover_cb;
    cb_info.pL2CA_UCD_Data_Cb = b3ds_proc_cls_data_cb;
    cb_info.pL2CA_UCD_Congestion_Status_Cb = b3ds_proc_cls_congestion_status_cb;

    L2CA_UcdRegister(BT_PSM_B3DS, &cb_info);
    return TRUE;
}

void b3ds_process_timeout(TIMER_LIST_ENT *p_tle)
{
    B3DS_TRACE_DEBUG("b3ds_process_timeout");

    if(p_tle->event == BTU_TTYPE_B3DS_SET_2D)
    {
        p_tle->event = 0;
        b3ds_cb.broadcast.period_mode = B3DS_PERIOD_2D_MODE;
        b3ds_set_csb_data(0, 0);
    }
}

void b3ds_set_csb_data(UINT32 clock, UINT16 offset)
{
    UINT16 pre_frame_sync_period;
    B3DS_TRACE_DEBUG("b3ds_set_csb_data");

    if (b3ds_cb.broadcast.period_mode == B3DS_PERIOD_2D_MODE)
    {
        B3DS_TRACE_DEBUG("b3ds_set_csb_data: B3DS_PERIOD_2D_MODE");
        b3ds_cb.broadcast.msg.frame_sync_instant = 0;
        b3ds_cb.broadcast.msg.frame_sync_instant_phase = 0;
    }
    else if(clock)
    {
        offset += b3ds_cb.broadcast.delay;

        // Check bit 0
        if(clock & 1)
            offset += 312;

        if(offset  < 625)
        {
            // Byte 0 bit 0 = 3DD Vative Bluetooth Clock Bit[0]
            b3ds_cb.broadcast.msg.frame_sync_instant = clock >> 1;
            b3ds_cb.broadcast.msg.frame_sync_instant_phase = offset;
        }
        else
        {
            b3ds_cb.broadcast.msg.frame_sync_instant = (clock >> 1) + offset / 625;
            b3ds_cb.broadcast.msg.frame_sync_instant_phase = offset % 625;
        }
    }

    // Set video mode
    if(b3ds_cb.broadcast.vedio_mode)
        b3ds_cb.broadcast.msg.frame_sync_instant |= 0x80000000;

    b3ds_cb.broadcast.msg.left_open_offset = b3ds_cb.broadcast.shutter_offset[b3ds_cb.broadcast.period_mode].left_open;
    b3ds_cb.broadcast.msg.left_close_offset = b3ds_cb.broadcast.shutter_offset[b3ds_cb.broadcast.period_mode].left_close;
    b3ds_cb.broadcast.msg.right_open_offset = b3ds_cb.broadcast.shutter_offset[b3ds_cb.broadcast.period_mode].right_open;
    b3ds_cb.broadcast.msg.right_close_offset = b3ds_cb.broadcast.shutter_offset[b3ds_cb.broadcast.period_mode].right_close;
    pre_frame_sync_period = b3ds_cb.broadcast.msg.frame_sync_period;
    b3ds_cb.broadcast.msg.frame_sync_period = b3ds_cb.broadcast.cal_frame_sync_period[b3ds_cb.broadcast.period_mode].period;
    b3ds_cb.broadcast.msg.frame_sync_period_fraction = b3ds_cb.broadcast.cal_frame_sync_period[b3ds_cb.broadcast.period_mode].period_fraction;

    // 2D or period changed
    if(b3ds_cb.broadcast.period_mode == B3DS_PERIOD_2D_MODE || b3ds_cb.broadcast.msg.frame_sync_period!=pre_frame_sync_period)
    {
        B3DS_TRACE_DEBUG("b3ds_set_csb_data: period changed");
        BTM_SetCsbData(B3DS_LT_ADDR, B3DS_CSB_DATA_NO_FRAGMENTATION, 17, (UINT8 *)&b3ds_cb.broadcast.msg);
    };

}

void b3ds_cal_frame_period(UINT32 synInstant, UINT16 synPhase)
{
    static UINT16 frameCount = 0;
    static UINT32 synInstantValue0 = 0;
    // static UINT16 synPhaseValue0 = 0;
    UINT32 durationInBtNativeClock;
    UINT32 durationInMSec;
    UINT16 clockOffset;
    UINT16 avgFramePeriod;
    UINT8 i;

    B3DS_TRACE_DEBUG("b3ds_cal_avg_frame_period: frameCount %d", frameCount);

    if (frameCount == 0)
    {
        synInstantValue0 = synInstant;
        // synPhaseValue0 = synPhase;  // Not Used
    }
    else if (frameCount == B3DS_FRAME_PERIOD_CALCULATE_FRAME)
    {
        if (synInstantValue0 > synInstant)
            durationInBtNativeClock = (0x0FFFFFFF - synInstantValue0 + synInstant);
        else
            durationInBtNativeClock = synInstant - synInstantValue0;

        durationInMSec = durationInBtNativeClock*3125/10; //  + (synPhase - synPhaseValue0)>>8;

        avgFramePeriod = durationInMSec / B3DS_FRAME_PERIOD_CALCULATE_CYCLE;

        for(i=B3DS_PERIOD_MAX;i!=B3DS_PERIOD_2D_MODE;i--)
            if(avgFramePeriod < b3ds_cal_frame_sync_period[i].max && avgFramePeriod > b3ds_cal_frame_sync_period[i].min)
                break;

        // Not matched but valid
        if(i == B3DS_PERIOD_2D_MODE && avgFramePeriod < B3DS_PERIOD_3D_24_HZ)
        {
            b3ds_cb.broadcast.period_mode = B3DS_PERIOD_DYNAMIC_CALCULATED;
            b3ds_cb.broadcast.cal_frame_sync_period[B3DS_PERIOD_DYNAMIC_CALCULATED].period = avgFramePeriod;
            b3ds_cb.broadcast.cal_frame_sync_period[B3DS_PERIOD_DYNAMIC_CALCULATED].period_fraction = 0;
            b3ds_cb.broadcast.shutter_offset[B3DS_PERIOD_DYNAMIC_CALCULATED].left_open = 2;
            b3ds_cb.broadcast.shutter_offset[B3DS_PERIOD_DYNAMIC_CALCULATED].left_close = (avgFramePeriod>>1)-1;
            b3ds_cb.broadcast.shutter_offset[B3DS_PERIOD_DYNAMIC_CALCULATED].right_open = (avgFramePeriod>>1)+1;
            b3ds_cb.broadcast.shutter_offset[B3DS_PERIOD_DYNAMIC_CALCULATED].right_close = avgFramePeriod-2;
        }
        // Matched or invalid
        else
            b3ds_cb.broadcast.period_mode = i;

        B3DS_TRACE_DEBUG("b3ds_cal_avg_frame_period: period_mode %d, avgFramePeriod %d", b3ds_cb.broadcast.period_mode, avgFramePeriod);

        // Reset
        frameCount = 0;
        synInstantValue0 = synInstant;
        // synPhaseValue0 = synPhase;
    }
    frameCount++;
}

#endif

