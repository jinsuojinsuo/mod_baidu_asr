# 一、mod_baidu_asr介绍
freeswitch百度语音识别模块

# 二、先决条件
freeswitch 1.4.26、1.6.20、1.8.7

# 三、使用命令编译
```
baidu_asr_dir="/home/liujinsuo/CLionProjects/mod_baidu_asr/asr-linux-cpp-demo-3.0.0.30628d440-V1" &&\
g++ -v -shared -Wall -O0 -fPIC -g -D__LINUX__ -Wno-unknown-pragmas -D_GLIBCXX_USE_CXX11_ABI=0  -std=c++11 -lrt -ldl -lpthread \
-I /usr/local/freeswitch/include/freeswitch \
-L /usr/local/freeswitch/lib \
-I $baidu_asr_dir/include \
-I $baidu_asr_dir/include/ASR \
-o mod_baidu_asr.so \
mod_baidu_asr.cpp \
$baidu_asr_dir/lib/libBDSpeechSDK.a \
$baidu_asr_dir/extern/lib/libcurl.a \
$baidu_asr_dir/extern/lib/libiconv.a \
$baidu_asr_dir/extern/lib/libz.a \
$baidu_asr_dir/extern/lib/libssl.a \
$baidu_asr_dir/extern/lib/libcrypto.a \
$baidu_asr_dir/extern/lib/libuuid.a \
$baidu_asr_dir/extern/lib/libssl.a &&\
mv -f mod_baidu_asr.so /usr/local/freeswitch/mod/
```

# 四、freeswitch加载模块
## 1.将 mod_baidu_asr.so 放入 {freeswitch/mod/}mod_baidu_asr.so

## 2.进入freeswitch控制台
/usr/local/freeswitch/bin/fs_cli

## 3.卸载模块
unload mod_baidu_asr

## 4.加载模块
load mod_baidu_asr

## 5.重新加载
reload mod_baidu_asr


# 五、呼叫测试 语音编码必须是pcmu@8000
## 1.呼叫字符串
originate {absolute_codec_string=PCMU}user/8006 baidu_asr:'10555002 jhRA15uv8Lvd4r9qbtmOODMv f0a12f8261e1121861a1cd3f4ed02f68 15362 5',park inline

## 2.参数介绍
baidu_asr:'app_id chunk_key secret_key PRODUCT_ID 静音切分帧30帧=300ms'

## 3.百度参考文档
参考文档: https://ai.baidu.com/ai-doc/SPEECH/Rkh4hbm7p
百度sdk下载: https://ai.baidu.com/sdk#asr



# 六、事件
## 1.接收事件
CUSTOM baidu_asr_flush_data 一句话中间结果
CUSTOM baidu_asr_finish 一句话最终结果

## 2.事件订阅
/noevents 
/event CUSTOM baidu_asr_finish baidu_asr_flush_data

## 3.事件内容
```
Event-Subclass: baidu_asr
Event-Name: CUSTOM
Core-UUID: d0cbe0b3-d81a-48cf-9d8a-065bb8da8708
FreeSWITCH-Hostname: iZ2ze7ykmwwdhj59z93p9eZ
FreeSWITCH-Switchname: iZ2ze7ykmwwdhj59z93p9eZ
FreeSWITCH-IPv4: 172.17.122.132
FreeSWITCH-IPv6: ::1
Event-Date-Local: 2019-06-19 18:17:46
Event-Date-GMT: Wed, 19 Jun 2019 10:17:46 GMT
Event-Date-Timestamp: 1560939466879469
Event-Calling-File: mod_baidu_asr.cpp
Event-Calling-Function: asr_output_callback
Event-Calling-Line-Number: 180
Event-Sequence: 996
ASR-Response: {"results_recognition":["欢迎指点佳美口腔。"],"origin_result":{"corpus_no":6704183949337432629,"err_no":0,"result":{"word":["欢迎指点佳美口腔。"]},"sn":"27C6DB1A-FE2F-40F1-A697-723EEE00DAD3","voice_energy":33272.292968750},"sn_start_time":"00:00.720","sn_end_time":"00:02.560"}
Unique-ID: 2222e8cb-f017-4e21-ba5d-6a9fb430131d
```


