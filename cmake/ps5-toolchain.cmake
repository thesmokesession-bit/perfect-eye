# Cross-compiling Perfect Dark (Dab's Mod) for the PS5: Mihawk's payload SDK fork, with SDL2,
# libcurl and OpenSSL from the PacBrew sysroot, RADV's Vulkan headers and a static shaderc built
# for the console (tools/build-shaderc.sh). Paths come from build.sh.
include("$ENV{PS5_PAYLOAD_SDK}/toolchain/prospero.cmake")
list(APPEND CMAKE_FIND_ROOT_PATH "$ENV{PACBREW}" "$ENV{SHADERC_PS5}" "$ENV{RADV_INCLUDE_ROOT}")
list(APPEND CMAKE_PREFIX_PATH "$ENV{PACBREW}" "$ENV{SHADERC_PS5}" "$ENV{RADV_INCLUDE_ROOT}")
set(CMAKE_VERBOSE_MAKEFILE OFF CACHE BOOL "" FORCE)
# The decomp's asset tools and the language packs run on the build machine.
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(PS5_PORT_DIR "${CMAKE_CURRENT_LIST_DIR}/.." CACHE PATH "")
set(CMAKE_C_FLAGS_INIT "-isystem $ENV{PACBREW}/include/SDL2 -isystem $ENV{PACBREW}/include")
set(CMAKE_CXX_FLAGS_INIT "-isystem $ENV{PACBREW}/include/SDL2 -isystem $ENV{PACBREW}/include")
# Executables are linked into a title by the PS5_Vulkan recipe (static RADV, signing).
set(CMAKE_C_LINK_EXECUTABLE "bash ${CMAKE_CURRENT_LIST_DIR}/../ps5/link-title.sh <TARGET> <OBJECTS> <LINK_LIBRARIES>")
set(CMAKE_CXX_LINK_EXECUTABLE "bash ${CMAKE_CURRENT_LIST_DIR}/../ps5/link-title.sh <TARGET> <OBJECTS> <LINK_LIBRARIES>")
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
