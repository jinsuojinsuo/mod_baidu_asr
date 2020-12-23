#include <iostream>
#include <unistd.h>
#include <cstring>
#include <cstdio>
#include <ctime>
#include <typeinfo>
#include <switch.h>

//引入语音识别模块
#include "BDSpeechSDK.hpp"
#include "bds_ASRDefines.hpp"
#include "bds_asr_key_definitions.hpp"

using namespace std;

//下面的代码是调用一个带参数的宏,这此宏定义在switch_type.h中,替换后如下一行
//switch_status_t mod_baidu_asr_load(switch_loadable_module_interface_t **module_interface, switch_memory_pool_t *pool);
//SWITCH_MODULE_LOAD_FUNCTION(加载模块时执行的函数)
SWITCH_MODULE_LOAD_FUNCTION(mod_baidu_asr_load);

//下面的代码是调用一个带参数的宏,这此宏定义在switch_type.h中,替换后如下一行
//switch_status_t mod_baidu_asr_shutdown (void)
//SWITCH_MODULE_SHUTDOWN_FUNCTION(卸载时执行的函数)
SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_baidu_asr_shutdown);

/*下面的代码是调用一个带参数的宏,这此宏定义在switch_type.h中,替换后如下一行
static const char modname[] = #mod_baidu_asr;
SWITCH_MOD_DECLARE_DATA switch_loadable_module_function_table_t mod_baidu_asr##_module_interface = {SWITCH_API_VERSION, mod_baidu_asr_load, mod_baidu_asr_shutdown, NULL, SMODF_NONE}
SWITCH_MODULE_DEFINITION(模块名,加载时执行的函数,卸载时执行的函数,runtime不知道干啥的)*/
SWITCH_MODULE_DEFINITION(mod_baidu_asr, mod_baidu_asr_load, mod_baidu_asr_shutdown, NULL);

// bds::BDSSDKMessage push_params;
// push_params.name = bds::ASR_CMD_PUSH_AUDIO;

struct switch_da_t {
    int stop;
    char *app_id;
    char *chunk_key;
    char *secret_key;
    char *product_id;
    bool asr_finish_tags;//线程是否结束识别 true已结束 未結束false
    float vad_pause_frames = 30;     //设置vad语句静音切分门限（帧）, 30帧 = 300ms
    bds::BDSpeechSDK *sdk;             //百度sdk
    bds::BDSSDKMessage *push_params; //传输方式
    switch_channel_t *channel;         //freeswitch channels
    switch_core_session_t *session;  //channels 包含 session
    switch_media_bug_t *bug;
};

/**
 * 请根据文档说明设置参数
 */
void asr_set_config_params(bds::BDSSDKMessage &cfg_params, switch_da_t *user_data) {
    //const bds::TBDVoiceRecognitionDebugLogLevel sdk_log_level = bds::EVRDebugLogLevelTrace;
    const bds::TBDVoiceRecognitionDebugLogLevel sdk_log_level = bds::EVRDebugLogLevelOff; // 关闭详细日志

    // app_id app_key app_secret 请测试成功后替换为您在网页上申请的appId appKey和appSecret
    const std::string app_id = user_data->app_id;
    const std::string chunk_key = user_data->chunk_key;
    const std::string secret_key = user_data->secret_key;
    const std::string product_id = user_data->product_id; // 普通话搜索模型：1536，普通话搜索模型+语义理解 15361, 普通话输入法模型（有逗号） 1537 , 8001 8002
    float vad_pause_frames = user_data->vad_pause_frames; //设置vad语句静音切分门限（帧）, 30帧 = 300ms

    // switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "app_id %s\n", user_data->app_id);
    // switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "chunk_key %s\n", user_data->chunk_key);
    // switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "secret_key %s\n", user_data->secret_key);
    // switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "product_id %s\n", user_data->product_id);
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "%s %s %s %s %s %f\n",
                      switch_channel_get_name(user_data->channel), user_data->app_id, user_data->chunk_key,
                      user_data->secret_key, user_data->product_id, user_data->vad_pause_frames);

    cfg_params.name = bds::ASR_CMD_CONFIG;

    cfg_params.set_parameter(bds::ASR_PARAM_KEY_APP_ID, app_id);

    // 自训练平台上线模型的调用参数，与product_id 8001 或 8002连用。
    // cfg_params.set_parameter(bds::ASR_PARAM_KEY_LMID, 1068);  // 设为 product_id = 8002

    cfg_params.set_parameter(bds::ASR_PARAM_KEY_CHUNK_KEY, chunk_key);
    cfg_params.set_parameter(bds::ASR_PARAM_KEY_PRODUCT_ID, product_id);
    cfg_params.set_parameter(bds::COMMON_PARAM_KEY_DEBUG_LOG_LEVEL, sdk_log_level);
    cfg_params.set_parameter(bds::ASR_PARAM_KEY_MAX_SPEECH_PAUSE, vad_pause_frames);

    // cfg_params.set_parameter(bds::ASR_PARAM_KEY_SAVE_AUDIO_ENABLE, 1);    //是否存识别的音频
    // cfg_params.set_parameter(bds::ASR_PARAM_KEY_SAVE_AUDIO_PATH, "sdk_save_audio.d");  //存音频的路径

    cfg_params.set_parameter(bds::ASR_PARAM_KEY_ENABLE_LONG_SPEECH, 1);                                                                                                   // 强制固定值
    cfg_params.set_parameter(bds::ASR_PARAM_KEY_CHUNK_ENABLE, 1);                                                                                                       // 强制固定值
    const std::string mfe_dnn_file_path = "/var/cpp/freeswitch_mod/mod_baidu_asr/asr-linux-cpp-demo-3.0.0.30628d440-V1/resources/asr_resource/bds_easr_mfe_dnn.dat";   //  bds_easr_mfe_dnn.dat文件路径
    const std::string mfe_cmvn_file_path = "/var/cpp/freeswitch_mod/mod_baidu_asr/asr-linux-cpp-demo-3.0.0.30628d440-V1/resources/asr_resource/bds_easr_mfe_cmvn.dat"; //  bds_easr_mfe_cmvn.dat文件路径
    cfg_params.set_parameter(bds::ASR_PARAM_KEY_MFE_DNN_DAT_FILE, mfe_dnn_file_path);                                                                                   // 强制固定值
    cfg_params.set_parameter(bds::ASR_PARAM_KEY_MFE_CMVN_DAT_FILE, mfe_cmvn_file_path);                                                                                   // 强制固定值
    cfg_params.set_parameter(bds::ASR_PARAM_KEY_COMPRESSION_TYPE, bds::EVR_AUDIO_COMPRESSION_PCM);

    // cfg_params.set_parameter(bds::ASR_PARAM_KEY_COMPRESSION_TYPE, bds::EVR_AUDIO_COMPRESSION_BV32); // 有损压缩, 可能遇见音频压缩问题

    //设置录音采样率 EVoiceRecognitionRecordSampleRate8K|EVoiceRecognitionRecordSampleRate16K
    cfg_params.set_parameter(bds::ASR_PARAM_KEY_SAMPLE_RATE, bds::EVoiceRecognitionRecordSampleRate8K);
}

// 设置启动参数
void asr_set_start_params(bds::BDSSDKMessage &start_params) {
    const std::string app = "YourOwnName";
    start_params.name = bds::ASR_CMD_START;
    start_params.set_parameter(bds::ASR_PARAM_KEY_APP, app);
    start_params.set_parameter(bds::ASR_PARAM_KEY_PLATFORM, "linux");          //固定值
    start_params.set_parameter(bds::ASR_PARAM_KEY_SDK_VERSION, "LINUX TEST"); //固定值
}

/**
 * SDK 识别过程中的回调，注意回调产生在SDK内部的线程中，并非调用线程。
 * @param message IN SDK的回调信息
 * @param user_arg IN 用户设置set_event_listener的第二个参数
 * {"results_recognition":["今"],"origin_result":{"corpus_no":6704127917118906481,"err_no":0,"result":{"word":["今"]},"sn":"5FC0A526-64E9-4DB8-AF7F-A959666166A8"},"sn_start_time":"00:00.340","sn_end_time":"00:01.700"}
 * {"results_recognition":["今天"],"origin_result":{"corpus_no":6704127917118906481,"err_no":0,"result":{"word":["今天"]},"sn":"5FC0A526-64E9-4DB8-AF7F-A959666166A8"},"sn_start_time":"00:00.340","sn_end_time":"00:01.209"}
 * {"results_recognition":["今天天"],"origin_result":{"corpus_no":6704127917118906481,"err_no":0,"result":{"word":["今天天"]},"sn":"5FC0A526-64E9-4DB8-AF7F-A959666166A8"},"sn_start_time":"00:00.340","sn_end_time":"00:01.550"}
 * {"results_recognition":["今天天气"],"origin_result":{"corpus_no":6704127917118906481,"err_no":0,"result":{"word":["今天天气"]},"sn":"5FC0A526-64E9-4DB8-AF7F-A959666166A8"},"sn_start_time":"00:00.340","sn_end_time":"00:01.840"}
 * {"results_recognition":["今天天气怎"],"origin_result":{"corpus_no":6704127917118906481,"err_no":0,"result":{"word":["今天天气怎"]},"sn":"5FC0A526-64E9-4DB8-AF7F-A959666166A8"},"sn_start_time":"00:00.340","sn_end_time":"00:02.200"}
 * {"results_recognition":["今天天气怎样？"],"origin_result":{"corpus_no":6704127917118906481,"err_no":0,"result":{"word":["今天天气怎样？"]},"sn":"5FC0A526-64E9-4DB8-AF7F-A959666166A8","voice_energy":26029.9433593750},"sn_start_time":"00:00.340","sn_end_time":"00:02.509"}
 *
 */
void asr_output_callback(bds::BDSSDKMessage &message, void *user_arg) {
    // FILE *err_output_file = stderr;
    switch_da_t *user_data = (switch_da_t *) user_arg;
    const char *channel_name = switch_channel_get_name(user_data->channel);

    if (message.name != bds::asr_callback_name) {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "%s shouldn't call\n", channel_name);
        return;
    }

    int status = 0;

    if (!message.get_parameter(bds::CALLBACK_ASR_STATUS, status)) {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "%s get status failed\n", channel_name);
        return;
    }

    switch (status) {
        // 识别工作开始，开始采集及处理数据
        case bds::EVoiceRecognitionClientWorkStatusStartWorkIng: {
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "%s 识别工作开始\n", channel_name);
            break;
        }

            // 检测到用户开始说话
        case bds::EVoiceRecognitionClientWorkStatusStart: {
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "%s 检测到用户开始说话\n", channel_name);
            break;
        }

            // 本地声音采集结束，等待识别结果返回并结束录音
        case bds::EVoiceRecognitionClientWorkStatusEnd: {
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "%s 本地声音采集结束，等待识别结果返回并结束录音\n", channel_name);
            break;
        }

            // 连续上屏,中间结果
        case bds::EVoiceRecognitionClientWorkStatusFlushData: {
            std::string json_result;
            message.get_parameter(bds::CALLBACK_ASR_RESULT, json_result);

            switch_event_t *event = nullptr;
            if (switch_event_create(&event, SWITCH_EVENT_CUSTOM) == SWITCH_STATUS_SUCCESS) {
                event->subclass_name = strdup("baidu_asr_flush_data");
                switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "Event-Subclass", event->subclass_name);                      //事件名
                switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "ASR-Response", json_result.c_str());                      //一句话识别结果
                switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "Unique-ID", switch_channel_get_uuid(user_data->channel)); //通道uuid
                switch_event_fire(&event);                                                                                              //发送事件
            }

            //switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "%s patial result: %s\n", switch_channel_get_name(user_data->channel), json_result.c_str());
            break;
        }

            //一句话的最终结果
        case bds::EVoiceRecognitionClientWorkStatusFinish: {
            std::string json_result;
            message.get_parameter(bds::CALLBACK_ASR_RESULT, json_result);

            switch_event_t *event = nullptr;
            if (switch_event_create(&event, SWITCH_EVENT_CUSTOM) == SWITCH_STATUS_SUCCESS) {
                event->subclass_name = strdup("baidu_asr_finish");
                switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "Event-Subclass", event->subclass_name);                      //事件名
                switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "ASR-Response", json_result.c_str());                      //一句话识别结果
                switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "Unique-ID", switch_channel_get_uuid(user_data->channel)); //通道uuid
                switch_event_fire(&event);                                                                                              //发送事件
            }

            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "%s final result: %s\n",
                              switch_channel_get_name(user_data->channel), json_result.c_str());
            break;
        }

            //语义解析
        case bds::EVoiceRecognitionClientWorkStatusChunkNlu: {
            const char *buf;
            int len = 0;
            message.get_parameter(bds::DATA_CHUNK, buf, len);
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "%s nlu result:", channel_name);
            for (int i = 0; i < len; ++i) {
                switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "%c", buf[i]);
            }
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "\n");
            break;
        }

            // 长语音结束状态 该实例处于空闲状态
        case bds::EVoiceRecognitionClientWorkStatusLongSpeechEnd: {
            user_data->asr_finish_tags = true;//标记为识别结束
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "%s 识别完成\n", channel_name);
            break;
        }

            // 产生错误 该实例处于空闲状态
        case bds::EVoiceRecognitionClientWorkStatusError: {
            int err_code = 0;
            int err_domain = 0;
            std::string err_desc;
            message.get_parameter(bds::CALLBACK_ERROR_CODE, err_code);
            message.get_parameter(bds::CALLBACK_ERROR_DOMAIN, err_domain);
            message.get_parameter(bds::CALLBACK_ERROR_DESC, err_desc);

            std::string sn;
            message.get_parameter(bds::CALLBACK_ERROR_SERIAL_NUM, sn);
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO,
                              "%s 识别出错, err_code: %d, err_domain: %d,err_desc: %s, sn: %s\n",
                              channel_name, err_code, err_domain, err_desc.c_str(),
                              sn.c_str());
            user_data->asr_finish_tags = true;//标记为识别结束
            break;
        }

            // 用户取消 该实例处于空闲状态
        case bds::EVoiceRecognitionClientWorkStatusCancel: {
            user_data->asr_finish_tags = true;//标记为识别结束
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "%s 用户取消\n", channel_name);
            break;
        }

            //录音数据回调 每次上传录音都会执行这里
        case bds::EVoiceRecognitionClientWorkStatusNewRecordData: {
            break;
        }

            //当前音量回调 6
        case bds::EVoiceRecognitionClientWorkStatusMeterLevel: {
            break;
        }

            //CHUNK: 识别结果中的第三方数据
            // case bds::EVoiceRecognitionClientWorkStatusChunkThirdData:
            // {
            // 	const char *buf;
            // 	int len = 0;
            // 	message.get_parameter(bds::DATA_CHUNK, buf, len);
            // 	//第三方结果未必是文本字符串，所以以%s打印未必有意义
            // 	fprintf(result_output_file, "third final result len[%d]\n", len);
            // 	//for (int i = 0; i < len; ++i) fprintf(result_output_file, "%c", buf[i]);
            // 	fprintf(result_output_file, "\n");
            // 	break;
            // }

            // CHUNK: 识别过程结束 应该就一句话结束
        case bds::EVoiceRecognitionClientWorkStatusChunkEnd: {
            // switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "%s CHUNK: 识别过程结束%d\n", switch_channel_get_name(user_data->channel), status);
            break;
        }

            //默认
        default: {
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "%s 其它状态%d\n", channel_name, status);
            break;
        }
    }
}

/**
 * 释放SDK
 */
void asr_online_release(bds::BDSpeechSDK *sdk) {
    bds::BDSpeechSDK::release_instance(sdk);
}

/**
 * 发送停止命令
 */
int asr_online_stop(bds::BDSpeechSDK *sdk) {
    FILE *err_output_file = stderr;
    std::string err_msg;
    bds::BDSSDKMessage stop_params;
    stop_params.name = bds::ASR_CMD_STOP;

    if (!sdk->post(stop_params, err_msg)) {
        fprintf(err_output_file, "stop sdk failed for %s\n", err_msg.c_str());
        return 1;
    }

    return 0;
}

//switch_core_media_bug_add()的回调函数
static switch_bool_t baidu_asr_callback(switch_media_bug_t *bug, void *user_arg, switch_abc_type_t type) {
    switch_da_t *user_data = (switch_da_t *) user_arg;
    std::string err_msg;
    const char *channel_name = switch_channel_get_name(user_data->channel);

    switch (type) {
        case SWITCH_ABC_TYPE_INIT: {//媒体bug设置成功
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "%s SWITCH_ABC_TYPE_INIT\n", channel_name);
            break;
        }
        case SWITCH_ABC_TYPE_CLOSE: { //媒体流关闭 资源回收
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "%s SWITCH_ABC_TYPE_CLOSE\n", channel_name);

            //告诉sdk，后续不会再post音频数据 ， 注意这个调用之后需要紧接着调用asr_online_stop
            user_data->push_params->set_parameter(bds::DATA_CHUNK, nullptr, 0);

            /*  6 发送停止传输音频数据标记  */
            asr_online_stop(user_data->sdk);

            /*  7 等待识别结束 */
            while (!user_data->asr_finish_tags) {
                usleep(10000);//微秒
            }

            /* 8 关闭日志 ，如果之前调用过 open_log_file  */
            // bds::BDSpeechSDK::close_log_file();

            /*  8 释放sdk  与get_instance 对应 SDK不是处于空闲状态（见下面的空闲状态定义），调用 bds::BDSpeechSDK::release_instance可能引起程序出core。  */
            asr_online_release(user_data->sdk);

            //内存清理
            delete user_data->push_params;
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "%s 释放语音识别通道成功\n", channel_name);
            break;
        }
        case SWITCH_ABC_TYPE_READ_REPLACE: {//读取到音频流
            switch_frame_t *frame;
            if ((frame = switch_core_media_bug_get_read_replace_frame(bug))) {
                int frame_len = frame->datalen;            //语音流长度
                char *frame_data = (char *) frame->data; //语音流内容

                switch_core_media_bug_set_read_replace_frame(bug, frame);

                if (frame->channels != 1) {
                    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "%s nonsupport channels: %d!\n", channel_name, frame->channels);
                    return SWITCH_FALSE;
                }

                user_data->push_params->set_parameter(bds::DATA_CHUNK, frame_data, frame_len);
                if (!user_data->sdk->post(*user_data->push_params, err_msg)) {
                    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "%s push audio data failed for %s\n", channel_name, err_msg.c_str());
                }
                // else
                // {
                // 	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "%s 发送数据成功 %s", frame_data, switch_channel_get_name(user_data->channel));
                // }
            }
            break;
        }
        default: {
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "default type=%s", to_string(type).c_str());
            break;
        }
    }
    return SWITCH_TRUE;
}

//执行这个app时的回调函数-下面的宏替换后如下
//static void start_baidu_asr_session_function (switch_core_session_t *session, 调用app时传的参数)
//static void start_baidu_asr_session_function (switch_core_session_t *session, const char *data)
SWITCH_STANDARD_APP(start_baidu_asr_session_function) {

    switch_channel_t *channel = switch_core_session_get_channel(session);

    switch_da_t *pvt;        //自定义参数
    switch_status_t status; //调用 switch_core_media_bug_add()的状态,用来判断断是否调用成功
    switch_codec_implementation_t read_impl;
    const char *channel_name = switch_channel_get_name(channel);

    //函数是内存赋值函数，用来给某一块内存空间进行赋值的
    memset(&read_impl, 0, sizeof(switch_codec_implementation_t));

    int argc;             //调用app时传参个数
    char *argv[5] = {0}; //调用app时传的参数 数组
    char *lbuf = nullptr;   //调用app时传的参数 字符串 会话内存池中的地址

    //zstr()//如果字符串为空或长度为零，则返回真值
    if (zstr(data)) {
        switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_WARNING, "%s 调用 baidu_asr 参数不能为空\n", channel_name);
        return;
    }

    //switch_core_session_strdup() 使用会话池中的内存分配简要复制字符串 返回指向新复制字符串的指针
    if (!(lbuf = switch_core_session_strdup(session, data))) {
        switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_WARNING, "%s 调用 baidu_asr 获取参数失败1\n", channel_name);
        return;
    }

    //switch_separate_string(要分割的字符串,分割符,拆分后的数组,数组中元素的最大数目) 将字符串分隔为数组,反回分割后数组的元素个数
    argc = switch_separate_string(lbuf, ' ', argv, (sizeof(argv) / sizeof(argv[0])));
    if (argc < 5) {
        switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_WARNING, "%s 调用 baidu_asr 获取参数个数不能小于4个\n", channel_name);
        return;
    }

    switch_core_session_get_read_impl(session, &read_impl);

    //为自定义数据从会话内存池分配内存
    //申请内存通道关闭后会自动释放
    //所以不需要手动释放
    if (!(pvt = (switch_da_t *) switch_core_session_alloc(session, sizeof(switch_da_t)))) {
        return;
    }

    pvt->stop = 0;
    pvt->app_id = argv[0];                   //识别系统Id
    pvt->chunk_key = argv[1];               //chunk_key
    pvt->secret_key = argv[2];               //secret_key
    pvt->product_id = argv[3];               //识别模型
    pvt->asr_finish_tags = false;           //线程是否结束识别 true已结束 未結束false
    pvt->vad_pause_frames = atof(argv[4]); //设置vad语句静音切分门限（帧）, 30帧 = 300ms
    pvt->session = session;
    pvt->channel = channel;

    /*  1 获取sdk实例   */
    std::string err_msg;
    bds::BDSpeechSDK *sdk = bds::BDSpeechSDK::get_instance(bds::SDK_TYPE_ASR, err_msg);
    if (!sdk) {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "%s get sdk failed for %s\n", channel_name, err_msg.c_str());
        return;
    }

    /*  2 设置输出回调  */
    sdk->set_event_listener(&asr_output_callback, pvt);

    /*  3 设置并发送sdk配置参数 */
    bds::BDSSDKMessage cfg_params;
    asr_set_config_params(cfg_params, pvt);
    if (!sdk->post(cfg_params, err_msg)) {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "%s init sdk failed for %s\n", channel_name, err_msg.c_str());
        bds::BDSpeechSDK::release_instance(sdk);
        return;
    }

    /*  4 设置并发送sdk启动参数 */
    bds::BDSSDKMessage start_params;
    asr_set_start_params(start_params);
    if (!sdk->post(start_params, err_msg)) {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "%s start sdk failed for %s\n", channel_name, err_msg.c_str());
        bds::BDSpeechSDK::release_instance(sdk);
        return;
    }

    //自定义数据
    pvt->sdk = sdk;                                                        //百度sdk对像
    pvt->push_params = new bds::BDSSDKMessage(bds::ASR_CMD_PUSH_AUDIO); //传输方式 二进制 及其长度 new的类需要用delete释放内存

    switch_media_bug_flag_t flags = SMBF_READ_REPLACE | SMBF_NO_PAUSE | SMBF_ONE_ONLY;

    //添加一个media bug到 session
    // switch_core_media_bug_add(session, "app名", NULL,switch_media_bug_callback_t callback,用户数据,停止时间,_In_ switch_media_bug_flag_t flags,_Out_ switch_media_bug_t **new_bug)
    if ((status = switch_core_media_bug_add(session, "baidu_asr", NULL, baidu_asr_callback, pvt, 0, flags, &(pvt->bug))) != SWITCH_STATUS_SUCCESS) {
        switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_WARNING, "%s Start baidu_asr media bug fail \n", channel_name);
        return;
    } else {
        switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_INFO, "%s Start baidu_asr media bug success \n", channel_name);
        switch_channel_set_private(channel, "baidu_asr", pvt);
    }
}

//模块加载时执行 宏替换后如下
//switch_status_t mod_baidu_asr_load(switch_loadable_module_interface_t **module_interface, switch_memory_pool_t *pool)
SWITCH_MODULE_LOAD_FUNCTION(mod_baidu_asr_load) {
    switch_application_interface_t *app_interface;

    //load时一定要先执行这一行
    *module_interface = switch_loadable_module_create_module_interface(pool, modname);

    //SWITCH_ADD_APP(app_interface,"模块名","短描述","长描述",调用模块时执行的回调函数,"看源码像是调用出错时的字符串")
    SWITCH_ADD_APP(app_interface, "baidu_asr", "baidu_asr_1", "baidu_asr_2", start_baidu_asr_session_function,
                   "baidu_asr调用出错7777777", SAF_MEDIA_TAP);
    return SWITCH_STATUS_SUCCESS;
}

//下面的代码是调用一个带参数的宏,这此宏定义在switch_type.h中,替换后如下一行
//switch_status_t mod_baidu_asr_shutdown (void)
//SWITCH_MODULE_LOAD_FUNCTION(卸载时执行的函数)
SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_baidu_asr_shutdown) {
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "baidu_asr_shutdown\n");
    bds::BDSpeechSDK::do_cleanup();//所有识别结束，不需要发起新的识别。SDK空闲时才能执行
    return SWITCH_STATUS_SUCCESS;
}
