# Exploit chain architecture

**[English](RESEARCH.md) | [Tiếng Việt](RESEARCH.vi.md)**

## Scope

This document describes the CVE-2023-33107 port for the Pixel 4 XL `coral`, build `TP1A.221005.002.B2`, kernel `4.14.276-g6ef255005cea-ab9062920`. Do not assume the constants below apply to another device or kernel.

## KGSL entry point

Two conditions in the target kernel form the chain's entry point:

1. `kgsl_iommu_set_svm_region` lacks a wrap check for `gpuaddr + size`.
2. `arm_lpae_map_sg` continues when `arm_lpae_init_pte` reports an existing PTE, corrupting mapping state.

The race between the `OVERLAP` and `BOGUS` mappings creates dangling IOMMU PTEs. The success signal on the test device is `OVERLAP mmap` returning `ENODEV`.

## Privilege escalation chain

1. Win the KGSL race and free the UAF region with dangling IOMMU mappings.
2. Apply memory pressure and spray tasks to reclaim the freed pages.
3. Use GPU `CP_MEM_WRITE` to read and write reclaimed pages while scanning for `task_struct` and user page-table signatures.
4. Derive the KASLR slide from the task's `fair_sched_class` pointer.
5. Replace the selected task's `real_cred` and `cred` with `init_cred`.
6. Locate a page-table alias using five marker pages at indices `1,3,5,7,9`.
7. Remap a PTE to the physical page containing `selinux_state`, verify its signature, and change the `enforcing` byte from `1` to `0`.
8. Start the broker and root executor; the client sends commands through the abstract Unix socket `@coral_rootd`.

## Coral-specific offsets

- `children`: `0x5d8`.
- `sched_class`: `0xb8`.
- `real_cred`: `0x760`.
- `cred`: `0x768`.
- `fair_sched_class` at link time: `0xffffff800a21e3c8`.
- `init_cred` at link time: `0xffffff800aa9e540`.
- `selinux_state` at link time: `0xffffff800aeb33b8`.
- `selinux_state.enforcing`: byte `+1`.

The KASLR slide is added to link-time kernel addresses at runtime. The physical base used for the `selinux_state` signature is `0x80080000` on the validated build.

## COW and SELinux

`uid=0` is insufficient to exec a shell while SELinux remains enforcing. A private-anonymous mapping previously produced a false result: the child wrote to a private COW page and read back `0`, while the kernel status page still reported `1`.

The release uses shared-anonymous mappings. After the GPU replaces the PTE, the process holding the alias calls `mprotect` to flush the TLB, then reads and writes physical memory directly without forking. The chain reports `SELINUX_PERMISSIVE_CONFIRMED` only after `/sys/fs/selinux/enforce` reads back `0`.

## Root broker and client

The broker detaches from the launcher with its own session and standard file descriptors directed to `/dev/null`. It ignores `SIGPIPE` and accepts only peer UID `0` or `2000`. Client input must be nonempty and NUL-terminated within five seconds.

Commands run one at a time with no duration or output-size cap. After `command_done`, the broker keeps draining the output pipe until `EAGAIN` before sending the exit status. Writes retry on `EINTR`; a disconnected client is marked dead while the pipe continues to be drained, preventing output from leaking into the next command. The executor forks a child, sets UID/GID to `0`, and executes `/system/bin/sh -c <command>`.

The client supports `su -c` directly on Android and `su --hex <command>` for the Windows scripts. `SU.bat` passes arguments to `scripts/su.ps1` with delayed expansion disabled; PowerShell encodes the command as UTF-8 hex. This prevents CMD from expanding `|`, `&&`, `&`, `>`, `<`, or consuming `!`, while preserving quotes, `;`, and `$()` through `adb shell`.

```sh
/data/local/tmp/coral_root su -c 'id'
```

After the command output, the broker sends one exit-status byte. The client does not print this byte and uses it as its own exit code.

## Limitations

- Race and reclaim do not succeed on every attempt.
- Incorrect reclaim can cause a kernel panic or reboot.
- The chain creates root only for the current boot and establishes no persistence.
- There is no automatic offset porting mechanism for other kernels.

CVE and reference exploit sources are listed in [NOTICE.md](../NOTICE.md).
