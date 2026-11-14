#pragma once
namespace hat {
void SetLanguage(int lang);   // 0 = English, 1 = Japanese
int  Language();
// Returns the translated string for the current language (falls back to the English text).
const char* tr(const char* en);
}
