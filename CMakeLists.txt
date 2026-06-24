# -----------------------------------------------------------------------------
# embed-dev-lab 仓库根 CMake 入口
# 聚合各 projects/ 子目录；实际构建多在工程目录通过 Presets 完成
# 用法: ./scripts/build.sh f103-manual-reg
# -----------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.20)
project(embed-dev-lab C ASM)

add_subdirectory(projects/f103-manual-reg)
