# CXI Provider for rdma-core

This directory contains the CXI (Cassini) InfiniBand provider for rdma-core, which provides user-space access to CXI network devices.

## Overview

The CXI provider implements the InfiniBand verbs API for CXI devices, enabling applications to use standard RDMA programming interfaces while accessing CXI-specific hardware features through vendor-specific extensions.

## Features

### Standard InfiniBand Verbs Support
- **Device Management**: Query device attributes and capabilities
- **Protection Domains**: Allocate and manage protection domains
- **Memory Registration**: Register and deregister memory regions
- **Completion Queues**: Create and manage completion queues
- **Queue Pairs**: Create and manage queue pairs for communication
- **Address Handles**: Create and manage address handles
- **Work Requests**: Post send and receive work requests

### CXI Direct Verbs Extensions
The provider includes vendor-specific extensions through CXI Direct Verbs (CXIDV):

1. **Method 1 - Device Information Query**
   - Query CXI-specific device parameters
   - Returns NIC address, PID granule, PID count, PID bits, min_free_shift

2. **Method 2 - Memory Region Information Query**
   - Query CXI-specific memory region attributes
   - Returns MD handle, IOVA, length, access flags

3. **Method 3 - Queue Pair Information Query**
   - Query CXI-specific queue pair information
   - Returns TXQ handle, TGQ handle, command queue handle, event queue handle, state

## Files

### Core Implementation
- **cxi.c** - Main provider implementation and device matching
- **verbs.c** - InfiniBand verbs operations and CXI Direct Verbs implementation
- **cxi.h** - Internal provider structures and definitions
- **verbs.h** - Function prototypes for verbs operations

### Public Headers
- **cxidv.h** - CXI Direct Verbs public API
- **cxi-abi.h** - Kernel-userspace ABI definitions

### Build System
- **CMakeLists.txt** - CMake build configuration
- **libcxi.map** - Symbol export map for shared library

### Documentation
- **man/** - Manual pages for CXI Direct Verbs functions
- **README.md** - This file

### Testing
- **test_cxi.c** - Simple test program demonstrating CXI provider usage

## Building

The CXI provider is built as part of the rdma-core build system:

```bash
# Configure and build rdma-core with CXI provider
mkdir build
cd build
cmake .. -DENABLE_STATIC=1
make
```

## Usage

### Basic InfiniBand Verbs

```c
#include <infiniband/verbs.h>

struct ibv_device **dev_list;
struct ibv_context *context;

// Get device list and open CXI device
dev_list = ibv_get_device_list(NULL);
context = ibv_open_device(dev_list[0]);

// Use standard verbs operations
struct ibv_pd *pd = ibv_alloc_pd(context);
struct ibv_mr *mr = ibv_reg_mr(pd, buffer, size, IBV_ACCESS_LOCAL_WRITE);
// ... etc
```

### CXI Direct Verbs Extensions

```c
#include <infiniband/verbs.h>
#include <infiniband/cxidv.h>

// Check if CXI Direct Verbs is supported
if (cxidv_is_supported(context)) {
    // Query device information using Method 1
    struct cxidv_method1_attr method1_attr;
    int ret = cxidv_method1(context, &method1_attr, sizeof(method1_attr));
    if (ret == 0) {
        printf("NIC Address: 0x%x\n", method1_attr.nic_addr);
        printf("PID Granule: %u\n", method1_attr.pid_granule);
    }

    // Query memory region information using Method 2
    struct cxidv_method2_attr method2_attr;
    ret = cxidv_method2(mr, &method2_attr, sizeof(method2_attr));
    if (ret == 0) {
        printf("MD Handle: 0x%x\n", method2_attr.md_handle);
    }

    // Query queue pair information using Method 3
    struct cxidv_method3_attr method3_attr;
    ret = cxidv_method3(qp, &method3_attr, sizeof(method3_attr));
    if (ret == 0) {
        printf("TXQ Handle: 0x%x\n", method3_attr.txq_handle);
    }
}
```

## Device Capabilities

CXI devices support various capabilities that can be queried:

- **CXIDV_DEVICE_CAP_ATOMIC_OPS** - Atomic operations
- **CXIDV_DEVICE_CAP_RDMA_READ** - RDMA read operations
- **CXIDV_DEVICE_CAP_RDMA_WRITE** - RDMA write operations
- **CXIDV_DEVICE_CAP_MULTICAST** - Multicast support
- **CXIDV_DEVICE_CAP_TRIGGERED_OPS** - Triggered operations
- **CXIDV_DEVICE_CAP_RESTRICTED_MEMBERS** - Restricted members

## Testing

A simple test program is provided to verify the provider functionality:

```bash
# Build the test program
gcc -o test_cxi test_cxi.c -libverbs -lcxi

# Run the test (requires CXI hardware)
./test_cxi
```

The test program will:
1. Enumerate InfiniBand devices and look for CXI devices
2. Query standard and CXI-specific device attributes
3. Test memory region registration and CXI Method 2
4. Test queue pair creation and CXI Method 3

## Kernel Driver Integration

This provider works with the CXI InfiniBand kernel driver located at:
- **drivers/infiniband/hw/cxi/** in the Linux kernel tree

The kernel driver must be loaded and CXI devices must be present for the provider to function.

## API Documentation

Detailed API documentation is available in the manual pages:
- **cxidv(7)** - Overview of CXI Direct Verbs
- **cxidv_method1(3)** - Device information query
- **cxidv_method2(3)** - Memory region information query  
- **cxidv_method3(3)** - Queue pair information query

## License

This provider is dual-licensed under GPL-2.0 OR BSD-2-Clause to match the rdma-core licensing model.

## Authors

Hewlett Packard Enterprise Development LP
