#pragma once

#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64) || defined(_M_AMD64))
#include <xmmintrin.h>
#endif

namespace wg::dsp {

/** Flush denormals to zero for the scope lifetime (SSE FTZ/DAZ on x86; no-op elsewhere). SPEC §8.4 */
class ScopedFlushDenormals
{
public:
    ScopedFlushDenormals() noexcept
    {
#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64) || defined(_M_AMD64))
        const unsigned int csr = _mm_getcsr();
        savedCsr_ = csr;
        _mm_setcsr(csr | 0x8040u); // FTZ | DAZ
#endif
    }

    ~ScopedFlushDenormals() noexcept
    {
#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64) || defined(_M_AMD64))
        _mm_setcsr(savedCsr_);
#endif
    }

    ScopedFlushDenormals(const ScopedFlushDenormals&) = delete;
    ScopedFlushDenormals& operator=(const ScopedFlushDenormals&) = delete;

private:
#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64) || defined(_M_AMD64))
    unsigned int savedCsr_ = 0u;
#endif
};

} // namespace wg::dsp
