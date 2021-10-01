set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_SYSROOT /opt/fslc-framebuffer/2.6.2/sysroots/armv7at2hf-neon-fslc-linux-gnueabi/)

set(cross_prefix /opt/fslc-framebuffer/2.6.2/sysroots/x86_64-fslcsdk-linux/usr/bin/arm-fslc-linux-gnueabi/)
set(CMAKE_C_COMPILER ${cross_prefix}arm-fslc-linux-gnueabi-gcc)
set(CMAKE_CXX_COMPILER ${cross_prefix}arm-fslc-linux-gnueabi-g++)

add_compile_options(-march=armv7-a -mthumb -mfpu=neon -mfloat-abi=hard)
add_link_options(-march=armv7-a -mthumb -mfpu=neon -mfloat-abi=hard)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
