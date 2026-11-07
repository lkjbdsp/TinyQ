#include "SimdBandFilter.h"
#include <emmintrin.h>
#include "../JuceIncludes.h"

namespace SimdBandFilter
{

bool isAligned(const void* x)
{
    return (((size_t) (x)) & 0xF) == 0;
}

ScopedCSR::ScopedCSR()
    : csr(_mm_getcsr())
{
    _mm_setcsr(csr | 0x8040);
}

ScopedCSR::~ScopedCSR()
{
    _mm_setcsr(csr);
}

void ScopedCSR::csrAssert()
{
    jassert((_mm_getcsr() & 0x8040) == 0x8040);
}

SimdBiquad::SimdBiquad()
{
    for (int i = 0; i < 4; ++i)
        setFilter(i, BandPassFilterCreator::BiquadParams());
}

void SimdBiquad::clear()
{
    intData.clear(kZ1);
    intData.clear(kZ2);
}

void SimdBiquad::setFilter(int bandIdx, const BandPassFilterCreator::BiquadParams& params)
{
    if (bandIdx < 0 || bandIdx >= 4)
        return;

    const double f = params.a2;
    const double e = sqrt(1 + params.a1 + params.a2);

    const double t0 = params.b0 / e;
    const double t1 = -params.b2 / e;
    const double t2 = (params.b2 + params.b1 + params.b0) / (e*e);

    intData.getFloat(kF, bandIdx) = static_cast<float> (f);
    intData.getFloat(kE, bandIdx) = static_cast<float> (e);
    intData.getFloat(kT0, bandIdx) = static_cast<float> (t0);
    intData.getFloat(kT1, bandIdx) = static_cast<float> (t1);
    intData.getFloat(kT2, bandIdx) = static_cast<float> (t2);
}

void SimdBiquad::processBlock(float* out, const float* in, int numSamples)
{
    ScopedCSR::csrAssert();

    if (! isAligned(out) || ! isAligned(in))
    {
        jassertfalse;
        return;
    }

    __m128 z1 = _mm_load_ps(intData.getPtr(kZ1));
    __m128 z2 = _mm_load_ps(intData.getPtr(kZ2));
    const __m128 e = _mm_load_ps(intData.getPtr(kE));
    const __m128 f = _mm_load_ps(intData.getPtr(kF));
    const __m128 t0 = _mm_load_ps(intData.getPtr(kT0));
    const __m128 t1 = _mm_load_ps(intData.getPtr(kT1));
    const __m128 t2 = _mm_load_ps(intData.getPtr(kT2));

    for (int i = 0; i < numSamples; ++i)
    {
        __m128 x = _mm_load_ps(in + 4 * i);

        __m128 tmp = _mm_sub_ps(x, z2);
        tmp = _mm_mul_ps(tmp, e);
        tmp = _mm_add_ps(_mm_mul_ps(z1, f), tmp);

        __m128 y = _mm_mul_ps(tmp, t0);
        y = _mm_add_ps(y, _mm_mul_ps(z1, t1));
        y = _mm_add_ps(y, _mm_mul_ps(z2, t2));

        z2 = _mm_add_ps(_mm_mul_ps(tmp, e), z2);
        z1 = tmp;

        _mm_store_ps(out + 4 * i, y);
    }

    _mm_store_ps(intData.getPtr(kZ1), z1);
    _mm_store_ps(intData.getPtr(kZ2), z2);
}

void BandFilter::clear()
{
    for (auto f : filters)
        f.clear();
}

void BandFilter::resetFilters()
{
    filters.clear();
}

void BandFilter::setFilter(int bandIdx, const BandPassFilterCreator::BiquadCascade& cascade)
{
    const size_t numFilters = cascade.size();
    jassert(filters.size() == 0 || filters.size() == numFilters);

    if (numFilters != filters.size())
        filters.resize(numFilters);

    for (size_t i = 0; i < numFilters; ++i)
        filters[i].setFilter(bandIdx, cascade[i]);
}

void BandFilter::processFilter(float* out, const float* in, int numSamples)
{
    const size_t numFilters = filters.size();
    if (numFilters < 1)
        return;

    filters[0].processBlock(out, in, numSamples);

    for (size_t i = 1; i < numFilters; ++i)
        filters[i].processBlock(out, out, numSamples);
}


}