#pragma once

#include <string>

/**
 * 语音服务接口类
 * 定义统一的语音处理接口，支持不同提供商的实现
 */
class SpeechService {
public:
    virtual ~SpeechService() = default;
    
    /**
     * 语音识别
     * @param speechData 语音数据
     * @param format 音频格式 (如: pcm, wav等)
     * @param rate 采样率
     * @param channel 声道数
     * @return 识别结果文本
     */
    virtual std::string recognize(const std::string& speechData, 
                                const std::string& format = "pcm", 
                                int rate = 16000, 
                                int channel = 1) = 0;
    
    /**
     * 语音合成
     * @param text 待合成文本
     * @param format 输出音频格式 (如: mp3-16k, wav等)
     * @param lang 语言
     * @param speed 语速 (0-15, 默认5)
     * @param pitch 音调 (0-15, 默认5)
     * @param volume 音量 (0-15, 默认5)
     * @return 音频文件URL或数据
     */
    virtual std::string synthesize(const std::string& text,
                                  const std::string& format = "mp3-16k",
                                  const std::string& lang = "zh",
                                  int speed = 5,
                                  int pitch = 5,
                                  int volume = 5) = 0;
};