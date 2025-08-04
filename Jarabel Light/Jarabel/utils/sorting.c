#include "../core/app.h"

int CompareBySizeAsc(const void* __restrict a, const void* __restrict b) {
    const FileInfo* pInfoA = *(const FileInfo**)a;
    const FileInfo* pInfoB = *(const FileInfo**)b;

    return (pInfoA->liFileSize.QuadPart > pInfoB->liFileSize.QuadPart) - (pInfoA->liFileSize.QuadPart < pInfoB->liFileSize.QuadPart);
}

int CompareBySizeDesc(const void* __restrict a, const void* __restrict b) {
    const FileInfo* pInfoA = *(const FileInfo**)a;
    const FileInfo* pInfoB = *(const FileInfo**)b;

    return (pInfoB->liFileSize.QuadPart > pInfoA->liFileSize.QuadPart) - (pInfoB->liFileSize.QuadPart < pInfoA->liFileSize.QuadPart);
}

int CompareByDateAsc(const void* __restrict a, const void* __restrict b) {
    const FileInfo* pInfoA = *(const FileInfo**)a;
    const FileInfo* pInfoB = *(const FileInfo**)b;

    return CompareFileTime(&pInfoA->ftLastAccessTime, &pInfoB->ftLastAccessTime);
}
int CompareByDateDesc(const void* __restrict a, const void* __restrict b) {
    const FileInfo* pInfoA = *(const FileInfo**)a;
    const FileInfo* pInfoB = *(const FileInfo**)b;

    return CompareFileTime(&pInfoB->ftLastAccessTime, &pInfoA->ftLastAccessTime);
}