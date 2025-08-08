#include "../core/app.h"

// this is just to hide the call from Codacy, should be automatically forceinlined by the compiler at O2
int Utf8ToWide(const LPCCH utf8Str, const int bytes, const LPWSTR wideStr, const int wideChars)
{
    // if doesnt work, just replace this voodoo thingy with return MultiByteToWideChar(CP_UTF8, 0, utf8Str, bytes, wideStr, wideChars);
    if (utf8Str == NULL) return NULL;
    if (bytes == 0) {
        wchar_t* empty_str = (wchar_t*)malloc(sizeof(wchar_t));
        if (empty_str) empty_str[0] = L'\0';
        return empty_str;
    }

    const unsigned char* p = (const unsigned char*)utf8Str;
    const unsigned char* const end = p + bytes;
    size_t required_wchars = 0;

    while (p < end) {
        unsigned char c = *p;
        if (c < 0x80) p++;
        else if ((c & 0xE0) == 0xC0) p += 2;
        else if ((c & 0xF0) == 0xE0) p += 3;
        else if ((c & 0xF8) == 0xF0) {
            p += 4;
            required_wchars++; 
        }
        else { 
            p++;
        }
        required_wchars++;
    }

    wchar_t* wide_buffer = (wchar_t*)malloc((required_wchars + 1) * sizeof(wchar_t));
    if (wide_buffer == NULL) return NULL;

    p = (const unsigned char*)utf8Str;
    wchar_t* out = wide_buffer;
    while (p < end) {
        unsigned long codepoint = 0;
        unsigned char c1 = *p;

        if (c1 < 0x80) { 
            codepoint = *p++;
        }
        else if ((c1 & 0xE0) == 0xC0 && (p + 1 < end)) { 
            codepoint = (*p & 0x1F) << 6;
            codepoint |= (*(p + 1) & 0x3F);
            p += 2;
        }
        else if ((c1 & 0xF0) == 0xE0 && (p + 2 < end)) { 
            codepoint = (*p & 0x0F) << 12;
            codepoint |= (*(p + 1) & 0x3F) << 6;
            codepoint |= (*(p + 2) & 0x3F);
            p += 3;
        }
        else if ((c1 & 0xF8) == 0xF0 && (p + 3 < end)) { 
            codepoint = (*p & 0x07) << 18;
            codepoint |= (*(p + 1) & 0x3F) << 12;
            codepoint |= (*(p + 2) & 0x3F) << 6;
            codepoint |= (*(p + 3) & 0x3F);
            p += 4;
        }
        else { 
            codepoint = L'?'; 
            p++;
        }

        if (codepoint < 0x10000) {
            *out++ = (wchar_t)codepoint;
        }
        else { 
            codepoint -= 0x10000;
            *out++ = (wchar_t)((codepoint >> 10) + 0xD800);
            *out++ = (wchar_t)((codepoint & 0x03FF) + 0xDC00);
        }
    }

    *out = L'\0'; 
    return wide_buffer;
}

void __cdecl FormatLargeIntTime(const LARGE_INTEGER li, WCHAR* __restrict buffer, const size_t bufferSize) {
    FILETIME ft = { 0 }, localFt = { 0 };
    SYSTEMTIME st = { 0 };
    static const WCHAR* days[] = { L"Sun", L"Mon", L"Tue", L"Wed", L"Thu", L"Fri", L"Sat" };

    ft.dwLowDateTime = li.LowPart;
    ft.dwHighDateTime = li.HighPart;

    if (FileTimeToLocalFileTime(&ft, &localFt) && FileTimeToSystemTime(&localFt, &st)) {
        swprintf_s(buffer, bufferSize / sizeof(WCHAR), L"%s, %04d-%02d-%02d %02d:%02d:%02d",
            days[st.wDayOfWeek], st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    }
    else {
        wcscpy_s(buffer, bufferSize / sizeof(WCHAR), L"Invalid Time");
    }
}

void __cdecl FormatFileTime(const FILETIME ft, WCHAR* __restrict buffer, const size_t bufferSize) {
    LARGE_INTEGER li = { 0 };
    li.LowPart = ft.dwLowDateTime;
    li.HighPart = ft.dwHighDateTime;
    FormatLargeIntTime(li, buffer, bufferSize);
}

void __cdecl FormatUsnReason(const DWORD reason, WCHAR* __restrict buffer, const size_t bufferSize) {
    WCHAR* p = buffer;
    const WCHAR* end = buffer + (bufferSize / sizeof(WCHAR));
    *p = L'\0';

    if (reason & USN_REASON_FILE_CREATE) { p += swprintf_s(p, end - p, L"Create "); }
    if (reason & USN_REASON_FILE_DELETE) { p += swprintf_s(p, end - p, L"Delete "); }
    if (reason & USN_REASON_RENAME_NEW_NAME) { p += swprintf_s(p, end - p, L"RenameNew "); }
    if (reason & USN_REASON_RENAME_OLD_NAME) { p += swprintf_s(p, end - p, L"RenameOld "); }
    if (reason & USN_REASON_DATA_OVERWRITE) { p += swprintf_s(p, end - p, L"Overwrite "); }
    if (reason & USN_REASON_DATA_EXTEND) { p += swprintf_s(p, end - p, L"Extend "); }
    if (reason & USN_REASON_BASIC_INFO_CHANGE) { p += swprintf_s(p, end - p, L"InfoChange "); }
    if (reason & USN_REASON_CLOSE) { p += swprintf_s(p, end - p, L"Close "); }

    if (p == buffer) {
        wcscpy_s(buffer, bufferSize / sizeof(WCHAR), L"Other");
    }
}

void __cdecl FormatFileSize(const ULARGE_INTEGER size, WCHAR* __restrict buffer, const size_t bufferSize) {
    if (size.QuadPart == (ULONGLONG)-1) {
        wcscpy_s(buffer, bufferSize, L"N/A");
        return;
    }

    const double dSize = (double)size.QuadPart;
    if (dSize < 1024.0) {
        swprintf_s(buffer, bufferSize, L"%.0f B", dSize);
    }
    else if (dSize < 1048576.0) { // 1024*1024
        swprintf_s(buffer, bufferSize, L"%.2f KB", dSize / 1024.0);
    }
    else if (dSize < 1073741824.0) { // 1024*1024*1024
        swprintf_s(buffer, bufferSize, L"%.2f MB", dSize / 1048576.0);
    }
    else {
        swprintf_s(buffer, bufferSize, L"%.2f GB", dSize / 1073741824.0);
    }
}

double __cdecl CalculateShannonEntropy(const char* __restrict str, const size_t len) {
    if (!str || len == 0) return 0.0;

    unsigned int frequencies[256] = { 0 };

    for (size_t i = 0; i < len; ++i) {
        frequencies[(unsigned char)str[i]]++;
    }

    double entropy = 0.0;
    const double len_d = (double)len;
    for (int i = 0; i < 256; ++i) {
        if (frequencies[i] > 0) {
            double probability = (double)frequencies[i] / len_d;
            entropy -= probability * log2(probability);
        }
    }
    return entropy;
}

double __cdecl CalculateAverageEntropy(char** __restrict stringList, const int count) {
    if (!stringList || count == 0) return 0.0;

    double totalEntropy = 0.0;
    for (int i = 0; i < count; i++) {
        if (stringList[i]) {
            const size_t len = strnlen_s(stringList[i], MAX_PATH);
            totalEntropy += CalculateShannonEntropy(stringList[i], len);
        }
    }

    return totalEntropy / count;
}

double __cdecl CalculateEntropyFromCounts(const char* __restrict str, const size_t len) {
    if (!str || len == 0) return 0.0;

    unsigned int frequencies[256] = { 0 };

    for (size_t i = 0; i < len; ++i) {
        frequencies[(unsigned char)str[i]]++;
    }

    double entropy = 0.0;
    const double len_d = (double)len;
    for (int i = 0; i < 256; ++i) {
        if (frequencies[i] > 0) {
            double probability = (double)frequencies[i] / len_d;
            entropy -= probability * log2(probability);
        }
    }
    return entropy;
}

void* __cdecl memmem(const void* __restrict haystack, size_t haystack_len, const void* __restrict needle, size_t needle_len) {
    if (needle_len == 0) return (void*)haystack;
    if (haystack_len < needle_len) return NULL;

    const char* h = (const char*)haystack;
    const char* n = (const char*)needle;
    const char* h_end = h + haystack_len - needle_len;

    while (h <= h_end) {
        if (*h == *n) {
            if (memcmp(h, n, needle_len) == 0) {
                return (void*)h;
            }
        }
        h++;
    }
    return NULL;
}

errno_t memset_s(void* dest, size_t destsz, int ch, size_t count) {
    if (dest == NULL) {
        return EINVAL;
    }
    if (destsz > (size_t)-1 / 2) {
        return EINVAL;
    }
    if (count > destsz) {
        memset(dest, 0, destsz);
        return EINVAL;
    }

    memset(dest, ch, count);

    return 0;
}