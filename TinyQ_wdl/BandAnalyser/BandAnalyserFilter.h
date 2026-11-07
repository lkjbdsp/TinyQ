#pragma once

#include "SimdBandFilter.h"

class SimdBandAnalyserFilter
{
public:

    struct Word
    {
        Word(const float* arData = nullptr);
        double data[4] = { 0, 0, 0, 0 };
    };

    SimdBandAnalyserFilter();
    void setup(int bandIdx, double fCenter, double bw, double sampleRate, int order);
    void clear();
    void clearHold();
    Word processBlock(const float* input, int numSamples);
    Word getHold() const;

private:

    enum Data
    {
        kRelMul,
        kValue,
        kHold,

        kNumData
    };

    SimdBandFilter::BandFilter filter;
    SimdBandFilter::AlignedFloats intData = SimdBandFilter::AlignedFloats(kNumData);
    SimdBandFilter::AlignedFloats procData = SimdBandFilter::AlignedFloats(blockSize);
    static const int blockSize = 128;
};