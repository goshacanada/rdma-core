---
date: 2024-12-30
footer: cxidv
header: "CXI Direct Verbs Manual"
layout: page
license: 'Licensed under the BSD license (FreeBSD Variant) - see COPYING.md'
section: 3
title: CXIDV_METHOD1
---

# NAME

cxidv_method1 - Execute CXI Method 1 to query device information

# SYNOPSIS

```c
#include <infiniband/cxidv.h>

int cxidv_method1(struct ibv_context *context,
                  struct cxidv_method1_attr *attr,
                  uint32_t inlen);
```

# DESCRIPTION

**cxidv_method1**() executes CXI vendor-specific Method 1 to query
CXI-specific device information.

# ARGUMENTS

*context*
: InfiniBand device context

*attr*
: Pointer to cxidv_method1_attr structure to be filled with device information

*inlen*
: Size of the attr structure

# RETURN VALUE

**cxidv_method1**() returns 0 on success, or the value of errno on failure.

# STRUCTURE

```c
struct cxidv_method1_attr {
    uint64_t comp_mask;
    uint32_t nic_addr;
    uint32_t pid_granule;
    uint32_t pid_count;
    uint32_t pid_bits;
    uint32_t min_free_shift;
    uint8_t reserved[4];
};
```

*comp_mask*
: Compatibility mask for future extensions

*nic_addr*
: CXI NIC address

*pid_granule*
: PID granule size

*pid_count*
: Number of PIDs available

*pid_bits*
: Number of PID bits

*min_free_shift*
: Minimum free shift parameter

# EXAMPLE

```c
struct cxidv_method1_attr attr;
int ret;

ret = cxidv_method1(context, &attr, sizeof(attr));
if (ret == 0) {
    printf("NIC Address: 0x%x\n", attr.nic_addr);
    printf("PID Granule: %u\n", attr.pid_granule);
    printf("PID Count: %u\n", attr.pid_count);
}
```

# SEE ALSO

**cxidv**(7), **cxidv_method2**(3), **cxidv_method3**(3), **cxidv_query_device**(3)

# AUTHORS

Hewlett Packard Enterprise Development LP
