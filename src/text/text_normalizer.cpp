/* Copyright (C) 2025 SpacemiT Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0 */

#include "text/text_normalizer.hpp"

#include <cctype>
#include <cmath>
#include <cstdint>

#include <algorithm>
#include <regex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "text/number_utils.hpp"
#include "text/text_utils.hpp"

namespace tts {
namespace text {

// =============================================================================
// 英文数字词表
// =============================================================================

namespace {

const char* ENGLISH_ONES[] = {
    "", "one", "two", "three", "four", "five",
    "six", "seven", "eight", "nine", "ten",
    "eleven", "twelve", "thirteen", "fourteen", "fifteen",
    "sixteen", "seventeen", "eighteen", "nineteen"
};

const char* ENGLISH_TENS[] = {
    "", "", "twenty", "thirty", "forty", "fifty",
    "sixty", "seventy", "eighty", "ninety"
};

const char* ENGLISH_ORDINALS[] = {
    "", "first", "second", "third", "fourth", "fifth",
    "sixth", "seventh", "eighth", "ninth", "tenth",
    "eleventh", "twelfth", "thirteenth", "fourteenth", "fifteenth",
    "sixteenth", "seventeenth", "eighteenth", "nineteenth"
};

const char* ENGLISH_DIGIT_NAMES[] = {
    "zero", "one", "two", "three", "four",
    "five", "six", "seven", "eight", "nine"
};

const char* CHINESE_DIGIT_NAMES[] = {
    "零", "一", "二", "三", "四",
    "五", "六", "七", "八", "九"
};

const char* ENGLISH_MONTHS[] = {
    "", "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"
};

const char* CHINESE_MONTHS[] = {
    "", "一", "二", "三", "四", "五", "六",
    "七", "八", "九", "十", "十一", "十二"
};

// 数学运算符映射
const std::unordered_map<std::string, std::pair<std::string, std::string>> MATH_OPERATORS = {
    {"+", {"加", "plus"}},
    {"-", {"减", "minus"}},
    {"−", {"减", "minus"}},  // Unicode minus
    {"*", {"乘", "times"}},
    {"×", {"乘", "times"}},
    {"÷", {"除以", "divided by"}},
    {"/", {"除以", "divided by"}},
    {"=", {"等于", "equals"}},
    {"≠", {"不等于", "not equal to"}},
    {">", {"大于", "greater than"}},
    {"<", {"小于", "less than"}},
    {"≥", {"大于等于", "greater than or equal to"}},
    {"≤", {"小于等于", "less than or equal to"}},
    {">=", {"大于等于", "greater than or equal to"}},
    {"<=", {"小于等于", "less than or equal to"}},
    {"^", {"的", "to the power of"}},
    {"√", {"根号", "square root of"}},
    {"±", {"正负", "plus or minus"}},
};

// 单位映射
const std::unordered_map<std::string, std::pair<std::string, std::string>> UNITS = {
    // 长度
    {"km", {"公里", "kilometers"}},
    {"m", {"米", "meters"}},
    {"cm", {"厘米", "centimeters"}},
    {"mm", {"毫米", "millimeters"}},
    {"mi", {"英里", "miles"}},
    {"ft", {"英尺", "feet"}},
    {"in", {"英寸", "inches"}},
    // 重量
    {"kg", {"公斤", "kilograms"}},
    {"g", {"克", "grams"}},
    {"mg", {"毫克", "milligrams"}},
    {"lb", {"磅", "pounds"}},
    {"oz", {"盎司", "ounces"}},
    // 容量
    {"L", {"升", "liters"}},
    {"l", {"升", "liters"}},
    {"ml", {"毫升", "milliliters"}},
    {"mL", {"毫升", "milliliters"}},
    // 温度
    {"°C", {"摄氏度", "degrees Celsius"}},
    {"°F", {"华氏度", "degrees Fahrenheit"}},
    {"℃", {"摄氏度", "degrees Celsius"}},
    {"℉", {"华氏度", "degrees Fahrenheit"}},
    // 面积
    {"m²", {"平方米", "square meters"}},
    {"km²", {"平方公里", "square kilometers"}},
    {"m2", {"平方米", "square meters"}},
    {"km2", {"平方公里", "square kilometers"}},
    // 速度
    {"km/h", {"公里每小时", "kilometers per hour"}},
    {"m/s", {"米每秒", "meters per second"}},
    {"mph", {"英里每小时", "miles per hour"}},
    // 数据
    {"KB", {"千字节", "kilobytes"}},
    {"MB", {"兆字节", "megabytes"}},
    {"GB", {"吉字节", "gigabytes"}},
    {"TB", {"太字节", "terabytes"}},
    {"Mbps", {"兆比特每秒", "megabits per second"}},
    {"Gbps", {"吉比特每秒", "gigabits per second"}},
};

// 货币符号映射
const std::unordered_map<std::string, std::pair<std::string, std::string>> CURRENCY_SYMBOLS = {
    {"¥", {"元", "yuan"}},
    {"￥", {"元", "yuan"}},
    {"$", {"美元", "dollars"}},
    {"€", {"欧元", "euros"}},
    {"£", {"英镑", "pounds"}},
    {"₩", {"韩元", "won"}},
    {"₹", {"卢比", "rupees"}},
};

// 货币后缀
const std::unordered_map<std::string, std::pair<std::string, std::string>> CURRENCY_SUFFIXES = {
    {"元", {"元", "yuan"}},
    {"块", {"块", "yuan"}},
    {"块钱", {"块钱", "yuan"}},
    {"美元", {"美元", "US dollars"}},
    {"美金", {"美金", "US dollars"}},
    {"人民币", {"人民币", "RMB"}},
};

}  // namespace

// =============================================================================
// 构造与析构
// =============================================================================

TextNormalizer::TextNormalizer() {}
TextNormalizer::~TextNormalizer() {}

// =============================================================================
// 主入口
// =============================================================================

std::string TextNormalizer::normalize(const std::string& text, Language lang) {
    if (text.empty()) return text;

    Language effective_lang = (lang == Language::AUTO) ? default_lang_ : lang;

    std::string result = text;

    // 按顺序处理各类特殊格式
    // 顺序很重要：先处理复合格式，再处理简单格式
    result = normalizeDateTime(result, effective_lang);   // 日期时间 (包含年份)
    result = normalizeCurrency(result, effective_lang);   // 货币
    result = normalizePhoneNumbers(result, effective_lang);  // 电话号码
    result = normalizePercentages(result, effective_lang);  // 百分比
    result = normalizeUnits(result, effective_lang);      // 单位
    result = normalizeFormulas(result, effective_lang);   // 公式
    result = normalizeNumbers(result, effective_lang);    // 普通数字

    return result;
}

void TextNormalizer::setDefaultLanguage(Language lang) {
    default_lang_ = lang;
}

// =============================================================================
// 公式规范化
// =============================================================================

std::string TextNormalizer::normalizeFormulas(const std::string& text, Language lang) {
    std::string result;
    auto chars = splitUtf8(text);

    for (size_t i = 0; i < chars.size(); ++i) {
        const std::string& ch = chars[i];

        // 检查双字符运算符
        if (i + 1 < chars.size()) {
            std::string two_chars = ch + chars[i + 1];
            auto it = MATH_OPERATORS.find(two_chars);
            if (it != MATH_OPERATORS.end()) {
                Language eff_lang = (lang == Language::AUTO)
                    ? detectLanguage(text, i) : lang;
                result += (eff_lang == Language::EN)
                    ? " " + it->second.second + " "
                    : it->second.first;
                ++i;  // 跳过下一个字符
                continue;
            }
        }

        // 检查单字符运算符
        auto it = MATH_OPERATORS.find(ch);
        if (it != MATH_OPERATORS.end()) {
            Language eff_lang = (lang == Language::AUTO)
                ? detectLanguage(text, i) : lang;

            // 特殊处理：检查 - 是否为负号
            bool is_negative = false;
            if (ch == "-" || ch == "−") {
                // 如果前面是运算符或起始位置，后面是数字，则为负号
                if (i + 1 < chars.size() && isDigit(chars[i + 1])) {
                    if (i == 0) {
                        is_negative = true;
                    } else {
                        const std::string& prev = chars[i - 1];
                        if (MATH_OPERATORS.count(prev) ||
                            prev == "(" || prev == "（" ||
                            prev == " ") {
                            is_negative = true;
                        }
                    }
                }
            }

            if (is_negative) {
                result += (eff_lang == Language::EN) ? "negative " : "负";
            } else {
                result += (eff_lang == Language::EN)
                    ? " " + it->second.second + " "
                    : it->second.first;
            }
        } else {
            result += ch;
        }
    }

    return result;
}

// =============================================================================
// 数字规范化
// =============================================================================

std::string TextNormalizer::normalizeNumbers(const std::string& text, Language lang) {
    // 正则匹配数字 (整数、小数、科学计数法)
    std::regex num_regex(R"((\d+\.?\d*(?:[eE][+-]?\d+)?))");

    std::string result;
    std::sregex_iterator it(text.begin(), text.end(), num_regex);
    std::sregex_iterator end;

    size_t last_pos = 0;
    while (it != end) {
        // 添加匹配前的文本
        result += text.substr(last_pos, it->position() - last_pos);

        std::string num_str = (*it)[1].str();
        size_t pos = it->position();

        // 确定语言
        Language eff_lang = (lang == Language::AUTO)
            ? detectLanguage(text, pos) : lang;

        // 检测数字类型
        NumberType type = detectNumberType(text, pos, num_str.length());

        std::string normalized;
        switch (type) {
            case NumberType::DECIMAL: {
                normalized = decimalToWords(num_str, eff_lang);
                break;
            }
            case NumberType::YEAR: {
                int year = std::stoi(num_str);
                normalized = yearToWords(year, eff_lang);
                break;
            }
            case NumberType::DIGIT:
            case NumberType::PHONE: {
                normalized = numberToDigits(num_str, eff_lang);
                break;
            }
            case NumberType::CARDINAL:
            default: {
                // 处理科学计数法
                size_t e_pos = num_str.find_first_of("eE");
                if (e_pos != std::string::npos) {
                    std::string mantissa = num_str.substr(0, e_pos);
                    std::string exponent = num_str.substr(e_pos + 1);
                    if (eff_lang == Language::EN) {
                        normalized = decimalToWords(mantissa, eff_lang) +
                            " times ten to the power of " +
                            numberToWords(std::stoll(exponent), eff_lang);
                    } else {
                        normalized = decimalToWords(mantissa, eff_lang) +
                            "乘以十的" +
                            numberToWords(std::stoll(exponent), eff_lang) +
                            "次方";
                    }
                } else if (num_str.find('.') != std::string::npos) {
                    normalized = decimalToWords(num_str, eff_lang);
                } else {
                    int64_t num = std::stoll(num_str);
                    normalized = numberToWords(num, eff_lang);
                }
                break;
            }
        }

        result += normalized;
        last_pos = pos + num_str.length();
        ++it;
    }

    // 添加剩余文本
    result += text.substr(last_pos);

    return result;
}

// =============================================================================
// 货币规范化
// =============================================================================

std::string TextNormalizer::normalizeCurrency(const std::string& text, Language lang) {
    std::string result;
    auto chars = splitUtf8(text);

    for (size_t i = 0; i < chars.size(); ++i) {
        const std::string& ch = chars[i];

        // 检查货币符号
        auto sym_it = CURRENCY_SYMBOLS.find(ch);
        if (sym_it != CURRENCY_SYMBOLS.end()) {
            // 提取后面的数字
            std::string num_str;
            size_t j = i + 1;
            bool has_decimal = false;
            while (j < chars.size()) {
                if (isDigit(chars[j])) {
                    num_str += chars[j];
                    ++j;
                } else if (chars[j] == "." && !has_decimal) {
                    num_str += chars[j];
                    has_decimal = true;
                    ++j;
                } else if (chars[j] == "," || chars[j] == "，") {
                    // 跳过千位分隔符
                    ++j;
                } else {
                    break;
                }
            }

            if (!num_str.empty()) {
                Language eff_lang = (lang == Language::AUTO)
                    ? detectLanguage(text, i) : lang;

                std::string amount;
                if (num_str.find('.') != std::string::npos) {
                    amount = decimalToWords(num_str, eff_lang);
                } else {
                    amount = numberToWords(std::stoll(num_str), eff_lang);
                }

                if (eff_lang == Language::EN) {
                    result += amount + " " + sym_it->second.second;
                } else {
                    result += amount + sym_it->second.first;
                }

                i = j - 1;  // 跳过已处理的数字
                continue;
            }
        }

        result += ch;
    }

    // 处理货币后缀 (如 "100元", "50块")
    std::regex currency_suffix_regex(R"((\d+(?:\.\d+)?)\s*(元|块|块钱|美元|美金|人民币))");
    std::smatch match;
    std::string temp = result;
    result.clear();

    std::sregex_iterator it(temp.begin(), temp.end(), currency_suffix_regex);
    std::sregex_iterator end;
    size_t last_pos = 0;

    while (it != end) {
        result += temp.substr(last_pos, it->position() - last_pos);

        std::string num_str = (*it)[1].str();
        std::string suffix = (*it)[2].str();

        Language eff_lang = (lang == Language::AUTO)
            ? detectLanguage(temp, it->position()) : lang;

        std::string amount;
        if (num_str.find('.') != std::string::npos) {
            amount = decimalToWords(num_str, eff_lang);
        } else {
            amount = numberToWords(std::stoll(num_str), eff_lang);
        }

        auto suf_it = CURRENCY_SUFFIXES.find(suffix);
        if (suf_it != CURRENCY_SUFFIXES.end()) {
            if (eff_lang == Language::EN) {
                result += amount + " " + suf_it->second.second;
            } else {
                result += amount + suf_it->second.first;
            }
        } else {
            result += amount + suffix;
        }

        last_pos = it->position() + it->length();
        ++it;
    }

    result += temp.substr(last_pos);

    return result;
}

// =============================================================================
// 日期时间规范化
// =============================================================================

std::string TextNormalizer::normalizeDateTime(const std::string& text, Language lang) {
    std::string result = text;

    // 匹配日期格式: YYYY-MM-DD 或 YYYY/MM/DD 或 YYYY年MM月DD日
    std::regex date_regex(R"((\d{4})[-/年](\d{1,2})[-/月](\d{1,2})日?)");
    std::smatch match;
    std::string temp;
    std::sregex_iterator it(result.begin(), result.end(), date_regex);
    std::sregex_iterator end;
    size_t last_pos = 0;

    while (it != end) {
        temp += result.substr(last_pos, it->position() - last_pos);

        int year = std::stoi((*it)[1].str());
        int month = std::stoi((*it)[2].str());
        int day = std::stoi((*it)[3].str());

        Language eff_lang = (lang == Language::AUTO)
            ? detectLanguage(result, it->position()) : lang;

        if (eff_lang == Language::EN) {
            // January 16th, 2024
            std::string day_ord = ordinalToWords(day, Language::EN);
            temp += std::string(ENGLISH_MONTHS[month]) + " " + day_ord + ", " +
                    yearToWords(year, Language::EN);
        } else {
            // 二零二四年一月十六日
            temp += yearToWords(year, Language::ZH) + "年" +
                    intToChineseReading(month) + "月" +
                    intToChineseReading(day) + "日";
        }

        last_pos = it->position() + it->length();
        ++it;
    }
    temp += result.substr(last_pos);
    result = temp;

    // 匹配时间格式: HH:MM 或 HH:MM:SS
    std::regex time_regex(R"((\d{1,2}):(\d{2})(?::(\d{2}))?)");
    temp.clear();
    it = std::sregex_iterator(result.begin(), result.end(), time_regex);
    last_pos = 0;

    while (it != end) {
        temp += result.substr(last_pos, it->position() - last_pos);

        int hour = std::stoi((*it)[1].str());
        int minute = std::stoi((*it)[2].str());
        int second = (*it)[3].matched ? std::stoi((*it)[3].str()) : -1;

        Language eff_lang = (lang == Language::AUTO)
            ? detectLanguage(result, it->position()) : lang;

        if (eff_lang == Language::EN) {
            // 12-hour format
            std::string period = (hour >= 12) ? "PM" : "AM";
            int hour12 = hour % 12;
            if (hour12 == 0) hour12 = 12;

            if (minute == 0) {
                temp += numberToWords(hour12, Language::EN) + " " + period;
            } else {
                temp += numberToWords(hour12, Language::EN) + " " +
                        numberToWords(minute, Language::EN) + " " + period;
            }
            if (second >= 0) {
                temp += " and " + numberToWords(second, Language::EN) + " seconds";
            }
        } else {
            // 十四点三十分
            temp += intToChineseReading(hour) + "点";
            if (minute > 0) {
                temp += intToChineseReading(minute) + "分";
            }
            if (second >= 0) {
                temp += intToChineseReading(second) + "秒";
            }
        }

        last_pos = it->position() + it->length();
        ++it;
    }
    temp += result.substr(last_pos);
    result = temp;

    // 匹配年份格式: N年 (4位数字+年)
    std::regex year_regex(R"((\d{4})年)");
    temp.clear();
    it = std::sregex_iterator(result.begin(), result.end(), year_regex);
    last_pos = 0;

    while (it != end) {
        temp += result.substr(last_pos, it->position() - last_pos);

        int year = std::stoi((*it)[1].str());
        Language eff_lang = (lang == Language::AUTO)
            ? detectLanguage(result, it->position()) : lang;

        temp += yearToWords(year, eff_lang) + (eff_lang == Language::EN ? "" : "年");

        last_pos = it->position() + it->length();
        ++it;
    }
    temp += result.substr(last_pos);

    return temp;
}

// =============================================================================
// 单位规范化
// =============================================================================

std::string TextNormalizer::normalizeUnits(const std::string& text, Language lang) {
    std::string result = text;

    // 按单位长度从长到短排序，避免短单位先匹配
    std::vector<std::pair<std::string, std::pair<std::string, std::string>>> sorted_units(
        UNITS.begin(), UNITS.end());
    std::sort(sorted_units.begin(), sorted_units.end(),
        [](const auto& a, const auto& b) {
            return a.first.length() > b.first.length();
        });

    for (const auto& [unit, names] : sorted_units) {
        // 匹配数字+单位
        std::regex unit_regex("(\\d+\\.?\\d*)" + std::string("(") + unit + ")");
        std::string temp;
        std::smatch match;
        std::sregex_iterator it(result.begin(), result.end(), unit_regex);
        std::sregex_iterator end;
        size_t last_pos = 0;

        while (it != end) {
            temp += result.substr(last_pos, it->position() - last_pos);

            std::string num_str = (*it)[1].str();
            Language eff_lang = (lang == Language::AUTO)
                ? detectLanguage(result, it->position()) : lang;

            std::string amount;
            if (num_str.find('.') != std::string::npos) {
                amount = decimalToWords(num_str, eff_lang);
            } else {
                amount = numberToWords(std::stoll(num_str), eff_lang);
            }

            if (eff_lang == Language::EN) {
                temp += amount + " " + names.second;
            } else {
                temp += amount + names.first;
            }

            last_pos = it->position() + it->length();
            ++it;
        }
        temp += result.substr(last_pos);
        result = temp;
    }

    return result;
}

// =============================================================================
// 电话号码规范化
// =============================================================================

std::string TextNormalizer::normalizePhoneNumbers(const std::string& text, Language lang) {
    // 匹配电话号码格式
    // - 11位数字 (如 13812345678)
    // - 带分隔符 (如 138-1234-5678, 138 1234 5678)
    // - 固话 (如 021-12345678, 010-12345678)
    std::regex phone_regex(
        R"(\b(1[3-9]\d{9})\b)"
        R"(|\b(1[3-9]\d[-\s]?\d{4}[-\s]?\d{4})\b)"
        R"(|\b(\d{3,4}[-\s]?\d{7,8})\b)");

    std::string result;
    std::sregex_iterator it(text.begin(), text.end(), phone_regex);
    std::sregex_iterator end;

    size_t last_pos = 0;
    while (it != end) {
        result += text.substr(last_pos, it->position() - last_pos);

        std::string phone = it->str();
        // 移除分隔符
        std::string digits_only;
        for (char c : phone) {
            if (std::isdigit(c)) {
                digits_only += c;
            }
        }

        Language eff_lang = (lang == Language::AUTO)
            ? detectLanguage(text, it->position()) : lang;

        result += numberToDigits(digits_only, eff_lang);

        last_pos = it->position() + it->length();
        ++it;
    }

    result += text.substr(last_pos);
    return result;
}

// =============================================================================
// 百分比规范化
// =============================================================================

std::string TextNormalizer::normalizePercentages(const std::string& text, Language lang) {
    std::regex percent_regex(R"((\d+\.?\d*)%)");

    std::string result;
    std::sregex_iterator it(text.begin(), text.end(), percent_regex);
    std::sregex_iterator end;

    size_t last_pos = 0;
    while (it != end) {
        result += text.substr(last_pos, it->position() - last_pos);

        std::string num_str = (*it)[1].str();
        Language eff_lang = (lang == Language::AUTO)
            ? detectLanguage(text, it->position()) : lang;

        std::string amount;
        if (num_str.find('.') != std::string::npos) {
            amount = decimalToWords(num_str, eff_lang);
        } else {
            amount = numberToWords(std::stoll(num_str), eff_lang);
        }

        if (eff_lang == Language::EN) {
            result += amount + " percent";
        } else {
            result += "百分之" + amount;
        }

        last_pos = it->position() + it->length();
        ++it;
    }

    result += text.substr(last_pos);
    return result;
}

// =============================================================================
// 上下文检测
// =============================================================================

Language TextNormalizer::detectLanguage(const std::string& text, size_t pos) const {
    // 检查周围的字符来判断语言
    auto chars = splitUtf8(text);

    // 统计中文和英文字符数量
    int chinese_count = 0;
    int english_count = 0;

    // 检查前后各10个字符
    int start = std::max(0, static_cast<int>(pos) - 10);
    int end_pos = std::min(chars.size(), pos + 10);

    for (size_t i = start; i < end_pos; ++i) {
        if (isChineseChar(chars[i])) {
            chinese_count++;
        } else if (isEnglishLetter(chars[i])) {
            english_count++;
        }
    }

    if (chinese_count > english_count) {
        return Language::ZH;
    } else if (english_count > chinese_count) {
        return Language::EN;
    }

    // 默认返回中文
    return Language::ZH;
}

NumberType TextNormalizer::detectNumberType(const std::string& text, size_t pos, size_t len) const {
    std::string num = text.substr(pos, len);

    // 检查是否包含小数点
    if (num.find('.') != std::string::npos) {
        return NumberType::DECIMAL;
    }

    // 检查是否为电话号码
    if (isPhoneNumber(num)) {
        return NumberType::PHONE;
    }

    // 检查是否为年份 (4位数字，1000-2999)
    if (len == 4) {
        int n = std::stoi(num);
        if (n >= 1000 && n <= 2999) {
            // 检查后面是否有"年"
            if (pos + len < text.length()) {
                auto chars = splitUtf8(text.substr(pos + len, 6));
                if (!chars.empty() && chars[0] == "年") {
                    return NumberType::YEAR;
                }
            }
        }
    }

    return NumberType::CARDINAL;
}

// =============================================================================
// 数字转换
// =============================================================================

std::string TextNormalizer::numberToWords(int64_t num, Language lang) const {
    if (lang == Language::EN) {
        if (num == 0) return "zero";
        if (num < 0) return "negative " + numberToWords(-num, lang);

        std::string result;

        if (num >= 1000000000000LL) {
            result += numberToWords(num / 1000000000000LL, lang) + " trillion ";
            num %= 1000000000000LL;
        }
        if (num >= 1000000000LL) {
            result += numberToWords(num / 1000000000LL, lang) + " billion ";
            num %= 1000000000LL;
        }
        if (num >= 1000000LL) {
            result += numberToWords(num / 1000000LL, lang) + " million ";
            num %= 1000000LL;
        }
        if (num >= 1000LL) {
            result += numberToWords(num / 1000LL, lang) + " thousand ";
            num %= 1000LL;
        }
        if (num >= 100LL) {
            result += std::string(ENGLISH_ONES[num / 100LL]) + " hundred ";
            num %= 100LL;
        }
        if (num >= 20LL) {
            result += std::string(ENGLISH_TENS[num / 10LL]);
            if (num % 10 != 0) {
                result += "-" + std::string(ENGLISH_ONES[num % 10]);
            }
            num = 0;
        }
        if (num > 0) {
            result += ENGLISH_ONES[num];
        }

        // 去除末尾空格
        while (!result.empty() && result.back() == ' ') {
            result.pop_back();
        }

        return result;
    } else {
        return intToChineseReading(num);
    }
}

std::string TextNormalizer::numberToDigits(const std::string& num, Language lang) const {
    std::string result;
    for (char c : num) {
        if (std::isdigit(c)) {
            int d = c - '0';
            if (!result.empty()) {
                result += (lang == Language::EN) ? " " : "";
            }
            result += (lang == Language::EN)
                ? ENGLISH_DIGIT_NAMES[d]
                : CHINESE_DIGIT_NAMES[d];
        }
    }
    return result;
}

std::string TextNormalizer::decimalToWords(const std::string& decimal, Language lang) const {
    size_t dot_pos = decimal.find('.');
    if (dot_pos == std::string::npos) {
        return numberToWords(std::stoll(decimal), lang);
    }

    std::string integer_part = decimal.substr(0, dot_pos);
    std::string decimal_part = decimal.substr(dot_pos + 1);

    std::string result;

    // 整数部分
    if (integer_part.empty() || integer_part == "0") {
        result = (lang == Language::EN) ? "zero" : "零";
    } else {
        result = numberToWords(std::stoll(integer_part), lang);
    }

    // 小数点
    result += (lang == Language::EN) ? " point" : "点";

    // 小数部分 (逐位读)
    for (char c : decimal_part) {
        if (std::isdigit(c)) {
            int d = c - '0';
            result += (lang == Language::EN)
                ? " " + std::string(ENGLISH_DIGIT_NAMES[d])
                : CHINESE_DIGIT_NAMES[d];
        }
    }

    return result;
}

std::string TextNormalizer::fractionToWords(int numerator, int denominator, Language lang) const {
    if (lang == Language::EN) {
        // 特殊情况
        if (denominator == 2) {
            if (numerator == 1) return "one half";
            return numberToWords(numerator, lang) + " halves";
        }
        if (denominator == 4) {
            if (numerator == 1) return "one quarter";
            return numberToWords(numerator, lang) + " quarters";
        }

        std::string denom_ord = ordinalToWords(denominator, lang);
        if (numerator == 1) {
            return "one " + denom_ord;
        }
        return numberToWords(numerator, lang) + " " + denom_ord + "s";
    } else {
        return intToChineseReading(denominator) + "分之" + intToChineseReading(numerator);
    }
}

std::string TextNormalizer::ordinalToWords(int num, Language lang) const {
    if (lang == Language::EN) {
        if (num >= 1 && num < 20) {
            return ENGLISH_ORDINALS[num];
        }

        std::string base = numberToWords(num, lang);

        // 处理以 1, 2, 3 结尾的特殊情况
        if (num % 10 == 1 && num % 100 != 11) {
            // 去掉末尾的 "one" 并加 "first"
            if (base.length() > 4 && base.substr(base.length() - 4) == "-one") {
                return base.substr(0, base.length() - 3) + "first";
            } else if (base.length() > 3 && base.substr(base.length() - 3) == "one") {
                return base.substr(0, base.length() - 3) + "first";
            }
        } else if (num % 10 == 2 && num % 100 != 12) {
            if (base.length() > 4 && base.substr(base.length() - 4) == "-two") {
                return base.substr(0, base.length() - 3) + "second";
            } else if (base.length() > 3 && base.substr(base.length() - 3) == "two") {
                return base.substr(0, base.length() - 3) + "second";
            }
        } else if (num % 10 == 3 && num % 100 != 13) {
            if (base.length() > 6 && base.substr(base.length() - 6) == "-three") {
                return base.substr(0, base.length() - 5) + "third";
            } else if (base.length() > 5 && base.substr(base.length() - 5) == "three") {
                return base.substr(0, base.length() - 5) + "third";
            }
        }

        // 默认：去掉末尾的 y 加 ieth，或直接加 th
        if (base.back() == 'y') {
            return base.substr(0, base.length() - 1) + "ieth";
        }
        return base + "th";
    } else {
        return "第" + intToChineseReading(num);
    }
}

std::string TextNormalizer::yearToWords(int year, Language lang) const {
    if (lang == Language::EN) {
        // 英文年份读法
        if (year >= 2000 && year < 2010) {
            return "two thousand " + (year == 2000 ? "" :
                "and " + numberToWords(year - 2000, lang));
        } else if (year >= 2010 && year < 2100) {
            int first = year / 100;
            int second = year % 100;
            return numberToWords(first, lang) + " " + numberToWords(second, lang);
        } else if (year >= 1000 && year < 2000) {
            int first = year / 100;
            int second = year % 100;
            if (second == 0) {
                return numberToWords(first, lang) + " hundred";
            } else if (second < 10) {
                return numberToWords(first, lang) + " oh " + numberToWords(second, lang);
            }
            return numberToWords(first, lang) + " " + numberToWords(second, lang);
        }
        return numberToWords(year, lang);
    } else {
        // 中文年份：逐位读
        std::string result;
        std::string year_str = std::to_string(year);
        for (char c : year_str) {
            int d = c - '0';
            result += CHINESE_DIGIT_NAMES[d];
        }
        return result;
    }
}

// =============================================================================
// 辅助方法
// =============================================================================

bool TextNormalizer::isPhoneNumber(const std::string& num) const {
    // 移除分隔符
    std::string digits;
    for (char c : num) {
        if (std::isdigit(c)) {
            digits += c;
        }
    }

    // 11位手机号，以1开头，第二位是3-9
    if (digits.length() == 11 && digits[0] == '1' &&
        digits[1] >= '3' && digits[1] <= '9') {
        return true;
    }

    // 固话：7-8位数字，或3-4位区号+7-8位号码
    if (digits.length() >= 7 && digits.length() <= 12) {
        // 检查是否以常见区号开头
        if ((digits.length() == 11 || digits.length() == 12) &&
            (digits.substr(0, 3) == "010" || digits.substr(0, 3) == "021" ||
            digits.substr(0, 3) == "020" || digits.substr(0, 3) == "025")) {
            return true;
        }
    }

    return false;
}

bool TextNormalizer::isCurrency(const std::string& text, size_t pos, size_t len) const {
    // 检查前面是否有货币符号
    if (pos > 0) {
        auto chars = splitUtf8(text.substr(0, pos));
        if (!chars.empty()) {
            const std::string& prev = chars.back();
            if (CURRENCY_SYMBOLS.count(prev)) {
                return true;
            }
        }
    }

    // 检查后面是否有货币后缀
    if (pos + len < text.length()) {
        auto chars = splitUtf8(text.substr(pos + len, 6));
        if (!chars.empty()) {
            std::string suffix;
            for (const auto& c : chars) {
                suffix += c;
                if (CURRENCY_SUFFIXES.count(suffix)) {
                    return true;
                }
            }
        }
    }

    return false;
}

bool TextNormalizer::isYear(const std::string& num, const std::string& context, size_t pos) const {
    if (num.length() != 4) return false;

    int n = std::stoi(num);
    if (n < 1000 || n > 2999) return false;

    // 检查后面是否有"年"
    if (pos + 4 < context.length()) {
        auto chars = splitUtf8(context.substr(pos + 4, 3));
        if (!chars.empty() && chars[0] == "年") {
            return true;
        }
    }

    return false;
}

bool TextNormalizer::isDate(const std::string& text, size_t pos) const {
    // 简化实现：检查 YYYY-MM-DD 或 YYYY/MM/DD 格式
    if (pos + 10 > text.length()) return false;

    std::string substr = text.substr(pos, 10);
    std::regex date_regex(R"(\d{4}[-/]\d{2}[-/]\d{2})");
    return std::regex_match(substr, date_regex);
}

bool TextNormalizer::isTime(const std::string& text, size_t pos) const {
    // 检查 HH:MM 或 HH:MM:SS 格式
    if (pos + 5 > text.length()) return false;

    std::string substr = text.substr(pos, 8);
    std::regex time_regex(R"(\d{1,2}:\d{2}(:\d{2})?)");
    return std::regex_match(substr, time_regex);
}

bool TextNormalizer::isPercentage(const std::string& text, size_t pos, size_t len) const {
    if (pos + len < text.length()) {
        return text[pos + len] == '%';
    }
    return false;
}

bool TextNormalizer::isScore(const std::string& text, size_t pos) const {
    // 检查 N:N 格式
    std::regex score_regex(R"(\d+:\d+)");
    std::smatch match;
    std::string substr = text.substr(pos);
    return std::regex_search(substr, match, score_regex) && match.position() == 0;
}

bool TextNormalizer::isRange(const std::string& text, size_t pos) const {
    // 检查 N-N 格式
    std::regex range_regex(R"(\d+-\d+)");
    std::smatch match;
    std::string substr = text.substr(pos);
    return std::regex_search(substr, match, range_regex) && match.position() == 0;
}

// =============================================================================
// 便捷函数
// =============================================================================

std::string normalizeText(const std::string& text, Language lang) {
    static TextNormalizer normalizer;
    return normalizer.normalize(text, lang);
}

}  // namespace text
}  // namespace tts
