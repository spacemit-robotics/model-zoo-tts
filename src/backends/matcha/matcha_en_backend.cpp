/* Copyright (C) 2025 SpacemiT Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0 */

#include "backends/matcha/matcha_en_backend.hpp"

#include <cstdint>

#include <iostream>
#include <string>
#include <vector>

#include "text/phoneme_utils.hpp"
#include "text/text_utils.hpp"

namespace tts {

// =============================================================================
// 构造与析构
// =============================================================================

MatchaEnBackend::MatchaEnBackend()
    : MatchaBackend(BackendType::MATCHA_EN)
    , espeak_initialized_(false) {
}

MatchaEnBackend::~MatchaEnBackend() {
    shutdownLanguageSpecific();
}

// =============================================================================
// MatchaBackend 纯虚方法实现
// =============================================================================

std::string MatchaEnBackend::getModelSubdir() const {
    return "matcha-icefall-en_US-ljspeech";
}

bool MatchaEnBackend::usesBlankTokens() const {
    return true;
}

ErrorInfo MatchaEnBackend::initializeLanguageSpecific(const TtsConfig& config) {
    // 检查 espeak-ng 是否可用
    if (!checkEspeakNgAvailable()) {
        return ErrorInfo::error(ErrorCode::INTERNAL_ERROR,
            "espeak-ng is required for English TTS but not available. "
            "Please install: brew install espeak-ng (macOS) or apt-get install espeak-ng (Linux)");
    }

    espeak_initialized_ = true;
    std::cout << "Info: espeak-ng found and available for English TTS." << std::endl;

    return ErrorInfo::ok();
}

void MatchaEnBackend::shutdownLanguageSpecific() {
    espeak_initialized_ = false;
}

// =============================================================================
// 初始化 espeak-ng
// =============================================================================

void MatchaEnBackend::initializeEspeak() {
    // espeak-ng 通过命令行调用，不需要特殊初始化
    espeak_initialized_ = checkEspeakNgAvailable();
}

// =============================================================================
// 文本转 Token IDs (英文)
// =============================================================================

std::vector<int64_t> MatchaEnBackend::textToTokenIds(const std::string& text) {
    std::vector<int64_t> token_ids;
    const auto& token_to_id = getTokenToIdMap();

    // 检查是否包含中文字符 - 如果包含则跳过
    if (text::containsChinese(text)) {
        return token_ids;  // 静默跳过中文
    }

    // 使用 espeak-ng 获取 IPA 音素
    std::string phonemes = processEnglishTextToPhonemes(text);
    if (phonemes.empty() && !text.empty()) {
        std::cerr << "Error: espeak-ng failed to process text" << std::endl;
        return token_ids;
    }

    // 转换为 Gruut US 格式
    std::string gruut_phonemes = text::convertToGruutEnUs(phonemes);

    // 添加开始 token (^) - sherpa-onnx 风格
    auto start_it = token_to_id.find("^");
    if (start_it != token_to_id.end()) {
        token_ids.push_back(start_it->second);
    }

    // 处理音素字符
    std::vector<std::string> phoneme_chars = text::splitUtf8(gruut_phonemes);
    bool last_was_space = false;

    for (const auto& phoneme_char : phoneme_chars) {
        if (phoneme_char.empty()) continue;

        // 过滤问题字符
        if (phoneme_char == "\u200D" ||  // Zero-width joiner
            phoneme_char == "\u200C" ||  // Zero-width non-joiner
            phoneme_char == "\uFEFF" ||  // Byte order mark
            phoneme_char == "\u00A0" ||  // Non-breaking space
            (phoneme_char.size() == 1 &&
            std::iscntrl(static_cast<unsigned char>(phoneme_char[0])))) {
            continue;
        }

        // 处理空格 - 限制连续空格
        if (phoneme_char == " ") {
            if (last_was_space) {
                continue;
            }
            last_was_space = true;
        } else {
            last_was_space = false;
        }

        auto token_it = token_to_id.find(phoneme_char);
        if (token_it != token_to_id.end()) {
            token_ids.push_back(token_it->second);
        } else if (phoneme_char != " ") {
            // 只记录非空格的未知 token
            std::cerr << "Warning: Unknown phoneme token: '" << phoneme_char << "'" << std::endl;
        }
    }

    // 添加结束 token ($) - sherpa-onnx 风格
    auto end_it = token_to_id.find("$");
    if (end_it != token_to_id.end()) {
        token_ids.push_back(end_it->second);
    }

    return token_ids;
}

}  // namespace tts
