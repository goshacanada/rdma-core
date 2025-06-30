---
date: 2024-12-30
footer: cxidv
header: "CXI Direct Verbs Manual"
layout: page
license: 'Licensed under the BSD license (FreeBSD Variant) - see COPYING.md'
section: 7
title: CXIDV
---

# NAME

cxidv - CXI Direct Verbs

# SYNOPSIS

```c
#include <infiniband/cxidv.h>
```

# DESCRIPTION

CXI Direct Verbs (CXIDV) is a vendor-specific extension to the InfiniBand
verbs API that provides access to CXI-specific hardware features and
capabilities.

The CXI Direct Verbs library provides three main vendor-specific methods:

**Method 1 - Device Information Query**
: Query CXI-specific device information including NIC address, PID granule,
  PID count, PID bits, and minimum free shift parameters.

**Method 2 - Memory Region Information Query**
: Query CXI-specific memory region attributes including MD handle, IOVA,
  length, and access flags.

**Method 3 - Queue Pair Information Query**
: Query CXI-specific queue pair information including TXQ handle, TGQ handle,
  command queue handle, event queue handle, and state.

# FUNCTIONS

**cxidv_query_device**(3)
: Query CXI device attributes

**cxidv_method1**(3)
: Execute CXI Method 1 - Device information query

**cxidv_method2**(3)
: Execute CXI Method 2 - Memory region information query

**cxidv_method3**(3)
: Execute CXI Method 3 - Queue pair information query

**cxidv_query_mr**(3)
: Alias for cxidv_method2 - Query memory region information

**cxidv_query_qp**(3)
: Alias for cxidv_method3 - Query queue pair information

# DEVICE CAPABILITIES

CXI devices support various capabilities that can be queried through the
device attributes:

**CXIDV_DEVICE_CAP_ATOMIC_OPS**
: Atomic operations support

**CXIDV_DEVICE_CAP_RDMA_READ**
: RDMA read operations support

**CXIDV_DEVICE_CAP_RDMA_WRITE**
: RDMA write operations support

**CXIDV_DEVICE_CAP_MULTICAST**
: Multicast support

**CXIDV_DEVICE_CAP_TRIGGERED_OPS**
: Triggered operations support

**CXIDV_DEVICE_CAP_RESTRICTED_MEMBERS**
: Restricted members support

# EXAMPLE

```c
#include <infiniband/verbs.h>
#include <infiniband/cxidv.h>

int main() {
    struct ibv_device **dev_list;
    struct ibv_context *context;
    struct cxidv_method1_attr method1_attr;
    int ret;

    /* Get device list and open context */
    dev_list = ibv_get_device_list(NULL);
    context = ibv_open_device(dev_list[0]);

    /* Check if CXI Direct Verbs is supported */
    if (!cxidv_is_supported(context)) {
        fprintf(stderr, "CXI Direct Verbs not supported\n");
        return 1;
    }

    /* Query device information using Method 1 */
    ret = cxidv_method1(context, &method1_attr, sizeof(method1_attr));
    if (ret == 0) {
        printf("NIC Address: 0x%x\n", method1_attr.nic_addr);
        printf("PID Granule: %u\n", method1_attr.pid_granule);
    }

    ibv_close_device(context);
    ibv_free_device_list(dev_list);
    return 0;
}
```

# SEE ALSO

**ibv_open_device**(3), **ibv_query_device**(3), **ibv_reg_mr**(3), **ibv_create_qp**(3)

# AUTHORS

Hewlett Packard Enterprise Development LP
