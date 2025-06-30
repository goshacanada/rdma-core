# CXI Provider Structure

This document describes the complete structure of the CXI provider for rdma-core.

## Directory Structure

```
providers/cxi/
├── CMakeLists.txt          # CMake build configuration
├── Makefile               # Standalone build for testing
├── README.md              # Provider documentation
├── STRUCTURE.md           # This file
├── libcxi.map             # Symbol export map
├── cxi-abi.h              # Kernel-userspace ABI definitions
├── cxi.h                  # Internal provider structures
├── cxi.c                  # Main provider implementation
├── verbs.h                # Verbs function prototypes
├── verbs.c                # Verbs implementation + CXI Direct Verbs
├── cxidv.h                # Public CXI Direct Verbs API
├── test_cxi.c             # Test program
└── man/                   # Manual pages
    ├── CMakeLists.txt
    ├── cxidv.7.md
    ├── cxidv_method1.3.md
    └── ...
```

## Key Components

### 1. Provider Registration (`cxi.c`)

- **Device Matching**: Matches CXI PCI devices (HPE 0x1590:0x0371, Cray 0x17db:0x0501)
- **Context Allocation**: Creates CXI context with vendor-specific attributes
- **Provider Operations**: Registers standard verbs operations

### 2. Verbs Implementation (`verbs.c`)

#### Standard InfiniBand Verbs
- Device and port queries
- Protection domain allocation/deallocation
- Memory region registration/deregistration
- Completion queue creation/destruction/polling
- Queue pair creation/destruction/modification
- Address handle creation/destruction
- Work request posting (send/receive)

#### CXI Direct Verbs Extensions
- **cxidv_method1()**: Device information query via CXI_IB_METHOD_1
- **cxidv_method2()**: Memory region query via CXI_IB_METHOD_2
- **cxidv_method3()**: Queue pair query via CXI_IB_METHOD_3
- **cxidv_query_device()**: High-level device attribute query

### 3. Data Structures (`cxi.h`)

```c
struct cxi_context {
    struct verbs_context ibvctx;
    uint32_t cmds_supp_udata_mask;
    uint16_t uarn;
    uint32_t device_caps;
    // ... device-specific attributes
};

struct cxi_qp {
    struct verbs_qp verbs_qp;
    struct cxi_wq sq, rq;
    uint32_t qp_handle;
    // ... CXI-specific QP attributes
};

struct cxi_mr {
    struct verbs_mr verbs_mr;
    uint32_t md_handle;
};
```

### 4. Public API (`cxidv.h`)

```c
// Device capabilities
enum {
    CXIDV_DEVICE_CAP_ATOMIC_OPS = 1 << 0,
    CXIDV_DEVICE_CAP_RDMA_READ = 1 << 1,
    // ...
};

// Vendor-specific method functions
int cxidv_method1(struct ibv_context *context,
                  struct cxidv_method1_attr *attr,
                  uint32_t inlen);

int cxidv_method2(struct ibv_mr *mr,
                  struct cxidv_method2_attr *attr,
                  uint32_t inlen);

int cxidv_method3(struct ibv_qp *qp,
                  struct cxidv_method3_attr *attr,
                  uint32_t inlen);
```

### 5. Kernel Interface (`cxi-abi.h`)

Defines the kernel-userspace ABI:
- Command/response structures for all operations
- Vendor-specific method and attribute IDs
- Object definitions (CXI_IB_OBJECT_GENERIC)
- Method definitions (CXI_IB_METHOD_1/2/3)

## Integration Points

### With rdma-core Build System

The provider integrates with rdma-core via:
- **CMakeLists.txt**: Defines shared provider library
- **libcxi.map**: Exports public symbols
- **publish_headers()**: Installs public headers

### With Kernel Driver

The provider communicates with the kernel driver via:
- **Standard verbs**: ibv_cmd_* functions for basic operations
- **Vendor ioctls**: DECLARE_COMMAND_BUFFER/execute_ioctl for CXI methods
- **Object handles**: Maps userspace objects to kernel objects

### With Applications

Applications can use the provider via:
- **Standard verbs**: ibv_* functions for basic RDMA operations
- **CXI Direct Verbs**: cxidv_* functions for vendor-specific features
- **Device detection**: cxidv_is_supported() to check for CXI devices

## Method Implementation Details

### Method 1 - Device Information Query
```c
DECLARE_COMMAND_BUFFER(cmd, CXI_IB_OBJECT_GENERIC, CXI_IB_METHOD_1, 5);
fill_attr_out_ptr(cmd, CXI_IB_ATTR_METHOD1_RESP_NIC_ADDR, &resp.nic_addr);
// ... fill other attributes
execute_ioctl(context, cmd);
```

### Method 2 - Memory Region Query
```c
DECLARE_COMMAND_BUFFER(cmd, CXI_IB_OBJECT_GENERIC, CXI_IB_METHOD_2, 5);
fill_attr_in_obj(cmd, CXI_IB_ATTR_METHOD2_MR_HANDLE, mr->handle);
fill_attr_out_ptr(cmd, CXI_IB_ATTR_METHOD2_RESP_MD_HANDLE, &resp.md_handle);
// ... fill other attributes
execute_ioctl(mr->context, cmd);
```

### Method 3 - Queue Pair Query
```c
DECLARE_COMMAND_BUFFER(cmd, CXI_IB_OBJECT_GENERIC, CXI_IB_METHOD_3, 6);
fill_attr_in_obj(cmd, CXI_IB_ATTR_METHOD3_QP_HANDLE, qp->handle);
fill_attr_out_ptr(cmd, CXI_IB_ATTR_METHOD3_RESP_TXQ_HANDLE, &resp.txq_handle);
// ... fill other attributes
execute_ioctl(qp->context, cmd);
```

## Error Handling

The provider implements comprehensive error handling:
- **Parameter validation**: Check for NULL pointers and invalid sizes
- **Kernel error propagation**: Return errno values from kernel operations
- **Resource cleanup**: Proper cleanup on allocation failures
- **Thread safety**: Spinlocks for shared data structures

## Testing Strategy

The test program (`test_cxi.c`) validates:
1. **Device enumeration**: Find CXI devices
2. **Standard verbs**: Basic operations work correctly
3. **Vendor methods**: All three CXI methods function properly
4. **Error handling**: Graceful handling of failures

## Future Extensions

The provider is designed for extensibility:
- **Additional methods**: Easy to add CXI_IB_METHOD_4, 5, etc.
- **Enhanced capabilities**: New device capability flags
- **Performance optimizations**: Direct hardware access paths
- **Advanced features**: Triggered operations, multicast, etc.

This structure provides a solid foundation for CXI device support in rdma-core while maintaining compatibility with standard InfiniBand applications.
