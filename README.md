# Pixel 4 XL GPU Temp Root

**[English](README.md) | [Tiếng Việt](README.vi.md)**

Temporary root through ADB for the Pixel 4 XL (`coral`) with a locked bootloader, using CVE-2023-33107 in Qualcomm KGSL/Adreno. It does not require an OEM unlock, does not use fastboot, and does not flash or modify partitions.

> Only run this on a device you own or are authorized to test. This is a build-specific kernel exploit, not a generic root solution for every Pixel 4 XL.

## Supported device

- Product: `coral`.
- Build: `TP1A.221005.002.B2`.
- Kernel: `4.14.276-g6ef255005cea-ab9062920`.
- Bootloader: `ro.boot.flash.locked=1`.
- Verified boot: `ro.boot.verifiedbootstate=green`.

`ROOT_TEMP.bat` checks all of these conditions before running the exploit. Do not use this binary on another build because its kernel offsets and physical addresses are specific to the build listed above.

## Requirements

- Windows and Windows PowerShell for the BAT scripts.
- [Android platform-tools](https://developer.android.com/tools/releases/platform-tools), with `adb` available in `PATH`.
- USB debugging enabled and the computer authorized for ADB.
- The ARM64 `bin/coral_root` binary, included in this repo or rebuilt with the Android NDK.

## Quick start

1. Connect the Pixel 4 XL over USB and check `adb devices`.
2. Run `ROOT_TEMP.bat`.
3. After the script reports success, run root commands through `SU.bat`:

```bat
SU.bat id
SU.bat getenforce
SU.bat "pm list packages"
SU.bat "printf '%s\n' 'root-ok'"
SU.bat "id | head -n 1"
SU.bat "echo ok && getprop ro.product.device"
SU.bat "echo hello!"
```

`SU.bat` returns the device command's exit code. For example, `SU.bat false` returns `1`.

Run `KIEM_TRA.bat` to check UID, SELinux, verified boot state, file write access, and the hash of the active slot's boot partition.

Expected output includes:

```text
uid=0(root) gid=0(root) groups=0(root) context=u:r:kernel:s0
Permissive
verifiedbootstate=green
flash.locked=1
dc076aa4a0f324592336572e87de070b1c0120596cbac76d5f6c2d40e28afcd4  /dev/block/by-name/boot_b
```

The partition is `boot_a` or `boot_b` depending on the active slot. The hash above corresponds to the stock `boot.img` for the supported build.

## If the exploit fails

Race and reclaim are probabilistic. If the log ends with `pt_candidates=0`, or the script fails while the device remains operational, reboot and run `ROOT_TEMP.bat` again.

The exploit can cause a kernel panic or reboot if the race or reclaim enters an unexpected state. Do not run multiple instances concurrently or try it on an unsupported build.

## Temporary root lifecycle

- The root daemon runs in RAM and remains available after the ADB launcher exits.
- A normal reboot removes root: the device returns to `uid=2000(shell)`, SELinux `Enforcing`, and the root socket disappears.
- The binary is stored at `/data/local/tmp/coral_root`; it does not start automatically and creates no persistence.
- No Magisk, APatch, boot image patch, or module is installed.

## Build the binary

Install the Android NDK and set `NDK_CLANG` to `clang.exe`. NDK 29 was used to validate the release.

```bat
set "NDK_CLANG=C:\Android\ndk\29.0.14206865\toolchains\llvm\prebuilt\windows-x86_64\bin\clang.exe"
build.bat
```

You can set `ANDROID_NDK_HOME` instead of `NDK_CLANG`. The script builds with `-Wall -Wextra -Werror` and writes `bin/coral_root`.

Equivalent compiler command:

```sh
clang --target=aarch64-linux-android29 -static -O2 -Wall -Wextra -Werror -pthread \
  -o bin/coral_root src/coral_kgsl_root.c
```

## Repository layout

- `bin/coral_root`: ARM64 exploit and `su -c` client.
- `src/coral_kgsl_root.c`: source corresponding to the release binary.
- `ROOT_TEMP.bat`: device checks, deployment, and exploit launcher.
- `SU.bat`: wrapper that passes commands to PowerShell without expanding CMD metacharacters.
- `scripts/su.ps1`: device selection, hex encoding, and root client invocation.
- `KIEM_TRA.bat`: root state and active boot partition checks.
- `build.bat`: Android NDK build script.
- `docs/RESEARCH.md` and `docs/RESEARCH.vi.md`: exploit chain and key offsets in English and Vietnamese.
- `docs/VALIDATION.md` and `docs/VALIDATION.vi.md`: test scope and results in English and Vietnamese.
- `SHA256SUMS.txt`: release file checksums.
- `NOTICE.md`: references and credits.

## Documentation and license

See [RESEARCH.md](docs/RESEARCH.md) for technical details and [VALIDATION.md](docs/VALIDATION.md) for validation results. References are listed in [NOTICE.md](NOTICE.md).

This repo is released under the [MIT License](LICENSE).
