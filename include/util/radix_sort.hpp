#ifndef CLTJ_UTIL_RADIX_SORT_HPP
#define CLTJ_UTIL_RADIX_SORT_HPP

#include <cstddef>
#include <cstdint>
#include <vector>

namespace util {

/// Stable LSD radix sort of @p a by the high 32 bits of each element, one byte per pass.
/// Bytes that are equal in every element are skipped. @p tmp is scratch; its contents are lost.
inline void radix_sort_high32(std::vector<uint64_t>& a, std::vector<uint64_t>& tmp) {
    const size_t n = a.size();
    if (n < 2)
        return;
    size_t count[4][256] = {};
    for (uint64_t x : a)
        for (int d = 0; d < 4; ++d)
            ++count[d][(x >> (32 + 8 * d)) & 0xff];
    tmp.resize(n);
    for (int d = 0; d < 4; ++d) {
        const int shift = 32 + 8 * d;
        size_t* c = count[d];
        if (c[(a[0] >> shift) & 0xff] == n)
            continue;
        size_t sum = 0;
        for (int b = 0; b < 256; ++b) {
            const size_t k = c[b];
            c[b] = sum;
            sum += k;
        }
        for (uint64_t x : a)
            tmp[c[(x >> shift) & 0xff]++] = x;
        a.swap(tmp);
    }
}

/// Stable LSD radix sort of the records of @p a, each @p width consecutive values, by the first value of each
/// record, one byte per pass. Bytes that are equal in every record are skipped. @p tmp is scratch.
inline void radix_sort_records32(std::vector<uint32_t>& a, size_t width, std::vector<uint32_t>& tmp) {
    const size_t n = a.size() / width;
    if (n < 2)
        return;
    size_t count[4][256] = {};
    for (size_t i = 0; i < n; ++i)
        for (int d = 0; d < 4; ++d)
            ++count[d][(a[i * width] >> (8 * d)) & 0xff];
    tmp.resize(a.size());
    for (int d = 0; d < 4; ++d) {
        const int shift = 8 * d;
        size_t* c = count[d];
        if (c[(a[0] >> shift) & 0xff] == n)
            continue;
        size_t sum = 0;
        for (int b = 0; b < 256; ++b) {
            const size_t k = c[b];
            c[b] = sum;
            sum += k;
        }
        for (size_t i = 0; i < n; ++i) {
            const uint32_t* src = &a[i * width];
            uint32_t* dst = &tmp[c[(src[0] >> shift) & 0xff]++ * width];
            for (size_t k = 0; k < width; ++k)
                dst[k] = src[k];
        }
        a.swap(tmp);
    }
}

}  // namespace util

#endif
