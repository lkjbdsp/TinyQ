#include "BandAnalyserFilter.h"
#include "../JuceIncludes.h"
#include <emmintrin.h>

SimdBandAnalyserFilter::Word::Word(const float* arData)
{
    if (arData != nullptr)
        for (int i = 0; i < 4; ++i)
            data[i] = arData[i];
}

SimdBandAnalyserFilter::SimdBandAnalyserFilter()
{
    intData.set(kRelMul, 0.1f);
    clear();
}

void SimdBandAnalyserFilter::setup(int bandIdx, double fCenter, double bw, double sampleRate, int order)
{
    const BandPassFilterCreator::BiquadCascade bandPass(BandPassFilterCreator::createBandPass(order, fCenter, bw, sampleRate));
    const double release = 300;
    const double relMul = 1 - std::exp(-1 / (0.001 * release * sampleRate));
    intData.getFloat(kRelMul, bandIdx) = static_cast<float> (relMul);
    filter.setFilter(bandIdx, bandPass);
}

void SimdBandAnalyserFilter::clear()
{
    filter.clear();
    intData.clear(kValue);
    clearHold();
}

void SimdBandAnalyserFilter::clearHold()
{
    intData.clear(kHold);
}

namespace SimdHelpers
{
__m128 abs_mask(void) {
    __m128i minus1 = _mm_set1_epi32(-1);
    return _mm_castsi128_ps(_mm_srli_epi32(minus1, 1));
}
__m128 vecabs_and(__m128 v) {
    return _mm_and_ps(abs_mask(), v);
}
}

SimdBandAnalyserFilter::Word SimdBandAnalyserFilter::processBlock(const float* input, int numSamples)
{
    float* tmp = procData.getData();
    
    const __m128 absMask = SimdHelpers::abs_mask();
    const __m128 rel = _mm_load_ps(intData.getPtr(kRelMul));
    __m128 value = _mm_load_ps(intData.getPtr(kValue));
    __m128 hold = _mm_load_ps(intData.getPtr(kHold));

#if JUCE_DEBUG
    float valTmp[4];
#endif

    while (numSamples > 0)
    {
        const int curNumSamples = numSamples < blockSize ? numSamples : blockSize;

        filter.processFilter(tmp, input, curNumSamples);

        for (int i = 0; i < curNumSamples; ++i)
        {
            jassert(std::abs(tmp[4*i]) < 1e6f);

            __m128 x = _mm_load_ps(tmp + 4 * i);
            x = _mm_and_ps(x, absMask);
            const __m128 relVal = _mm_add_ps(value, _mm_mul_ps(_mm_sub_ps(x, value), rel));
            const __m128 attMask = _mm_cmpge_ps(x, relVal);
            const __m128 relMask = _mm_cmplt_ps(x, relVal);
            value = _mm_add_ps(_mm_and_ps(x, attMask), _mm_and_ps(relVal, relMask));
            hold = _mm_max_ps(hold, value);

#if JUCE_DEBUG
            _mm_storeu_ps(valTmp, value);
            jassert(valTmp[0] >= 0 && valTmp[0] < 1e6f);
#endif
        }

        numSamples -= curNumSamples;
        input += 4 * curNumSamples;
    }

    _mm_store_ps(intData.getPtr(kValue), value);
    _mm_store_ps(intData.getPtr(kHold), hold);

    return Word(intData.getPtr(kValue));
}

SimdBandAnalyserFilter::Word SimdBandAnalyserFilter::getHold() const
{
    return Word(intData.getPtr(kHold));
}