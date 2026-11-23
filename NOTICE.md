# Credits and references

- Project Zero: [CVE-2023-33107 — Qualcomm KGSL/Adreno analysis](https://googleprojectzero.github.io/0days-in-the-wild/0day-RCAs/2023/CVE-2023-33107.html).
- KETO0422: [Galaxy A90 5G exploit write-up and reference implementation](https://keto0422.github.io/2026/02/11/cve-writeup/). The KGSL race, dangling-PTE approach and page-table spray informed the initial port.
- Google: [Pixel factory images](https://developers.google.com/android/images) for the `coral-tp1a.221005.002.b2` build and stock kernel artifacts.

This repository contains the Pixel 4 XL (`coral`) port, target-specific kernel offsets, task/page-table reclaim logic, physical SELinux state handling and the temporary root command broker.
