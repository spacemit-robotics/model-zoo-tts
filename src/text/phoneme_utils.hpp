/* Copyright (C) 2025 SpacemiT Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0 */

#ifndef PHONEME_UTILS_HPP
#define PHONEME_UTILS_HPP

/**
 * PhonemeUtils - 音素处理工具模块
 *
 * 提供 IPA 音素转换等功能。
 */

#include <string>

namespace tts {
namespace text {

// =============================================================================
// IPA 音素转换
// =============================================================================

/**
 * @brief 将 espeak-ng IPA 转换为 gruut en-us 格式
 * @param ipa espeak-ng 输出的 IPA 字符串
 * @return gruut en-us 格式的音素字符串
 *
 * 转换规则：
 * - 移除零宽连接符 (U+200D)
 * - R色元音分解：ɝ -> ɜɹ, ɚ -> əɹ
 * - 双元音合并：eɪ -> A, aɪ -> I, ɔɪ -> Y, oʊ -> O, aʊ -> W
 * - 塞擦音：tʃ -> ʧ, dʒ -> ʤ
 * - 辅音标准化：g -> ɡ, r -> ɹ
 */
std::string convertToGruutEnUs(const std::string& ipa);

}  // namespace text
}  // namespace tts

#endif  // PHONEME_UTILS_HPP
