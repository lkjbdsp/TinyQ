#pragma once

#include <vector>
#include <memory>
#include "lkjbFilter.h"

namespace TQFilterStage
{

    enum FilterType
    {
        filterTypePeak,
        filterTypeLoShelf,
        filterTypeHiShelf,

        filterNumTypes
    };

    enum PassType
    {
        passLoPass,
        passHiPass,

        passNumTypes
    };

    class FilterInstance
    {
    public:

        void clear()
        {
            state[0].clear();
            state[1].clear();
        }

        void setFilter(float freq, float Q, float gain, FilterType filterType, float sampleRate)
        {
            curFreq = freq;
            curGain = gain;
            curQ = Q;
            curFilterType = filterType;
            lkjb::FilterType type = filterType == filterTypePeak ? lkjb::filterTypePeak
                : filterType == filterTypeLoShelf ? lkjb::filterTypeLoShelf
                : lkjb::filterTypeHiShelf;

            lkjb::MatchedZTransform::setMZTiFilter(band, freq, Q, gain, type, sampleRate);
        }

        FilterType getFilterType() const
        {
            return curFilterType;
        };

        float getGain() const
        {
            return curGain;
        }

        float getFreq() const
        {
            return curFreq;
        }

        float getQ() const
        {
            return curQ;
        }

        inline void processBlock(float* processData, int numSamples)
        {
            processBlock(processData, nullptr, numSamples);
        }

        inline void processBlock(float* dataL, float* dataR, int numSamples)
        {
            for (int i = 0; i < numSamples; ++i)
                dataL[i] = band.process(state[0], dataL[i]);

            if (dataR == nullptr)
                return;

            for (int i = 0; i < numSamples; ++i)
                dataR[i] = band.process(state[1], dataR[i]);
        }

        inline void getResponse(float* mags, const float* normFreqs, int numPoints)
        {
            for (int i = 0; i < numPoints; ++i)
                mags[i] = static_cast<float> (band.response(normFreqs[i]));
        }

    private:
        FilterType curFilterType = filterTypePeak;
        float curFreq = 0.f;
        float curGain = 0.f;
        float curQ = 0.f;
        lkjb::MZTIFilterParams<float> band;
        lkjb::MZTIFilterParams<float>::State state[2];
    };

    class PassInstance
    {
    public:

        void clear()
        {
            stateL.clear();
            stateR.clear();
        }

        void setFilter(int order, float freq, PassType filterType, float sr)
        {
            if (curOrder == order && curFreq == freq && curPassType == filterType && curSR == sr)
                return;

            curOrder = order;
            curPassType = filterType;
            curFreq = freq;
            curSR = sr;
            auto type = curPassType == PassType::passLoPass ? lkjb::bwLowPass : lkjb::bwHighPass;
            params.setFilter(curOrder, freq, type, curSR);
        }

        inline void processBlock(float* processData, int numSamples)
        {
            processBlock(processData, nullptr, numSamples);
        }

        inline void processBlock(float* dataL, float* dataR, int numSamples)
        {
            for (int i = 0; i < numSamples; ++i)
                dataL[i] = params.process(stateL, dataL[i]);

            if (dataR == nullptr)
                return;

            for (int i = 0; i < numSamples; ++i)
                dataR[i] = params.process(stateR, dataR[i]);
        }

        
        inline void getResponse(float* mags, const float* normFreqs, int numPoints)
        {
            for (int i = 0; i < numPoints; ++i)
                mags[i] = static_cast<float> (params.response(normFreqs[i]));
        }

    private:

        int curOrder = 0;
        float curFreq = 0.f;
        PassType curPassType = passLoPass;
        float curSR = 0.f;

        lkjb::ButterWorthParams<float> params = lkjb::ButterWorthParams<float>(8);
        lkjb::ButterWorthParams<float>::State stateL = lkjb::ButterWorthParams<float>::State(8);
        lkjb::ButterWorthParams<float>::State stateR = lkjb::ButterWorthParams<float>::State(8);
    };
}
