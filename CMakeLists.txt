# -----------------------------------------------------------------------------
# embed-dev-lab 仓库根 CMake 入口
# 聚合各 modules/ 子目录；实际构建多在模块目录通过 Presets 完成
# 用法: ./scripts/build.sh f103-blink
# -----------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.20)
project(embed-dev-lab C ASM)

add_subdirectory(modules/f103-blink)
