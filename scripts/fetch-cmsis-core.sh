#!/usr/bin/env bash
# 兼容入口：初始化全部 CMSIS submodules（core + device-f1）
exec "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/fetch-cmsis.sh" "$@"
