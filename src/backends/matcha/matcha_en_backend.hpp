/* Copyright (C) 2025 SpacemiT Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0 */

#ifndef MATCHA_EN_BACKEND_HPP
#define MATCHA_EN_BACKEND_HPP

#include <cstdint>

#include <string>
#include <vector>

#include "backends/matcha/matcha_backend.hpp"

namespace tts {

// =============================================================================
// Matcha English Backend
// =============================================================================
//
// 英文 TTS 后端实现。
// 使用 espeak-ng 生成 IPA 音素，转换为 Gruut US 格式。
//
// 模型: matcha-icefall-en_US-ljspeech
// 采样率: 22050 Hz
// 特点: 使用 blank tokens
//

class MatchaEnBackend : public MatchaBackend {
public:
    MatchaEnBackend();
    ~MatchaEnBackend() override;

protected:
    // -------------------------------------------------------------------------
    // MatchaBackend 纯虚方法实现
    // -------------------------------------------------------------------------

    std::vector<int64_t> textToTokenIds(const std::string& text) override;
    std::string getModelSubdir() const override;
    bool usesBlankTokens() const override;
    ErrorInfo initializeLanguageSpecific(const TtsConfig& config) override;
    void shutdownLanguageSpecific() override;

private:
    // -------------------------------------------------------------------------
    // 英文文本处理
    // -------------------------------------------------------------------------

    /// @brief 初始化 espeak-ng
    void initializeEspeak();

    // -------------------------------------------------------------------------
    // 成员变量
    // -------------------------------------------------------------------------

    bool espeak_initialized_ = false;
};

}  // namespace tts

#endif  // MATCHA_EN_BACKEND_HPP
