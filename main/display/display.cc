#include <esp_log.h>
#include <esp_err.h>
#include <string>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <font_awesome.h>
#include <unordered_map>

#include "display.h"
#include "board.h"
#include "application.h"
#include "audio_codec.h"
#include "settings.h"
#include "assets/lang_config.h"

#define TAG "Display"

Display::Display() {
}

Display::~Display() {
}

std::string ToBanglish(const char* text) {
    static const std::unordered_map<uint32_t, const char*> letters = {
        {0x0985, "o"}, {0x0986, "a"}, {0x0987, "i"}, {0x0988, "i"},
        {0x0989, "u"}, {0x098A, "u"}, {0x098B, "ri"}, {0x098F, "e"},
        {0x0990, "oi"}, {0x0993, "o"}, {0x0994, "ou"}, {0x0995, "k"},
        {0x0996, "kh"}, {0x0997, "g"}, {0x0998, "gh"}, {0x0999, "ng"},
        {0x099A, "ch"}, {0x099B, "chh"}, {0x099C, "j"}, {0x099D, "jh"},
        {0x099E, "n"}, {0x099F, "t"}, {0x09A0, "th"}, {0x09A1, "d"},
        {0x09A2, "dh"}, {0x09A3, "n"}, {0x09A4, "t"}, {0x09A5, "th"},
        {0x09A6, "d"}, {0x09A7, "dh"}, {0x09A8, "n"}, {0x09AA, "p"},
        {0x09AB, "ph"}, {0x09AC, "b"}, {0x09AD, "bh"}, {0x09AE, "m"},
        {0x09AF, "y"}, {0x09B0, "r"}, {0x09B2, "l"}, {0x09B6, "sh"},
        {0x09B7, "sh"}, {0x09B8, "s"}, {0x09B9, "h"}, {0x09DC, "r"},
        {0x09DD, "rh"}, {0x09DF, "y"}, {0x09CE, "t"}, {0x09A6, "d"}
    };
    static const std::unordered_map<uint32_t, const char*> marks = {
        {0x09BE, "a"}, {0x09BF, "i"}, {0x09C0, "i"}, {0x09C1, "u"},
        {0x09C2, "u"}, {0x09C3, "ri"}, {0x09C7, "e"}, {0x09C8, "oi"},
        {0x09CB, "o"}, {0x09CC, "ou"}, {0x0982, "ng"}, {0x0983, "h"},
        {0x0981, "n"}
    };
    std::string result;
    std::string current_word;
    bool pending_vowel = false;
    bool word_had_bengali = false;
    auto flush_word = [&]() {
        if (!current_word.empty() && current_word.size() >= 2 &&
            current_word.back() == 'a') {
            current_word.pop_back();
        }
        if (word_had_bengali && current_word == "halo") current_word = "holo";
        result += current_word;
        current_word.clear();
        word_had_bengali = false;
    };

    const unsigned char* cursor = reinterpret_cast<const unsigned char*>(text ? text : "");
    while (*cursor != '\0') {
        uint32_t codepoint = *cursor++;
        if (codepoint >= 0xC0) {
            if ((codepoint & 0xE0) == 0xC0) {
                const uint32_t next = *cursor++;
                codepoint = (codepoint & 0x1F) << 6 | (next & 0x3F);
            } else if ((codepoint & 0xF0) == 0xE0) {
                const uint32_t next1 = *cursor++;
                const uint32_t next2 = *cursor++;
                codepoint = (codepoint & 0x0F) << 12 | (next1 & 0x3F) << 6 | (next2 & 0x3F);
            } else if ((codepoint & 0xF8) == 0xF0) {
                const uint32_t next1 = *cursor++;
                const uint32_t next2 = *cursor++;
                const uint32_t next3 = *cursor++;
                codepoint = (codepoint & 0x07) << 18 | (next1 & 0x3F) << 12 |
                            (next2 & 0x3F) << 6 | (next3 & 0x3F);
            }
        }

        if (codepoint >= 0x09E6 && codepoint <= 0x09EF) {
            word_had_bengali = true;
            current_word += static_cast<char>('0' + codepoint - 0x09E6);
        } else if (codepoint == 0x09CD) {
            word_had_bengali = true;
            if (!current_word.empty() && current_word.back() == 'a') current_word.pop_back();
            pending_vowel = true;
        } else if (marks.count(codepoint)) {
            word_had_bengali = true;
            if (!current_word.empty() && current_word.back() == 'a') current_word.pop_back();
            current_word += marks.at(codepoint);
            pending_vowel = false;
        } else if (letters.count(codepoint)) {
            word_had_bengali = true;
            current_word += letters.at(codepoint);
            if (!pending_vowel && codepoint != 0x09DF) current_word += 'a';
            pending_vowel = false;
        } else if (codepoint >= 0x0980 && codepoint <= 0x09FF) {
            current_word += '?';
        } else if (codepoint < 0x80 && (isalnum(codepoint) || codepoint == '_')) {
            current_word += static_cast<char>(codepoint);
        } else {
            flush_word();
            result += static_cast<char>(codepoint < 0x80 ? codepoint : ' ');
        }
        if (codepoint == ' ' || codepoint == '\n' || codepoint == '\t') flush_word();
    }
    flush_word();
    return result;
}

void Display::SetStatus(const char* status) {
    ESP_LOGW(TAG, "SetStatus: %s", status);
}

void Display::ShowNotification(const std::string &notification, int duration_ms) {
    ShowNotification(notification.c_str(), duration_ms);
}

void Display::ShowNotification(const char* notification, int duration_ms) {
    ESP_LOGW(TAG, "ShowNotification: %s", notification);
}

void Display::UpdateStatusBar(bool update_all) {
}


void Display::SetEmotion(const char* emotion) {
    ESP_LOGW(TAG, "SetEmotion: %s", emotion);
}

void Display::SetChatMessage(const char* role, const char* content) {
    ESP_LOGW(TAG, "Role:%s", role);
    ESP_LOGW(TAG, "     %s", content);
}

void Display::ClearChatMessages() {
    // Default empty implementation, override in subclasses if needed
}

void Display::SetTheme(Theme* theme) {
    current_theme_ = theme;
    Settings settings("display", true);
    settings.SetString("theme", theme->name());
}

void Display::SetPowerSaveMode(bool on) {
    ESP_LOGW(TAG, "SetPowerSaveMode: %d", on);
}
