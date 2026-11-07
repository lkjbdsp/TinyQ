#pragma once

#include <vector>
#include "BandPassFilterCreator.h"

namespace SimdBandFilter
{

class ScopedCSR
{
public:
    ScopedCSR();
    ~ScopedCSR();
    static void csrAssert();

private:
    int csr;
};

struct AlignedFloats
{
    AlignedFloats(int size_ = 0)
        : size(0)
    {
        setSize(size_);
    }

    void setSize(int newSize)
    {
        if (newSize != size)
        {
            size = newSize;
            data.resize(size * 4 + 4);
            const size_t addr = (size_t) data.data();
            const size_t offset = addr & 0xF;
            ptr = (float*) (addr + 0x10 - offset);
        }
    }

    float* getPtr(int wordIdx) const { return &(ptr[4 * wordIdx]); }

    float* getData() { return ptr; }

    float& getFloat(int wordIdx, int subIndex)
    {
        jassert(wordIdx >= 0 && wordIdx < size);
        jassert(subIndex >= 0 && subIndex < 4);
        return ptr[4 * wordIdx + subIndex];
    }

    void set(int wordIdx, float value)
    {
        set(wordIdx, value, value, value, value);
    }

    void set(int wordIdx, float value1, float value2, float value3, float value4)
    {
        ptr[4 * wordIdx] = value1;
        ptr[4 * wordIdx + 1] = value2;
        ptr[4 * wordIdx + 2] = value3;
        ptr[4 * wordIdx + 3] = value4;
    }

    const int getSize() { return size; }

    void clear()
    {
        for (int i = 0; i < size * 4; ++i)
            ptr[i] = 0;
    }

    void clear(int wordIdx)
    {
        for (int i = 0; i < 4; ++i)
            ptr[4 * wordIdx + i] = 0;
    }

    template <class DataType>
    void fromSingleChannel(const DataType* channelData, int numSamples)
    {
        jassert(numSamples <= size);

        for (int i = 0; i < numSamples; ++i)
            for (int n = 0; n < 4; ++n)
                ptr[4 * i + n] = static_cast<float> (channelData[i]);
    }

    template <class DataType>
    void fromSingleChannel(int subIdx, const DataType* channelData, int numSamples)
    {
        jassert(subIdx >= 0 && subIdx < 4 && numSamples <= size);

        for (int i = 0; i < numSamples; ++i)
            ptr[4 * i + subIdx] = static_cast<float> (channelData[i]);
    }

    template <class DataType>
    void toSingleChannel(int subIdx, DataType* channelData, int numSamples)
    {
        jassert(subIdx >= 0 && subIdx < 4 && numSamples <= size);

        for (int i = 0; i < numSamples; ++i)
            channelData[i] = static_cast<DataType> (ptr[4 * i + subIdx]);
    }

private:
    float* ptr;
    std::vector<float> data;
    int size;
};

class SimdBiquad
{
public:

    SimdBiquad();
    void clear();
    void setFilter(int bandIdx, const BandPassFilterCreator::BiquadParams& params);
    void processBlock(float* out, const float* in, int numSamples);

private:

    enum Data
    {
        kE,
        kF,
        kT0,
        kT1,
        kT2,
        kZ1,
        kZ2,

        kNumData
    };

    AlignedFloats intData = AlignedFloats(kNumData);
};


class BandFilter
{
public:

    void clear();
    void resetFilters();
    void setFilter(int bandIdx, const BandPassFilterCreator::BiquadCascade& cascade);
    void processFilter(float* out, const float* in, int numSamples);

private:

    std::vector<SimdBiquad> filters;
};

}
