#pragma once

#include <cstring>

// Shared helpers for INI-style text file parsers (CITY.TXT, MAPS.TXT, etc.)

// Trim trailing spaces, tabs, and carriage returns in-place.
inline void trim_trailing(char* s) {
    int len = (int)strlen(s);
    while (len > 0 && (s[len - 1] == ' ' || s[len - 1] == '\t' || s[len - 1] == '\r')) {
        s[--len] = '\0';
    }
}

// Return true if line starts with prefix.
inline bool str_starts_with(const char* line, const char* prefix) {
    return strncmp(line, prefix, strlen(prefix)) == 0;
}

// Parse "On"/"Off" value (case-insensitive, skips leading whitespace).
inline bool parse_on_off(const char* val) {
    while (*val == ' ' || *val == '\t') {
        val++;
    }
    return (strncasecmp(val, "On", 2) == 0);
}

// Parse "Yes"/"No" value (case-insensitive, skips leading whitespace).
inline bool parse_yes_no(const char* val) {
    while (*val == ' ' || *val == '\t') {
        val++;
    }
    return (strncasecmp(val, "Yes", 3) == 0);
}
