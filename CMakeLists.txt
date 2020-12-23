cmake_minimum_required(VERSION 3.10)
project(mod_baidu_asr)

set(CMAKE_CXX_STANDARD 14)


#编译错误检查测试时建议打开
add_compile_options(
        -Wall
        -Werror
        -Wextra
        -pedantic
        -Wimplicit-fallthrough
        -Wsequence-point
        -Wswitch-default
        -Wswitch-unreachable
        -Wswitch-enum
        -Wstringop-truncation
        -Wbool-compare
        -Wtautological-compare
        -Wfloat-equal
        -Wshadow=global
        -Wpointer-arith
        -Wpointer-compare
        -Wcast-align
        -Wcast-qual
        -Wwrite-strings #禁止使用c字符串
        -Wdangling-else
        -Wlogical-op
        -Wconversion
        -Wno-pedantic-ms-format

        -Wno-unknown-pragmas
        -Wno-float-equal
        -Wno-switch-default
        -Wno-conversion
        -Wno-cast-qual
        -Wno-logical-op
        -Wno-switch-enum
)


add_definitions(
        -D__LINUX__
        -D_GLIBCXX_USE_CXX11_ABI=0
        -g
        -O0
        -fPIC
        -v

)

#指定头文件搜索路径 -I
include_directories(
        ./asr-linux-cpp-demo-3.0.0.30628d440-V1/include
        ./asr-linux-cpp-demo-3.0.0.30628d440-V1/include/ASR
        /usr/local/freeswitch/include/freeswitch
)

#指定静态库或动态库搜索路径 -L
link_directories(
        /usr/local/freeswitch/lib
        ./asr-linux-cpp-demo-3.0.0.30628d440-V1/lib/
        ./asr-linux-cpp-demo-3.0.0.30628d440-V1/extern/lib/
)

#生成动态库
add_library(mod_baidu_asr SHARED mod_baidu_asr.cpp)

#去除库文件名前缀lib必须放在 add_library下面
set_target_properties(mod_baidu_asr PROPERTIES PREFIX "")

#链接动态库
target_link_libraries(mod_baidu_asr
        -lrt
        -ldl
        -lpthread
        )

#指定要链接的静态库-l
target_link_libraries(mod_baidu_asr
        libBDSpeechSDK.a
        libcurl.a
        libiconv.a
        libz.a
        libssl.a
        libcrypto.a
        libuuid.a
        libssl.a
        )

#定义make install 命令
install(TARGETS mod_baidu_asr
    LIBRARY DESTINATION /usr/local/freeswitch/mod/
    LIBRARY PERMISSIONS OWNER_READ,OWNER_WRITE,OWNER_EXECUTE,GROUP_READ,GROUP_WRITE,WORLD_READ,WORLD_WRITE
)

