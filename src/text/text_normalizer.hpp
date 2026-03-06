/* Copyright (C) 2025 SpacemiT Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0 */

#ifndef TEXT_NORMALIZER_HPP
#define TEXT_NORMALIZER_HPP

/**
 * TextNormalizer - 文本规范化模块
 *
 * 将数字、公式、货币、日期时间等特殊格式转换为可读文本。
 * 支持中文和英文两种语言输出。
 */

#include <cstdint>

#include <memory>
#include <regex>
#include <string>
#include <unordered_map>
#include <vector>

namespace tts {
namespace text {

// =============================================================================
// Language (语言)
// =============================================================================

enum class Language {
    ZH,         // 中文
    EN,         // 英文
    AUTO        // 自动检测 (根据上下文)
};

// =============================================================================
// NumberType (数字类型)
// =============================================================================

enum class NumberType {
    CARDINAL,       // 基数词: 123 -> "一百二十三" / "one hundred twenty-three"
    ORDINAL,        // 序数词: 第1 -> "第一" / "first"
    DIGIT,          // 逐位读: 123 -> "一二三" / "one two three"
    PHONE,          // 电话号码
    YEAR,           // 年份: 2024 -> "二零二四年" / "twenty twenty-four"
    DATE,           // 日期
    TIME,           // 时间
    PERCENTAGE,     // 百分比
    DECIMAL,        // 小数
    FRACTION,       // 分数
    CURRENCY,       // 货币
    RANGE,          // 范围: 10-20
    SCORE,          // 比分: 3:2
    UNKNOWN         // 未知
};

// =============================================================================
// NormalizerMatch (匹配结果)
// =============================================================================

struct NormalizerMatch {
    size_t start;           // 起始位置
    size_t length;          // 长度
    std::string original;   // 原始文本
    std::string normalized;  // 规范化后文本
    NumberType type;        // 类型 (用于数字)
};

// =============================================================================
// TextNormalizer (文本规范化器)
// =============================================================================

class TextNormalizer {
public:
    TextNormalizer();
    ~TextNormalizer();

    /**
     * @brief 规范化文本
     * @param text 输入文本
     * @param lang 目标语言 (ZH/EN/AUTO)
     * @return 规范化后的文本
     */
    std::string normalize(const std::string& text, Language lang = Language::AUTO);

    /**
     * @brief 设置默认语言
     * @param lang 语言
     */
    void setDefaultLanguage(Language lang);

    /**
     * @brief 获取默认语言
     */
    Language getDefaultLanguage() const { return default_lang_; }

private:
    Language default_lang_ = Language::AUTO;

    // 规范化处理
    std::string normalizeFormulas(const std::string& text, Language lang);
    std::string normalizeNumbers(const std::string& text, Language lang);
    std::string normalizeCurrency(const std::string& text, Language lang);
    std::string normalizeDateTime(const std::string& text, Language lang);
    std::string normalizeUnits(const std::string& text, Language lang);
    std::string normalizePhoneNumbers(const std::string& text, Language lang);
    std::string normalizePercentages(const std::string& text, Language lang);

    // 上下文检测
    Language detectLanguage(const std::string& text, size_t pos) const;
    NumberType detectNumberType(const std::string& text, size_t pos, size_t len) const;

    // 数字转换
    std::string numberToWords(int64_t num, Language lang) const;
    std::string numberToDigits(const std::string& num, Language lang) const;
    std::string decimalToWords(const std::string& decimal, Language lang) const;
    std::string fractionToWords(int numerator, int denominator, Language lang) const;
    std::string ordinalToWords(int num, Language lang) const;
    std::string yearToWords(int year, Language lang) const;

    // 辅助方法
    bool isPhoneNumber(const std::string& num) const;
    bool isCurrency(const std::string& text, size_t pos, size_t len) const;
    bool isYear(const std::string& num, const std::string& context, size_t pos) const;
    bool isDate(const std::string& text, size_t pos) const;
    bool isTime(const std::string& text, size_t pos) const;
    bool isPercentage(const std::string& text, size_t pos, size_t len) const;
    bool isScore(const std::string& text, size_t pos) const;
    bool isRange(const std::string& text, size_t pos) const;
};

// =============================================================================
// 便捷函数
// =============================================================================

/**
 * @brief 规范化文本 (便捷函数)
 * @param text 输入文本
 * @param lang 语言
 * @return 规范化后的文本
 */
std::string normalizeText(const std::string& text, Language lang = Language::AUTO);

}  // namespace text
}  // namespace tts

#endif  // TEXT_NORMALIZER_HPP
