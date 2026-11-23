# Pixel 4 XL GPU Temp Root

**[English](README.md) | [Tiếng Việt](README.vi.md)**

Temp-root qua ADB cho Pixel 4 XL (`coral`) đang khóa bootloader, sử dụng CVE-2023-33107 trong Qualcomm KGSL/Adreno. Không cần mở OEM lock, không dùng fastboot và không flash hoặc sửa partition.

> Chỉ chạy trên thiết bị của bạn hoặc thiết bị bạn được phép kiểm thử. Đây là kernel exploit đặc thù cho một build; không phải bản root tổng quát cho mọi Pixel 4 XL.

## Thiết bị được hỗ trợ

- Product: `coral`.
- Build: `TP1A.221005.002.B2`.
- Kernel: `4.14.276-g6ef255005cea-ab9062920`.
- Bootloader: `ro.boot.flash.locked=1`.
- Verified boot: `ro.boot.verifiedbootstate=green`.

`ROOT_TEMP.bat` tự kiểm tra toàn bộ điều kiện trên trước khi chạy exploit. Không dùng binary này cho build khác vì kernel offset và địa chỉ physical đã được xác định riêng cho build nêu trên.

## Yêu cầu

- Windows và Windows PowerShell để dùng các script BAT.
- [Android platform-tools](https://developer.android.com/tools/releases/platform-tools), trong đó `adb` đã có trong `PATH`.
- USB debugging đã bật và máy tính đã được cho phép ADB.
- Binary ARM64 `bin/coral_root`, có sẵn trong repo hoặc tự build bằng Android NDK.

## Sử dụng nhanh

1. Kết nối Pixel 4 XL qua USB và kiểm tra `adb devices`.
2. Chạy `ROOT_TEMP.bat`.
3. Khi script báo thành công, chạy lệnh root bằng `SU.bat`:

```bat
SU.bat id
SU.bat getenforce
SU.bat "pm list packages"
SU.bat "printf '%s\n' 'root-ok'"
SU.bat "id | head -n 1"
SU.bat "echo ok && getprop ro.product.device"
SU.bat "echo hello!"
```

`SU.bat` trả exit code của lệnh trên thiết bị. Ví dụ, `SU.bat false` trả `1`.

Chạy `KIEM_TRA.bat` để kiểm tra UID, SELinux, trạng thái verified boot, quyền ghi file và hash boot partition của slot đang chạy.

Kết quả mong đợi gồm:

```text
uid=0(root) gid=0(root) groups=0(root) context=u:r:kernel:s0
Permissive
verifiedbootstate=green
flash.locked=1
dc076aa4a0f324592336572e87de070b1c0120596cbac76d5f6c2d40e28afcd4  /dev/block/by-name/boot_b
```

Tên partition là `boot_a` hoặc `boot_b` tùy slot đang chạy. Hash trên tương ứng stock `boot.img` của build được hỗ trợ.

## Nếu exploit thất bại

Race và reclaim có tính xác suất. Nếu log kết thúc bằng `pt_candidates=0` hoặc script trả lỗi mà thiết bị vẫn hoạt động, reboot rồi chạy lại `ROOT_TEMP.bat`.

Exploit có thể gây kernel panic hoặc reboot nếu race/reclaim đi sai trạng thái. Không chạy đồng thời nhiều instance và không thử trên build chưa được hỗ trợ.

## Vòng đời temp-root

- Root daemon chạy trong RAM và tiếp tục hoạt động sau khi launcher ADB thoát.
- Reboot thường sẽ xóa root: thiết bị trở về `uid=2000(shell)`, SELinux `Enforcing` và root socket không còn.
- Binary được đặt tại `/data/local/tmp/coral_root`; file này không tự khởi động lại và không tạo persistence.
- Không có Magisk, APatch, boot image patch hoặc module được cài đặt.

## Tự build binary

Cài Android NDK rồi đặt `NDK_CLANG` tới `clang.exe`. NDK 29 là toolchain đã dùng để kiểm chứng bản phát hành.

```bat
set "NDK_CLANG=C:\Android\ndk\29.0.14206865\toolchains\llvm\prebuilt\windows-x86_64\bin\clang.exe"
build.bat
```

Bạn cũng có thể đặt `ANDROID_NDK_HOME` thay cho `NDK_CLANG`. Script build bằng `-Wall -Wextra -Werror` và ghi kết quả vào `bin/coral_root`.

Lệnh compiler tương đương:

```sh
clang --target=aarch64-linux-android29 -static -O2 -Wall -Wextra -Werror -pthread \
  -o bin/coral_root src/coral_kgsl_root.c
```

## Cấu trúc repo

- `bin/coral_root`: exploit và client `su -c` cho ARM64.
- `src/coral_kgsl_root.c`: source tương ứng với binary phát hành.
- `ROOT_TEMP.bat`: kiểm tra thiết bị, deploy và chạy exploit.
- `SU.bat`: wrapper chuyển command tới PowerShell mà không bung metacharacter trong CMD.
- `scripts/su.ps1`: chọn thiết bị, encode command thành hex và gọi root client.
- `KIEM_TRA.bat`: kiểm tra trạng thái root và active boot partition.
- `build.bat`: build lại binary bằng Android NDK.
- `docs/RESEARCH.md` và `docs/RESEARCH.vi.md`: kiến trúc chuỗi exploit và các offset chính bằng English và tiếng Việt.
- `docs/VALIDATION.md` và `docs/VALIDATION.vi.md`: phạm vi kiểm thử và kết quả xác nhận bằng English và tiếng Việt.
- `SHA256SUMS.txt`: checksum của các file phát hành.
- `NOTICE.md`: nguồn tham khảo và credit.

## Tài liệu và giấy phép

Chi tiết kỹ thuật nằm trong [RESEARCH.vi.md](docs/RESEARCH.vi.md), kết quả kiểm chứng trong [VALIDATION.vi.md](docs/VALIDATION.vi.md). Nguồn tham khảo được ghi trong [NOTICE.md](NOTICE.md).

Repo phát hành theo [MIT License](LICENSE).
