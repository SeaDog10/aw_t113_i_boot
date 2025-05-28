set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_VERSION 1)
cmake_minimum_required(VERSION 3.20)

project(t113 C CXX ASM)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

# 设置交叉编译工具链路径
set(TOOLCHAIN_PATH "D:/toolchain/gcc-arm-none-eabi-10.3-2021.10-win32/gcc-arm-none-eabi-10.3-2021.10/bin")

# 指定编译器和工具
set(CMAKE_C_COMPILER "${TOOLCHAIN_PATH}/arm-none-eabi-gcc.exe")
set(CMAKE_CXX_COMPILER "${TOOLCHAIN_PATH}/arm-none-eabi-g++.exe")
set(CMAKE_ASM_COMPILER "${TOOLCHAIN_PATH}/arm-none-eabi-gcc.exe")
set(CMAKE_AR "${TOOLCHAIN_PATH}/arm-none-eabi-ar.exe")
set(CMAKE_OBJCOPY "${TOOLCHAIN_PATH}/arm-none-eabi-objcopy.exe")
set(CMAKE_OBJDUMP "${TOOLCHAIN_PATH}/arm-none-eabi-objdump.exe")
set(CMAKE_LINKER "${TOOLCHAIN_PATH}/arm-none-eabi-ld.exe")
set(SIZE "${TOOLCHAIN_PATH}/arm-none-eabi-size.exe")
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# 编译选项（根据实际需求修改）
add_compile_options(
    -mcpu=cortex-a7
    -marm
    -mfpu=vfpv4
    -mfloat-abi=soft
    -mno-thumb-interwork
    -mno-unaligned-access
    -Wall
    -O0
    -gdwarf-2
)

set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/link_scripts/link.lds")
add_link_options(
    -mcpu=cortex-a7
    -marm
    -mfpu=vfpv4
    -mfloat-abi=soft
    -mno-thumb-interwork
    -mno-unaligned-access
    -Wall
    -Wl,--gc-sections,-Map=../build/${PROJECT_NAME}.map,-cref,-u,system_vectors
    -T ${LINKER_SCRIPT}
)

# 添加包含路径
include_directories(
    ${CMAKE_SOURCE_DIR}/application
    ${CMAKE_SOURCE_DIR}/boards
    ${CMAKE_SOURCE_DIR}/libcpu
    # ${CMAKE_SOURCE_DIR}/memheap
    # ${CMAKE_SOURCE_DIR}/partition
)

# 添加源文件
file(GLOB_RECURSE SRC_FILES
    "${CMAKE_SOURCE_DIR}/application/*.c"
    "${CMAKE_SOURCE_DIR}/boards/*.c"
    "${CMAKE_SOURCE_DIR}/libcpu/*.c"
    "${CMAKE_SOURCE_DIR}/libcpu/*.S"
    # "${CMAKE_SOURCE_DIR}/memheap/*.c"
    # "${CMAKE_SOURCE_DIR}/partition/*.c"
)

# 构建目标
add_executable(${PROJECT_NAME}.elf ${SRC_FILES} ${LINKER_SCRIPT})

# 编译后生成 bin 和 hex 文件（可选）
add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary ${PROJECT_NAME}.elf ${PROJECT_NAME}.bin
    COMMAND ${SIZE} ${PROJECT_NAME}.elf
    COMMAND ${CMAKE_OBJDUMP} -S ${PROJECT_NAME}.elf > ${PROJECT_NAME}.S
)
