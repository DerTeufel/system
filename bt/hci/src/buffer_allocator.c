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

#include <assert.h>

#include "buffer_allocator.h"
#include "gki.h"

#include "bt_vendor_lib.h"
#include "osi/include/log.h"
#include "vendor.h"

// TODO(zachoverflow): move the assertion into GKI_getbuf in the future
static void *buffer_alloc(size_t size) {
#if MTK_FWLOG_TRIG_COREDUMP == TRUE
  if ((int)size > GKI_MAX_BUF_SIZE) {
    static int b_trig_coredump = false;
    int stack_trigger_reason;

    LOG_INFO("%s Stack triggers firmware coredump - size(%d) > GKI_MAX_BUF_SIZE", __func__, size);
    stack_trigger_reason = 31; /* 31 for host assert */
    if (0 == vendor_get_interface()->send_command(VENDOR_SET_FW_ASSERT, &stack_trigger_reason)) {
      LOG_INFO("%s Act FW Coredump Success!", __func__);
      b_trig_coredump = true;
    } else {
      LOG_WARN("%s Act FW Coredump Fails!", __func__);
      assert(size <= GKI_MAX_BUF_SIZE);
    }
  }
#else
  assert(size <= GKI_MAX_BUF_SIZE);
#endif

  return GKI_getbuf((uint16_t)size);
}

static const allocator_t interface = {
  buffer_alloc,
  GKI_freebuf
};

const allocator_t *buffer_allocator_get_interface() {
  return &interface;
}
