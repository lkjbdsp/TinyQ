#include "BandAnalyser.h"
#include "../JuceIncludes.h"

namespace
{
template <class DataType>
void clearVector(std::vector<DataType>& vec)
{
    for (auto v : vec)
        v = static_cast<DataType> (0);
}
}

BandAnalyser::BandAnalyser()
{
}

void BandAnalyser::setBlockSize(int newBlockSize)
{
    simdBuffer.setSize(newBlockSize);
}

void BandAnalyser::setup(int numFilters, double minFreq, double maxFreq, double sampleRate, int order)
{
    const int numSimdFilters = (numFilters + 3) / 4;
    numFilters = 4 * numSimdFilters;

    simdFilters.resize(numSimdFilters);
    freqVals.resize(numFilters);
    peakVals.resize(numFilters);
    holdVals.resize(numFilters);

    const double freqMul = std::pow(maxFreq / minFreq, 1. / numFilters);
    const double bwMul = freqMul - 1 / freqMul;

    for (int i = 0; i < numFilters; ++i)
    {
        const double centerFreq = minFreq * std::exp(std::log(maxFreq / minFreq) * i / (numFilters - 1.));
        freqVals[i] = centerFreq;
    }

    for (int i = 0; i < numSimdFilters; ++i)
    {
        for (int n = 0; n < 4; ++n)
        {
            int idx = 4 * i + n;
            double centerFreq = freqVals[idx];
            double bw = centerFreq * bwMul;

            if (centerFreq + bw > sampleRate / 2)
                bw = sampleRate / 2 - centerFreq;

            simdFilters[i].setup(n, centerFreq, bw, sampleRate, order);
        }
    }

    clear();
}

size_t BandAnalyser::getNumBands() const
{
    return 4*simdFilters.size();
}

void BandAnalyser::setEnabled(bool en)
{
    if (! enabled && en)
        clear();

    enabled = en;
}

bool BandAnalyser::getEnabled() const
{
    return enabled;
}

void BandAnalyser::clear()
{
    clearVector(peakVals);
    clearHold();

    for (auto& f : simdFilters)
        f.clear();
}

void BandAnalyser::clearHold()
{
    clearVector(holdVals);

    for (auto& f : simdFilters)
        f.clearHold();
}

void BandAnalyser::processBlock(const double* data, int numSamples)
{
    if (! enabled)
        return;

    simdBuffer.fromSingleChannel(data, numSamples);

    for (size_t i = 0; i < simdFilters.size(); ++i)
    {
        SimdBandAnalyserFilter& filter(simdFilters[i]);
        const SimdBandAnalyserFilter::Word peak(filter.processBlock(simdBuffer.getData(), numSamples));
        const SimdBandAnalyserFilter::Word hold(filter.getHold());

        for (int n = 0; n < 4; ++n)
        {
            peakVals[4 * i + n] = peak.data[n];
            holdVals[4 * i + n] = hold.data[n];
        }
    }
}

