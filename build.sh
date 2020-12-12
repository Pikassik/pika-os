#!/usr/bin/env bash
make && cp kernel.bin isodir/boot/kernel.bin && grub-mkrescue -o kernel.iso isodir
