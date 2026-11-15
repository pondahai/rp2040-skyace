# skyace —— 由 rp2040-retro-sdk 的 template/ 產生（tools/new_app.py）
#
#   直接 USB 燒錄版：
#       cmake -S . -B build -G Ninja && ninja -C build
#   載入器版（放 SD 卡）+ standalone 版：
#       cmake -S . -B build_offset -G Ninja -DLOADER_OFFSET_BUILD=ON && ninja -C build_offset
#   編完推上掌機並啟動（掌機停在載入器選單、USB 接著）：
#       ninja -C build_offset skyace_run
#   PC 預覽：sh pc/build.sh && build_pc/preview out.png
cmake_minimum_required(VERSION 3.13)

# 找 SDK：-DRETRO_SDK_PATH=... > ./sdk（git submodule）> ../..（放在 SDK 的 examples/ 底下）
if(NOT RETRO_SDK_PATH)
    if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/sdk/cmake/retro_sdk.cmake")
        set(RETRO_SDK_PATH "${CMAKE_CURRENT_LIST_DIR}/sdk")
    elseif(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../../cmake/retro_sdk.cmake")
        set(RETRO_SDK_PATH "${CMAKE_CURRENT_LIST_DIR}/../..")
    else()
        message(FATAL_ERROR "找不到 rp2040-retro-sdk。請 git submodule add 到 ./sdk，"
                            "或用 -DRETRO_SDK_PATH=<路徑> 指定。")
    endif()
endif()

include(pico_sdk_import.cmake)
project(skyace C CXX ASM)
set(CMAKE_C_STANDARD 11)
pico_sdk_init()

include(${RETRO_SDK_PATH}/cmake/retro_sdk.cmake)

add_executable(skyace main.c scene.c ships.c)
target_link_libraries(skyace retro_sdk)          # 要用 SD 卡（hal/sd.h）改成 retro_sdk_sd
target_compile_options(skyace PRIVATE -Wall -Wextra)
retro_app(skyace COVER ${CMAKE_CURRENT_LIST_DIR}/assets/SKYACE.RAW)
