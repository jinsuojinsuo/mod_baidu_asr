cmake_minimum_required(VERSION 3.10)
project(mod_baidu_asr)

set(CMAKE_CXX_STANDARD 11)


#编译错误检查测试时建议打开
add_compile_options(
        -Wall
        #        -Werror
        #        -Wextra
        #        -pedantic
        #        -Wimplicit-fallthrough
        #        -Wsequence-point
        #        -Wswitch-default
        #        -Wswitch-unreachable
        #        -Wswitch-enum
        #        -Wstringop-truncation
        #        -Wbool-compare
        #        -Wtautological-compare
        #        -Wfloat-equal
        #        -Wshadow=global
        #        -Wpointer-arith
        #        -Wpointer-compare
        #        -Wcast-align
        #        -Wcast-qual
        #        -Wwrite-strings #禁止使用c字符串
        #        -Wdangling-else
        #        -Wlogical-op
        #        -Wconversion
        #        -Wno-pedantic-ms-format
        #
        -Wno-unknown-pragmas
        #        -Wno-float-equal
        #        -Wno-switch-default
        #        -Wno-conversion
        #        -Wno-cast-qual
        #        -Wno-logical-op
        #        -Wno-switch-enum
)


add_definitions(
        -v
        -D__LINUX__
        -D_GLIBCXX_USE_CXX11_ABI=0
        -O0
        -fPIC
        -g
        -D_BDS_EASR_MFE_DNN="${PROJECT_SOURCE_DIR}/asr-linux-cpp-demo/resources/asr_resource/bds_easr_mfe_dnn.dat"
        -D_BDS_EASR_MFE_CMVN="${PROJECT_SOURCE_DIR}/asr-linux-cpp-demo/resources/asr_resource/bds_easr_mfe_cmvn.dat"
)

#指定头文件搜索路径 -I
include_directories(
        ./asr-linux-cpp-demo/include
        ./asr-linux-cpp-demo/include/ASR
        /usr/local/freeswitch/include/freeswitch
)

#指定静态库或动态库搜索路径 -L
link_directories(
        /usr/local/freeswitch/lib/
        ./asr-linux-cpp-demo/lib/
        ./asr-linux-cpp-demo/extern/lib/
)

#生成动态库
add_library(mod_baidu_asr SHARED mod_baidu_asr.cpp)

#去除库文件名前缀lib必须放在 add_library下面
set_target_properties(mod_baidu_asr PROPERTIES PREFIX "")

#指定要链接的静态库-ls
target_link_libraries(mod_baidu_asr
        rt
        dl
        pthread
        libBDSpeechSDK.a
        libcurl.a
        libiconv.a
        libz.a
        libssl.a
        libcrypto.a
        libuuid.a
        )

#定义make install 命令
install(TARGETS mod_baidu_asr
        LIBRARY DESTINATION /usr/local/freeswitch/mod/
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
        )


