# Release validation

**[English](VALIDATION.md) | [Tiếng Việt](VALIDATION.vi.md)**

## Environment

- Pixel 4 XL `coral`.
- Build `TP1A.221005.002.B2`.
- Kernel `4.14.276-g6ef255005cea-ab9062920`.
- `locked` bootloader and `green` verified boot.
- Active slot during validation: `_b`.
- Stock `boot.img` and `boot_b` SHA-256: `dc076aa4a0f324592336572e87de070b1c0120596cbac76d5f6c2d40e28afcd4`.

The public repo contains no device serial, username, or personal computer path.

## Tests performed

1. Built the source with Android NDK clang using `-Wall -Wextra -Werror`; the build passed.
2. Ran `ROOT_TEMP.bat` from two clean stock boots; both completed with `ROOT_DAEMON_DETACHED` and exit code `0`.
3. Used the client to execute `id` and `getenforce`, create a file in `/data/local/tmp`, and read the active boot partition.
4. `SU.bat id` returned `0`; `SU.bat false` returned `1`; `KIEM_TRA.bat` returned `0`. `SU.bat "printf '%s\n' 'root-ok'"` printed `root-ok`. Tests for `|`, `&&`, `&`, `>`, `<`, and `!` all ran on Android; `getprop ro.product.device` after `&` and `&&` returned `coral`, confirming that Windows did not execute the right-hand side.
5. Called the root client repeatedly after the launcher exited; the broker remained available.
6. Rebooted and confirmed:
   - `uid=2000(shell)`;
   - SELinux `Enforcing`;
   - root client reported `Connection refused`.
7. Ran `ROOT_TEMP.bat` after reboot and confirmed that root was recreated.
8. Compared the active boot partition with the stock hash above before and after testing; no partition write occurred.
9. Ran the standalone public repo from a clean boot: verified `locked=1`, `green`, the expected kernel, deployment from `bin/`, successful root, and output without NUL bytes.

## Key output

```text
INIT_CRED_SWITCH_CONFIRMED
PTE_ALIAS_FOUND
SELINUX_STATE_CANDIDATE base=0x80080000 bytes=0,1,0,1 pointers=4
SELINUX_SIGNATURE_WRITE base=0x80080000 new=0 mode=0
SELINUX_PERMISSIVE_CONFIRMED
ROOT_DAEMON_DETACHED
uid=0(root) gid=0(root) groups=0(root) context=u:r:kernel:s0
Permissive
```

## Observed failure case

One attempt won the race but ended with `pt_candidates=0`. The binary exited with an error, did not claim root, and required a clean reboot before retrying. This is a probabilistic reclaim limitation, not a bootloader-based root fallback.

## Current release binary

Validated ARM64 binary SHA-256:

```text
60f8ee3a9ce56d491297c127fc899fff7e5e967113a8c0149a79741cc9c54958  bin/coral_root
```

The latest checksums for all files are in [SHA256SUMS.txt](../SHA256SUMS.txt).
