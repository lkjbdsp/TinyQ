#pragma once

#include "BandAnalyserFilter.h"

class BandAnalyser
{
public:

    BandAnalyser();
    void setBlockSize(int newBlockSize);
    void setup(int numFilters, double minFreq, double maxFreq, double sampleRate, int order);
    size_t getNumBands() const;
    void setEnabled(bool en);
    bool getEnabled() const;

    void clear();
    void clearHold();
    void processBlock(const double* data, int numSamples);

    template<class DataType>
    void getFreqs(std::vector<DataType>& freqs);
    template<class DataType>
    bool getData(std::vector<DataType>& peak, std::vector<DataType>& hold);

private:

    bool enabled = true;
    std::vector<SimdBandAnalyserFilter> simdFilters;
    SimdBandFilter::AlignedFloats simdBuffer;
    std::vector<double> freqVals;
    std::vector<double> peakVals;
    std::vector<double> holdVals;
};

template<class DataType>
void BandAnalyser::getFreqs(std::vector<DataType>& freqs)
{
    if (freqs.size() != freqVals.size())
    {
        jassertfalse;
        return;
    }

    for (size_t i = 0; i < freqs.size(); ++i)
        freqs[i] = static_cast<DataType> (freqVals[i]);
}

template<class DataType>
bool BandAnalyser::getData(std::vector<DataType>& peak, std::vector<DataType>& hold)
{
    if (peak.size() != peakVals.size() || hold.size() != holdVals.size())
        return false;

    for (size_t i = 0; i < peak.size(); ++i)
        peak[i] = static_cast<DataType> (peakVals[i]);

    for (size_t i = 0; i < hold.size(); ++i)
        hold[i] = static_cast<DataType> (holdVals[i]);

    return true;
}