cmake_minimum_required (VERSION 3.22)
project (injector)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE SRC
    "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/*.c"
)
set(INCS
    "${CMAKE_CURRENT_LIST_DIR}/src"
)

add_executable(injector ${SRC})
target_include_directories(injector PRIVATE ${INCS})

set(THIS_COMPILE_FLAGS
    -O3 -march=armv8-a -mtune=cortex-a53 -D_GNU_SOURCE -Wall -Werror -fdata-sections -ffunction-sections 
    -fno-threadsafe-statics -fno-rtti -fno-strict-aliasing -fomit-frame-pointer -finline-functions 
    -fvisibility=hidden -fvisibility-inlines-hidden -flto
)
set(THIS_LINK_FLAGS
    -static-libstdc++ -fPIE -O3 -ffixed-x18 -flto -Wl,--hash-style=both -Wl,-exclude-libs,ALL -Wl,--gc-sections 
    -Wl,--icf=all,-O3,--lto-O3,--strip-all 
)

target_compile_options(injector PRIVATE ${THIS_COMPILE_FLAGS})
target_link_options(injector PRIVATE ${THIS_LINK_FLAGS})
