# Kiểm chứng bản phát hành

**[English](VALIDATION.md) | [Tiếng Việt](VALIDATION.vi.md)**

## Môi trường

- Pixel 4 XL `coral`.
- Build `TP1A.221005.002.B2`.
- Kernel `4.14.276-g6ef255005cea-ab9062920`.
- Bootloader `locked`, verified boot `green`.
- Active slot trong phiên kiểm thử: `_b`.
- Stock `boot.img` và `boot_b` có SHA-256 `dc076aa4a0f324592336572e87de070b1c0120596cbac76d5f6c2d40e28afcd4`.

Không có serial thiết bị, tên user hoặc đường dẫn máy tính cá nhân trong repo public.

## Các kiểm thử đã thực hiện

1. Build source bằng Android NDK clang với `-Wall -Wextra -Werror`; build pass.
2. Chạy `ROOT_TEMP.bat` từ hai lần boot stock sạch; cả hai lần hoàn tất bằng `ROOT_DAEMON_DETACHED` và exit code `0`.
3. Client thực thi `id`, `getenforce`, tạo file trong `/data/local/tmp` và đọc active boot partition.
4. `SU.bat id` trả `0`; `SU.bat false` trả `1`; `KIEM_TRA.bat` trả `0`. `SU.bat "printf '%s\n' 'root-ok'"` in `root-ok`. Các test `|`, `&&`, `&`, `>`, `<` và `!` đều chạy trên Android; `getprop ro.product.device` sau `&` và `&&` trả `coral`, xác nhận vế phải không bị Windows thực thi.
5. Gọi root client nhiều lần sau khi launcher thoát; broker vẫn phản hồi.
6. Reboot và xác nhận:
   - `uid=2000(shell)`;
   - SELinux `Enforcing`;
   - root client báo `Connection refused`.
7. Chạy lại `ROOT_TEMP.bat` sau reboot và xác nhận root được tạo lại.
8. Đối chiếu active boot partition trước/sau kiểm thử bằng hash stock nêu trên; không có partition write.
9. Chạy lại repo public độc lập từ boot sạch: kiểm tra `locked=1`, `green`, đúng kernel, deploy từ `bin/`, root thành công và output không còn byte NUL.
10. Regression khi ngắt client: binary trước vá mất daemon sau khi client bị kill giữa output. Sau khi broker bỏ qua `SIGPIPE`, 10/10 vòng ngắt tương tự đều pass; mọi lệnh `id` tiếp theo trả `uid=0` và broker PID `20861` không đổi.
11. Protocol broker đã tăng cường: 50/50 kiểm tra marker cuối đều pass và hash output `seq 1 10000` khớp chính xác; lệnh 35 giây trả `0`; output 130.000.000 byte hoàn tất trong bốn giây và lệnh tiếp theo trả `0`; thêm 10/10 vòng disconnect pass với broker PID `21305` không đổi; client rỗng hoặc im lặng không chặn lệnh tiếp theo; peer đổi sang UID `10000` bị đóng kết nối mà không có output lệnh (`read=0`).

## Kết quả chính

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

## Trường hợp thất bại đã quan sát

Một lần chạy thắng race nhưng kết thúc bằng `pt_candidates=0`. Binary thoát với lỗi, không báo root và cần reboot sạch trước khi thử lại. Đây là giới hạn xác suất của reclaim, không phải đường fallback root bằng bootloader.

## Binary phát hành hiện tại

SHA-256 của binary ARM64 đã kiểm chứng:

```text
6f0a1ec0d80122ddf464bdd4893dd24bf426dad13acb03b0e5e830a66101222f  bin/coral_root
```

Checksum mới nhất của toàn bộ file nằm trong [SHA256SUMS.txt](../SHA256SUMS.txt).
