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

#define LOG_TAG "bt_stack_config"

#include <assert.h>

#include "osi/include/future.h"
#include "stack_config.h"
#include "osi/include/log.h"

#if MTK_STACK_CONFIG == TRUE
#include <stdio.h>
#include <string.h>
#endif

const char *BTSNOOP_LOG_PATH_KEY = "BtSnoopFileName";
const char *BTSNOOP_TURNED_ON_KEY = "BtSnoopLogOutput";
const char *BTSNOOP_SHOULD_SAVE_LAST_KEY = "BtSnoopSaveLog";
const char *TRACE_CONFIG_ENABLED_KEY = "TraceConf";

static config_t *config;

#if (MTK_SSP_DEBUG_MODE_STACK_CONFIG == TRUE)
const char *FEATURE_SSP_DEBUG_MODE_ENABLED_KEY = "SspDebugModeConf";
#endif /* MTK_SSP_DEBUG_MODE_STACK_CONFIG */

#if MTK_STACK_CONFIG_BL == TRUE
const char *BLACKLIST_ADDR_KEY = "AddressBlacklist";
const char *BLACKLIST_NAME_KEY = "ExactNameBlacklist";
const char *BLACKLIST_PARTIAL_NAME_KEY = "PartialNameBlacklist";
#endif

#if MTK_STACK_CONFIG_LOG == TRUE
const char *BTLOG_FWLOG_HCI_CMD1 = "C1";
const char *BTLOG_FWLOG_HCI_CMD2 = "C2";
#endif /* MTK_STACK_CONFIG_LOG */

#if MTK_STACK_CONFIG == TRUE
const char *EXTFILE_OVERRIDE_TMPKEY = "OverrideConf"; /* easy to parse /sdcard/btsc, it's not a key in config */
const char *STACK_CONF_OVERRIDE_KEY = "MtkStackConfigOverride";

#if MTK_STACK_CONFIG_DEFAULT_OVERRIDE == TRUE
const char *BTDefaultConfOverrideFile = "bt_stack.conf.sqc";
#endif

static bool parse_override_cfg(void) {
  FILE *target_file = NULL;
  char str_ovconf_fpath[MTK_STACK_CONFIG_FPATH_LEN] = { '\0' };
  const char *p_redir_ovconf_fpath = NULL;
  char *p_fpath = NULL, prefix_fpath1[MTK_STACK_CONFIG_FPATH_LEN] = "/etc/bluetooth/";
  bool b_override = false;

  char str_redir_ov_fpath[MTK_STACK_CONFIG_FPATH_LEN] = { '\0' };
  /* MtkStackConfigOverride = /scard/btsc in bt_stack.conf */
  strlcpy(str_redir_ov_fpath, config_get_string(config, CONFIG_MTK_CONF_SECTION, STACK_CONF_OVERRIDE_KEY, ""), sizeof(str_redir_ov_fpath));

  LOG_INFO("%s M_BTCONF redir file is \"%s\"", __func__, str_redir_ov_fpath);

  p_redir_ovconf_fpath = str_redir_ov_fpath;

  target_file = fopen(p_redir_ovconf_fpath, "rt");
  if (!target_file) {
    LOG_INFO("%s M_BTCONF open redir-file %s fails!", __func__, p_redir_ovconf_fpath);

#if MTK_STACK_CONFIG_DEFAULT_OVERRIDE == TRUE
    LOG_INFO("%s M_BTCONF set the override default config: %s!", __func__, BTDefaultConfOverrideFile);
    strlcpy(str_ovconf_fpath, BTDefaultConfOverrideFile, sizeof(str_ovconf_fpath));
#else
    /* MTK_STACK_CONFIG_DEFAULT_OVERRIDE is not defined or MTK_STACK_CONFIG_DEFAULT_OVERRIDE == 0 */
    return false; /* Don't override config - keep it as default config of bluedroid */
#endif

  } else {
      fclose(target_file);

      config_t *redir_config = config_new(p_redir_ovconf_fpath);
      if (redir_config) {
          /* copy ov filepath from /scard/btsc */
          strlcpy(str_ovconf_fpath, config_get_string(redir_config, CONFIG_DEFAULT_SECTION, EXTFILE_OVERRIDE_TMPKEY, ""), sizeof(str_ovconf_fpath));

          config_free(redir_config);
      }
  }

  LOG_INFO("%s M_BTCONF OverrideConf= %s", __func__, str_ovconf_fpath);

  if (str_ovconf_fpath[0] != '\0') {
    FILE *test_file = NULL;

    if (!strcmp(str_ovconf_fpath, "bt_stack.conf.sqc") ||
        !strcmp(str_ovconf_fpath, "bt_stack.conf.debug")) {

      if ((strlen(str_ovconf_fpath) + strlen(prefix_fpath1)) <= (MTK_STACK_CONFIG_FPATH_LEN - 1))
        strcat(prefix_fpath1, str_ovconf_fpath);
      else {
        LOG_ERROR("%s M_BTCONF file/path \"prefix+overrideconf_fpath\" exceeds the size of array: %d", __func__, MTK_STACK_CONFIG_FPATH_LEN);
        return false;
      }

      test_file = fopen(prefix_fpath1, "rt");
      if (!test_file) {
        LOG_INFO("%s M_BTCONF open %s fails!", __func__, prefix_fpath1);
        return false;
      } else {

        fclose(test_file);
        p_fpath = prefix_fpath1;
      }
    } else {

      test_file = fopen(str_ovconf_fpath, "rt");
      if (!test_file) {
        LOG_INFO("%s M_BTCONF open %s fails!", __func__, str_ovconf_fpath);
        return false;
      } else {

        fclose(test_file);
        p_fpath = str_ovconf_fpath;
      }
    }
  }

  if (p_fpath) {
    LOG_INFO("%s M_BTCONF config_override file/path \"%s\"", __func__, p_fpath);
    b_override = config_override(config, p_fpath);
  } else
      LOG_INFO("%s M_BTCONF config_override file/path is NULL", __func__);

  return b_override;
}

#endif

// Module lifecycle functions

static future_t *init() {
  const char *path = "/etc/bluetooth/bt_stack.conf";
  assert(path != NULL);

  LOG_INFO("%s attempt to load stack conf from %s", __func__, path);

  config = config_new(path);
  if (!config) {
    LOG_INFO("%s file >%s< not found", __func__, path);
    return future_new_immediate(FUTURE_FAIL);
  }

#if MTK_STACK_CONFIG == TRUE
  config_dump(config); /* dump config before override */

  /* override stack config */
  if (parse_override_cfg()) {
    LOG_INFO("%s M_BTCONF Override file is deployed", __func__);

    config_dump(config); /* dump config after override */

  } else
      LOG_INFO("%s M_BTCONF Override file is not deployed", __func__);

#endif /* MTK_STACK_CONFIG == TRUE */

  return future_new_immediate(FUTURE_SUCCESS);
}

static future_t *clean_up() {
  config_free(config);
  return future_new_immediate(FUTURE_SUCCESS);
}

const module_t stack_config_module = {
  .name = STACK_CONFIG_MODULE,
  .init = init,
  .start_up = NULL,
  .shut_down = NULL,
  .clean_up = clean_up,
  .dependencies = {
    NULL
  }
};

// Interface functions

static const char *get_btsnoop_log_path(void) {
  return config_get_string(config, CONFIG_DEFAULT_SECTION, BTSNOOP_LOG_PATH_KEY, "/data/misc/bluedroid/btsnoop_hci.log");
}

static bool get_btsnoop_turned_on(void) {
  return config_get_bool(config, CONFIG_DEFAULT_SECTION, BTSNOOP_TURNED_ON_KEY, false);
}

static bool get_btsnoop_should_save_last(void) {
  return config_get_bool(config, CONFIG_DEFAULT_SECTION, BTSNOOP_SHOULD_SAVE_LAST_KEY, false);
}

static bool get_trace_config_enabled(void) {
  return config_get_bool(config, CONFIG_DEFAULT_SECTION, TRACE_CONFIG_ENABLED_KEY, false);
}

static config_t *get_all(void) {
  return config;
}

const stack_config_t interface = {
  get_btsnoop_log_path,
  get_btsnoop_turned_on,
  get_btsnoop_should_save_last,
  get_trace_config_enabled,
  get_all
};

const stack_config_t *stack_config_get_interface() {
  return &interface;
}

#if MTK_STACK_CONFIG_BL == TRUE
static const char *get_role_switch_addr_blacklist(void) {
  return config_get_string(config, CONFIG_MTK_ROLE_SWITCH_BLACKLIST_SECTION, BLACKLIST_ADDR_KEY, "");
}

static const char *get_role_switch_name_blacklist(void) {
  return config_get_string(config, CONFIG_MTK_ROLE_SWITCH_BLACKLIST_SECTION, BLACKLIST_NAME_KEY, "");
}

static const char *get_role_switch_partial_name_blacklist(void) {
  return config_get_string(config, CONFIG_MTK_ROLE_SWITCH_BLACKLIST_SECTION, BLACKLIST_PARTIAL_NAME_KEY, "");
}

static const char *get_sniff_subrating_addr_blacklist(void) {
  return config_get_string(config, CONFIG_MTK_SNIFF_SUBRATING_BLACKLIST_SECTION, BLACKLIST_ADDR_KEY, "");
}

static const char *get_sniff_subrating_name_blacklist(void) {
  return config_get_string(config, CONFIG_MTK_SNIFF_SUBRATING_BLACKLIST_SECTION, BLACKLIST_NAME_KEY, "");
}

static const char *get_sniff_subrating_partial_name_blacklist(void) {
  return config_get_string(config, CONFIG_MTK_SNIFF_SUBRATING_BLACKLIST_SECTION, BLACKLIST_PARTIAL_NAME_KEY, "");
}

static const char *get_hogp_conn_update_addr_blacklist(void) {
  return config_get_string(config, CONFIG_MTK_HOGP_CONN_UPDATE_BLACKLIST_SECTION, BLACKLIST_ADDR_KEY, NULL);
}

static const char *get_hogp_conn_update_name_blacklist(void) {
  return config_get_string(config, CONFIG_MTK_HOGP_CONN_UPDATE_BLACKLIST_SECTION, BLACKLIST_NAME_KEY, NULL);
}

static const char *get_hogp_conn_update_partial_name_blacklist(void) {
  return config_get_string(config, CONFIG_MTK_HOGP_CONN_UPDATE_BLACKLIST_SECTION, BLACKLIST_PARTIAL_NAME_KEY, NULL);
}

static const char *get_ble_conn_int_min_limit_reject_addr_blacklist(void) {
  return config_get_string(config, CONFIG_MTK_BLE_CONN_INT_MIN_LIMIT_REJECT_BLACKLIST_SECTION, BLACKLIST_ADDR_KEY, NULL);
}

static const char *get_ble_conn_int_min_limit_reject_name_blacklist(void) {
  return config_get_string(config, CONFIG_MTK_BLE_CONN_INT_MIN_LIMIT_REJECT_BLACKLIST_SECTION, BLACKLIST_NAME_KEY, NULL);
}

static const char *get_ble_conn_int_min_limit_reject_partial_name_blacklist(void) {
  return config_get_string(config, CONFIG_MTK_BLE_CONN_INT_MIN_LIMIT_REJECT_BLACKLIST_SECTION, BLACKLIST_PARTIAL_NAME_KEY, NULL);
}

static const char *get_ble_conn_int_min_limit_addr_blacklist(void) {
  return config_get_string(config, CONFIG_MTK_BLE_CONN_INT_MIN_LIMIT_BLACKLIST_SECTION, BLACKLIST_ADDR_KEY, NULL);
}

static const char *get_ble_conn_int_min_limit_name_blacklist(void) {
  return config_get_string(config, CONFIG_MTK_BLE_CONN_INT_MIN_LIMIT_BLACKLIST_SECTION, BLACKLIST_NAME_KEY, NULL);
}

static const char *get_ble_conn_int_min_limit_partial_name_blacklist(void) {
  return config_get_string(config, CONFIG_MTK_BLE_CONN_INT_MIN_LIMIT_BLACKLIST_SECTION, BLACKLIST_PARTIAL_NAME_KEY, NULL);
}

static const char *get_ble_conn_timeout_addr_blacklist(void) {
  return config_get_string(config, CONFIG_MTK_BLE_CONN_TIMEOUT_BLACKLIST_SECTION, BLACKLIST_ADDR_KEY, NULL);
}

static const char *get_ble_conn_timeout_name_blacklist(void) {
  return config_get_string(config, CONFIG_MTK_BLE_CONN_TIMEOUT_BLACKLIST_SECTION, BLACKLIST_NAME_KEY, NULL);
}

static const char *get_avrcp_15_back_to_13_addr_blacklist(void) {
  return config_get_string(config, CONFIG_MTK_AVRCP_15_BACK_TO_13_BLACKLIST_SECTION, BLACKLIST_ADDR_KEY, "");
}

static const char *get_avrcp_15_back_to_14_addr_blacklist(void) {
  return config_get_string(config, CONFIG_MTK_AVRCP_15_BACK_TO_14_BLACKLIST_SECTION, BLACKLIST_ADDR_KEY, "");
}

static const char *get_scms_t_addr_blacklist(void) {
  return config_get_string(config, CONFIG_MTK_SCMS_T_BLACKLIST_SECTION, BLACKLIST_ADDR_KEY, "");
}

static const char *get_hfp_15_eSCO_name_blacklist(void) {
  return config_get_string(config, CONFIG_MTK_HFP_15_ESCO_BLACKLIST_SECTION, BLACKLIST_NAME_KEY, "");
}

static const char *get_hfp_15_eSCO_addr_blacklist(void) {
  return config_get_string(config, CONFIG_MTK_HFP_15_ESCO_BLACKLIST_SECTION, BLACKLIST_ADDR_KEY, "");
}

static const char *get_secure_connections_partial_name_blacklist(void) {
  return config_get_string(config, CONFIG_MTK_SECURE_CONNECTIONS_BLACKLIST_SECTION, BLACKLIST_PARTIAL_NAME_KEY, "");
}

static const char *get_song_position_addr_blacklist(void) {
  return config_get_string(config, CONFIG_MTK_SONG_POSITION_BLACKLIST_SECTION, BLACKLIST_ADDR_KEY, "");
}

static const char *get_song_position_name_blacklist(void) {
  return config_get_string(config, CONFIG_MTK_SONG_POSITION_BLACKLIST_SECTION, BLACKLIST_NAME_KEY, "");
}

static const char *get_a2dp_delay_start_cmd_blacklist(void) {
  return config_get_string(config, CONFIG_MTK_A2DP_DELAY_START_CMD_BLACKLIST_SECTION, BLACKLIST_ADDR_KEY, "");
}

static const char *get_hfp_15_eSCO_mSBC_name_blacklist(void) {
  return config_get_string(config, CONFIG_MTK_HFP_15_ESCO_MSBC_BLACKLIST_SECTION, BLACKLIST_NAME_KEY, "");
}
static const char *get_hid_disable_sdp_name_blacklist(void) {
  return config_get_string(config, CONFIG_MTK_HID_DISABLE_SDP_BLACKLIST_SECTION, BLACKLIST_NAME_KEY, "");
}
static const char *get_hid_disable_sdp_addr_blacklist(void) {
  return config_get_string(config, CONFIG_MTK_HID_DISABLE_SDP_BLACKLIST_SECTION, BLACKLIST_ADDR_KEY, "");
}

static const char *get_hfp_delay_open_sco_blacklist(void) {
  return config_get_string(config, CONFIG_MTK_HFP_DELAY_OPEN_SCO_BLACKLIST_SECTION, BLACKLIST_ADDR_KEY, "");
}

static const char * get_av_connect_on_sdp_fail_carkit_blacklist(void) {
  return config_get_string(config, CONFIG_MTK_AV_CONNECT_ON_SDP_FAIL_BLACKLIST_SECTION, BLACKLIST_ADDR_KEY, NULL);
}

static const char *get_avrcp_release_key_addr_blacklist(void) {
  return config_get_string(config, CONFIG_MTK_AVRCP_RELEASE_KEY_SECTION, BLACKLIST_ADDR_KEY, "");
}

const stack_config_blacklist_t role_switch_blacklist_interface = {
  get_role_switch_addr_blacklist,
  get_role_switch_name_blacklist,
  get_role_switch_partial_name_blacklist
};

const stack_config_blacklist_t sniff_subrating_blacklist_interface = {
  get_sniff_subrating_addr_blacklist,
  get_sniff_subrating_name_blacklist,
  get_sniff_subrating_partial_name_blacklist
};

const stack_config_blacklist_t hogp_conn_update_blacklist_interface = {
  get_hogp_conn_update_addr_blacklist,
  get_hogp_conn_update_name_blacklist,
  get_hogp_conn_update_partial_name_blacklist
};

const stack_config_blacklist_t ble_conn_int_min_limit_reject_blacklist_interface = {
  get_ble_conn_int_min_limit_reject_addr_blacklist,
  get_ble_conn_int_min_limit_reject_name_blacklist,
  get_ble_conn_int_min_limit_reject_partial_name_blacklist
};

const stack_config_blacklist_t ble_conn_int_min_limit_blacklist_interface = {
  get_ble_conn_int_min_limit_addr_blacklist,
  get_ble_conn_int_min_limit_name_blacklist,
  get_ble_conn_int_min_limit_partial_name_blacklist
};

const stack_config_blacklist_t ble_conn_timeout_blacklist_interface = {
  get_ble_conn_timeout_addr_blacklist,
  get_ble_conn_timeout_name_blacklist,
  NULL
};

const stack_config_blacklist_t avrcp_15_back_to_13_blacklist_interface = {
  get_avrcp_15_back_to_13_addr_blacklist,
  NULL,
  NULL
};

const stack_config_blacklist_t avrcp_15_back_to_14_blacklist_interface = {
  get_avrcp_15_back_to_14_addr_blacklist,
  NULL,
  NULL
};

const stack_config_blacklist_t scms_t_blacklist_interface = {
  get_scms_t_addr_blacklist,
  NULL,
  NULL
};

const stack_config_blacklist_t hfp_15_eSCO_blacklist_interface = {
  get_hfp_15_eSCO_addr_blacklist,
  get_hfp_15_eSCO_name_blacklist,
  NULL
};

const stack_config_blacklist_t secure_connections_blacklist_interface = {
  NULL,
  NULL,
  get_secure_connections_partial_name_blacklist
};

const stack_config_blacklist_t song_position_blacklist_interface = {
  get_song_position_addr_blacklist,
  get_song_position_name_blacklist,
  NULL
};

const stack_config_blacklist_t a2dp_delay_start_cmd_blacklist_interface = {
  get_a2dp_delay_start_cmd_blacklist,
  NULL,
  NULL
};

const stack_config_blacklist_t hfp_15_eSCO_mSBC_blacklist_interface = {
  NULL,
  get_hfp_15_eSCO_mSBC_name_blacklist,
  NULL
};

const stack_config_blacklist_t hid_disable_sdp_blacklist_interface = {
  get_hid_disable_sdp_addr_blacklist,
  get_hid_disable_sdp_name_blacklist,
  NULL
};

const stack_config_blacklist_t hfp_delay_open_sco_blacklist_interface = {
  get_hfp_delay_open_sco_blacklist,
  NULL,
  NULL
};

const stack_config_blacklist_t av_connect_on_sdp_fail_blacklist_interface = {
  get_av_connect_on_sdp_fail_carkit_blacklist,
  NULL,
  NULL
};

const stack_config_blacklist_t avrcp_release_key_blacklist_interface = {
  get_avrcp_release_key_addr_blacklist,
  NULL,
  NULL
};

const stack_config_blacklist_t *stack_config_role_switch_blacklist_get_interface() {
  return &role_switch_blacklist_interface;
}

const stack_config_blacklist_t *stack_config_sniff_subrating_blacklist_get_interface() {
  return &sniff_subrating_blacklist_interface;
}

const stack_config_blacklist_t *stack_config_hogp_conn_update_blacklist_get_interface() {
  return &hogp_conn_update_blacklist_interface;
}

const stack_config_blacklist_t *stack_config_ble_conn_int_min_limit_reject_blacklist_get_interface() {
  return &ble_conn_int_min_limit_reject_blacklist_interface;
}

const stack_config_blacklist_t *stack_config_ble_conn_int_min_limit_blacklist_get_interface() {
  return &ble_conn_int_min_limit_blacklist_interface;
}

const stack_config_blacklist_t *stack_config_ble_conn_timeout_blacklist_get_interface() {
  return &ble_conn_timeout_blacklist_interface;
}

const stack_config_blacklist_t *stack_config_avrcp_15_back_to_13_blacklist_get_interface() {
  return &avrcp_15_back_to_13_blacklist_interface;
}

const stack_config_blacklist_t *stack_config_avrcp_15_back_to_14_blacklist_get_interface() {
  return &avrcp_15_back_to_14_blacklist_interface;
}

const stack_config_blacklist_t *stack_config_scms_t_blacklist_get_interface() {
  return &scms_t_blacklist_interface;
}

const stack_config_blacklist_t *stack_config_hfp_15_eSCO_blacklist_get_interface() {
  return &hfp_15_eSCO_blacklist_interface;
}

const stack_config_blacklist_t *stack_config_secure_connections_blacklist_interface() {
  return &secure_connections_blacklist_interface;
}

const stack_config_blacklist_t *stack_config_song_position_blacklist_get_interface() {
  return &song_position_blacklist_interface;
}

const stack_config_blacklist_t *stack_config_a2dp_delay_start_cmd_blacklist_get_interface() {
  return &a2dp_delay_start_cmd_blacklist_interface;
}

const stack_config_blacklist_t *stack_config_hfp_15_eSCO_MSBC_blacklist_get_interface() {
  return &hfp_15_eSCO_mSBC_blacklist_interface;
}

const stack_config_blacklist_t *stack_config_hid_disable_sdp_blacklist_get_interface() {
  return &hid_disable_sdp_blacklist_interface;
}

const stack_config_blacklist_t *stack_config_hfp_delay_open_sco_blacklist_get_interface() {
  return &hfp_delay_open_sco_blacklist_interface;
}

const stack_config_blacklist_t *stack_config_av_connect_on_sdp_fail_blacklist_get_interface() {
  return &av_connect_on_sdp_fail_blacklist_interface;
}

const stack_config_blacklist_t *stack_config_avrcp_release_key_blacklist_get_interface() {
  return &avrcp_release_key_blacklist_interface;
}

#endif /* MTK_STACK_CONFIG_BL == TRUE */

#if MTK_STACK_CONFIG_LOG == TRUE
uint8_t fwcfg_tx[MTK_STACK_CONFIG_NUM_OF_HEXLIST][MTK_STACK_CONFIG_NUM_OF_HEXITEMS];

static void init_fwlog_cmd_ary(void) {
  int fi;

  /* Initialize fwcfg_tx[fi][0], device/src/Controller.c will send hci commands depends on the value is not ZERO,
   * So the max. number of hci commands can be sent is 5 NUM_OF_EXTCMD
   */
  for (fi = 0 ; fi < MTK_STACK_CONFIG_NUM_OF_HEXLIST; fi++) {
    fwcfg_tx[fi][0] = 0;
  }
}

static bool parse_fwlog_pairs(config_t *pick_fwlog_conf) {
  bool b_check_hci_cmd_ready = false;

  if (!pick_fwlog_conf) {
    LOG_INFO("%s M_BTCONF invalid conf for fwlog key/vals", __func__);
    return false;

  } else {
    #define NUM_OF_OCTS_BF_PARAMETER 4
    #define IDX_OF_PARAMLEN_IN_OCTS 3
    /* C1	= 01 BE FC 01 05 */
    /* C2	= 01 5F FC 2A 50 01 09 00 00 00
              01 00 00 00
              00 00 00 00
              01 00 00 00
              01 00 00 00
              01 00 00 00
              01 00 00 00
              01 00 01 00
              03 38 00 00
              01 00 00 00

       param len of C1 = 0x01
       param len of C2 = 0x2A
       NUM_OF_OCTS_BF_PARAMETER 4 including len oct
     */

    unsigned int c1val[MTK_STACK_CONFIG_NUM_OF_HEXITEMS] = {'\0'};
    unsigned int c2val[MTK_STACK_CONFIG_NUM_OF_HEXITEMS] = {'\0'};
    const char* C1;
    const char *C2[MTK_STACK_CONFIG_NUM_OF_HEXROWITEMS] = { '\0' };
    int hexcidx = 0;
    char strkey[32] = { '\0' };

    C1 = config_get_string(pick_fwlog_conf, CONFIG_MTK_FWLOG_SECTION, BTLOG_FWLOG_HCI_CMD1, "");

    C2[0] = config_get_string(pick_fwlog_conf, CONFIG_MTK_FWLOG_SECTION, BTLOG_FWLOG_HCI_CMD2, "");
    for (hexcidx = 1; hexcidx < MTK_STACK_CONFIG_NUM_OF_HEXROWITEMS; hexcidx++) {
      sprintf(strkey, "%s%02d", BTLOG_FWLOG_HCI_CMD2, hexcidx);
      LOG_INFO("strkey=%s", strkey);
      C2[hexcidx] = config_get_string(pick_fwlog_conf, CONFIG_MTK_FWLOG_SECTION, strkey, '\0');
    }

    if (*C1) {
      int j, plen;
      int ret = sscanf(C1, "%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
                       c1val, c1val+1, c1val+2, c1val+3, c1val+4, c1val+5, c1val+6, c1val+7,
                       c1val+8, c1val+9, c1val+10, c1val+11, c1val+12, c1val+13, c1val+14, c1val+15);
      LOG_INFO("M_BTCONF scan %d hex", ret);
      for (j = 0; j < ret; j++) {
          LOG_INFO(" 0x%x", c1val[j]);
          fwcfg_tx[0][j] = (uint8_t)c1val[j];
      }

      plen = fwcfg_tx[0][IDX_OF_PARAMLEN_IN_OCTS];
      if (ret == (plen + NUM_OF_OCTS_BF_PARAMETER)) {
        LOG_INFO("M_BTCONF scanned param num matches the value (0x%02x) in command", plen);
        b_check_hci_cmd_ready = true;
      } else {
        LOG_INFO("M_BTCONF scanned param num (0x%02x) doesn't match the value (0x%02x) in command", ret - NUM_OF_OCTS_BF_PARAMETER, plen);
        b_check_hci_cmd_ready = false;
      }
    } else {
      LOG_INFO("M_BTCONF no \"C1\" section");
      b_check_hci_cmd_ready = false;
    }

    if (*C2[0] && b_check_hci_cmd_ready) {
      int j, plen, ret, cidx = 0, didx = 0;

      while (cidx < MTK_STACK_CONFIG_NUM_OF_HEXROWITEMS && C2[cidx]) {
        ret = sscanf(C2[cidx], "%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
                     c2val, c2val+1, c2val+2, c2val+3, c2val+4, c2val+5, c2val+6, c2val+7,
                     c2val+8, c2val+9, c2val+10, c2val+11, c2val+12, c2val+13, c2val+14, c2val+15);
        LOG_INFO("M_BTCONF scan %d hex", ret);
        if (ret > 0) {
          for (j = 0; j < ret; j++) {
            LOG_INFO(" 0x%x", c2val[j]);
            fwcfg_tx[1][didx] = (uint8_t)c2val[j];
            didx++;
          }
        }
        cidx++;
      }

      plen = fwcfg_tx[1][IDX_OF_PARAMLEN_IN_OCTS];
      LOG_INFO("M_BTCONF plen = 0x%x", fwcfg_tx[1][IDX_OF_PARAMLEN_IN_OCTS]);
      if (didx == (plen + NUM_OF_OCTS_BF_PARAMETER)) {
        LOG_INFO("M_BTCONF scanned param num matches the value (0x%02x) in command", plen);
        b_check_hci_cmd_ready = true;
      } else {
        LOG_INFO("M_BTCONF scanned param num (0x%02x) doesn't match the value (0x%02x) in command", didx - NUM_OF_OCTS_BF_PARAMETER, plen);
        b_check_hci_cmd_ready = false;
      }
    } else {
      if (!*C2[0]) {
        LOG_INFO("M_BTCONF no \"C2\" section");
        b_check_hci_cmd_ready = false;
      }
    }
    return b_check_hci_cmd_ready;
  }

  return false;
}

static bool get_fwlog_hci_pack_hexlists(void) {

  int ret = false;
  init_fwlog_cmd_ary();

  if (config_get_bool(config, CONFIG_DEFAULT_SECTION, BTSNOOP_TURNED_ON_KEY, false)) {
    LOG_INFO("M_BTCONF attempt to parse fwlog hci cmds");
    if (false == parse_fwlog_pairs(config)) {
      /* empty hci commands */
      LOG_INFO("M_BTCONF empty hci cmds");
      init_fwlog_cmd_ary();
    } else
      ret = true;

  } else {
    LOG_INFO("%s M_BTCONF %s=false => Don't parse fwlog hci cmds", __func__, BTSNOOP_TURNED_ON_KEY);
  }

  return ret;
}

static const uint8_t *get_fwlog_hci_whole_hexlist(void) {
  return (uint8_t *)fwcfg_tx;
}

const stack_config_pack_hexlist_t fwlog_hci_hexlist_interface = {
  get_fwlog_hci_pack_hexlists,
  get_fwlog_hci_whole_hexlist,
};

const stack_config_pack_hexlist_t *stack_config_fwlog_hexs_get_interface() {
  return &fwlog_hci_hexlist_interface;
}

#endif /* MTK_STACK_CONFIG_LOG == TRUE */

#if (MTK_SSP_DEBUG_MODE_STACK_CONFIG == TRUE)
static bool get_feature_ssp_debug_mode_enabled(void);

stack_config_hci_opt_packet_t stack_config_hci_opt_packet_interface = {
  get_feature_ssp_debug_mode_enabled,
};

static bool get_feature_ssp_debug_mode_enabled(void){
  bool is_enabled =
    config_get_bool(config, CONFIG_MTK_HCI_OPT_PACKETS_SECTION, FEATURE_SSP_DEBUG_MODE_ENABLED_KEY, false);
  LOG_INFO("%s: is_enabled=%d", __func__, is_enabled);
  return is_enabled;
}

const stack_config_hci_opt_packet_t *stack_config_hci_opt_packet_get_interface(void){
  return &stack_config_hci_opt_packet_interface;
}
#endif /* MTK_SSP_DEBUG_MODE_STACK_CONFIG */

