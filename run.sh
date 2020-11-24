#!/usr/bin/env bash
qemu-system-i386 -d int -monitor stdio -m 4g -cdrom kernel.iso
