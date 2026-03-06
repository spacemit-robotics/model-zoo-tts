/* Copyright (C) 2025 SpacemiT Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0 */

#include "text/phoneme_utils.hpp"

#include <string>
#include <utility>
#include <vector>

namespace tts {
namespace text {

// =============================================================================
// IPA phoneme conversion
// =============================================================================

std::string convertToGruutEnUs(const std::string& ipa) {
    std::string text = ipa;

    // First, remove zero-width joiner (U+200D) that espeak-ng sometimes adds
    {
        std::string zwj = "\xe2\x80\x8d";  // Zero-width joiner UTF-8
        size_t pos = 0;
        while ((pos = text.find(zwj, pos)) != std::string::npos) {
            text.erase(pos, zwj.length());
        }
    }

    // R-colored vowels (standard IPA -> Gruut US decomposed)
    std::vector<std::pair<std::string, std::string>> replacements = {
        // nurse
        {"\xc9\x9d", "\xc9\x9c\xc9\xb9"},
        // letter
        {"\xc9\x9a", "\xc9\x99\xc9\xb9"},

        // Diphthongs (diphthong -> single uppercase letter)
        // Must process longer patterns first
        {"e\xc9\xaa", "A"},   // face
        {"a\xc9\xaa", "I"},   // price
        {"\xc9\x94\xc9\xaa", "Y"},   // choice
        {"o\xca\x8a", "O"},   // goat (American)
        {"\xc9\x99\xca\x8a", "O"},   // goat (British compatibility)
        {"\xc9\x9b\xca\x8a", "O"},   // goat variant
        {"a\xca\x8a", "W"},   // mouth

        // Affricates
        {"t\xca\x83", "\xca\xa7"},   // cheese
        {"d\xca\x92", "\xca\xa4"},   // joy

        // Consonant normalization
        {"g", "\xc9\xa1"},    // Standard g -> Script g (U+0261)
        {"r", "\xc9\xb9"},    // Standard r -> Turned r (U+0279)
    };

    for (const auto& rep : replacements) {
        size_t pos = 0;
        while ((pos = text.find(rep.first, pos)) != std::string::npos) {
            text.replace(pos, rep.first.length(), rep.second);
            pos += rep.second.length();
        }
    }

    return text;
}

}  // namespace text
}  // namespace tts
