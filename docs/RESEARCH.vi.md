# Kiến trúc chuỗi exploit

**[English](RESEARCH.md) | [Tiếng Việt](RESEARCH.vi.md)**

## Phạm vi

Tài liệu này mô tả bản port CVE-2023-33107 cho Pixel 4 XL `coral`, build `TP1A.221005.002.B2`, kernel `4.14.276-g6ef255005cea-ab9062920`. Các hằng số bên dưới không được mặc định dùng cho thiết bị hoặc kernel khác.

## Điểm vào KGSL

Hai điều kiện trong kernel mục tiêu tạo thành điểm vào của chuỗi:

1. `kgsl_iommu_set_svm_region` thiếu kiểm tra wrap của `gpuaddr + size`.
2. `arm_lpae_map_sg` tiếp tục xử lý khi `arm_lpae_init_pte` báo PTE đã tồn tại, làm sai trạng thái mapping.

Race giữa mapping `OVERLAP` và `BOGUS` tạo dangling IOMMU PTE. Dấu hiệu race thành công trên thiết bị kiểm thử là `OVERLAP mmap` trả `ENODEV`.

## Chuỗi leo quyền

1. Thắng KGSL race và giải phóng vùng UAF có IOMMU mapping còn treo.
2. Áp memory pressure và spray task để reclaim các trang đã giải phóng.
3. Dùng GPU `CP_MEM_WRITE` để đọc/ghi các trang reclaim, đồng thời quét dấu hiệu `task_struct` và user page table.
4. Lấy KASLR slide từ con trỏ `fair_sched_class` trong task.
5. Đổi `real_cred` và `cred` của selected task sang `init_cred`.
6. Tìm page-table alias bằng năm trang marker tại index `1,3,5,7,9`.
7. Remap một PTE sang physical page chứa `selinux_state`, xác nhận signature rồi đổi byte `enforcing` từ `1` sang `0`.
8. Khởi chạy broker và root executor; client gửi lệnh qua abstract Unix socket `@coral_rootd`.

## Offset đặc thù cho coral

- `children`: `0x5d8`.
- `sched_class`: `0xb8`.
- `real_cred`: `0x760`.
- `cred`: `0x768`.
- `fair_sched_class` tại link-time: `0xffffff800a21e3c8`.
- `init_cred` tại link-time: `0xffffff800aa9e540`.
- `selinux_state` tại link-time: `0xffffff800aeb33b8`.
- `selinux_state.enforcing`: byte `+1`.

Các địa chỉ kernel link-time được cộng với KASLR slide tại runtime. Physical base dùng cho signature của `selinux_state` là `0x80080000` trên build đã kiểm thử.

## Vấn đề COW và SELinux

`uid=0` không đủ để exec shell khi SELinux vẫn enforcing. Bản private-anonymous mapping từng cho kết quả giả: process con ghi vào private COW page, đọc lại thấy `0`, nhưng kernel status page vẫn báo `1`.

Bản phát hành dùng shared-anonymous mapping. Sau khi GPU thay PTE, process giữ alias gọi `mprotect` để flush TLB rồi đọc/ghi physical trực tiếp, không qua fork. Chỉ khi `/sys/fs/selinux/enforce` đọc lại `0`, chuỗi mới báo `SELINUX_PERMISSIVE_CONFIRMED`.

## Root broker và client

Broker tách khỏi launcher bằng session riêng và file descriptor chuẩn trỏ tới `/dev/null`. Executor tạo process con, đặt UID/GID về `0` rồi exec `/system/bin/sh -c <command>`.

Client hỗ trợ `su -c` trực tiếp trên Android và `su --hex <command>` cho script Windows. `SU.bat` chuyển tham số sang `scripts/su.ps1` với delayed expansion bị tắt; PowerShell mã hóa command thành UTF-8 hex. Cách này tránh CMD bung `|`, `&&`, `&`, `>`, `<` hoặc nuốt `!`, đồng thời giữ đúng dấu nháy, `;`, `$()` qua lớp `adb shell`.

```sh
/data/local/tmp/coral_root su -c 'id'
```

Sau output của lệnh, broker gửi một byte exit status. Client không in byte này và dùng nó làm exit code của chính mình.

## Giới hạn

- Race/reclaim không bảo đảm thành công mỗi lần.
- Sai reclaim có thể gây kernel panic hoặc reboot.
- Chuỗi chỉ tạo root trong phiên boot hiện tại, không tạo persistence.
- Không có cơ chế tự động port offset cho kernel khác.

Nguồn CVE và exploit tham khảo được liệt kê trong [NOTICE.md](../NOTICE.md).
