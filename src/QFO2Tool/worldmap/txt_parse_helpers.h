#pragma once

#include <cctype>
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

// Case-insensitive substring search (portable replacement for GNU strcasestr).
inline bool str_contains_nocase(const char* haystack, const char* needle) {
    if (needle[0] == '\0') {
        return true;
    }
    for (; *haystack != '\0'; haystack++) {
        const char* h = haystack;
        const char* n = needle;
        while (*h != '\0' && *n != '\0' &&
               tolower((unsigned char)*h) == tolower((unsigned char)*n)) {
            h++;
            n++;
        }
        if (*n == '\0') {
            return true;
        }
    }
    return false;
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
