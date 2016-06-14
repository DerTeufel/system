/******************************************************************************
 *
 *  Copyright (C) 2014 Google, Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

#pragma once

#include <stdbool.h>

#include "config.h"
#include "module.h"

#ifdef HAS_MDROID_BUILDCFG
#include "mdroid_buildcfg.h"
#endif

static const char STACK_CONFIG_MODULE[] = "stack_config_module";

typedef struct {
  const char *(*get_btsnoop_log_path)(void);
  bool (*get_btsnoop_turned_on)(void);
  bool (*get_btsnoop_should_save_last)(void);
  bool (*get_trace_config_enabled)(void);
  config_t *(*get_all)(void);
} stack_config_t;

const stack_config_t *stack_config_get_interface();

#if MTK_STACK_CONFIG_BL == TRUE
typedef struct {
  const char *(*get_addr_blacklist)(void);
  const char *(*get_name_blacklist)(void);
  const char *(*get_partial_name_blacklist)(void);
} stack_config_blacklist_t;

const stack_config_blacklist_t *stack_config_role_switch_blacklist_get_interface();
const stack_config_blacklist_t *stack_config_sniff_subrating_blacklist_get_interface();
const stack_config_blacklist_t *stack_config_hogp_conn_update_blacklist_get_interface();
const stack_config_blacklist_t *stack_config_ble_conn_int_min_limit_reject_blacklist_get_interface();
const stack_config_blacklist_t *stack_config_ble_conn_int_min_limit_blacklist_get_interface();
const stack_config_blacklist_t *stack_config_ble_conn_timeout_blacklist_get_interface();
const stack_config_blacklist_t *stack_config_avrcp_15_back_to_13_blacklist_get_interface();
const stack_config_blacklist_t *stack_config_avrcp_15_back_to_14_blacklist_get_interface();
const stack_config_blacklist_t *stack_config_scms_t_blacklist_get_interface();
const stack_config_blacklist_t *stack_config_hfp_15_eSCO_blacklist_get_interface();
const stack_config_blacklist_t *stack_config_secure_connections_blacklist_interface();
const stack_config_blacklist_t *stack_config_song_position_blacklist_get_interface();
const stack_config_blacklist_t *stack_config_a2dp_delay_start_cmd_blacklist_get_interface();
const stack_config_blacklist_t *stack_config_hfp_15_eSCO_MSBC_blacklist_get_interface();
const stack_config_blacklist_t *stack_config_hid_disable_sdp_blacklist_get_interface();
const stack_config_blacklist_t *stack_config_av_connect_on_sdp_fail_blacklist_get_interface();
const stack_config_blacklist_t *stack_config_avrcp_release_key_blacklist_get_interface();
const stack_config_blacklist_t *stack_config_hfp_delay_open_sco_blacklist_get_interface();
#endif

#if MTK_STACK_CONFIG_LOG == TRUE
typedef struct {
  bool (*get_pack_hexlists)(void);
  const uint8_t *(*get_whole_hexlists)(void);
} stack_config_pack_hexlist_t;

const stack_config_pack_hexlist_t *stack_config_fwlog_hexs_get_interface();
#endif

#if (MTK_SSP_DEBUG_MODE_STACK_CONFIG == TRUE)
typedef struct{
    bool (*get_hci_opt_packet_ssp_debug_mode_enabled)(void);
}stack_config_hci_opt_packet_t;

const stack_config_hci_opt_packet_t *stack_config_hci_opt_packet_get_interface(void);
#endif/* MTK_SSP_DEBUG_MODE_STACK_CONFIG */

