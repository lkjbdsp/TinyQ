#pragma once

#include "TinyQ.h"

namespace BandSorter
{
struct BandData
{
    static int compareElements (BandData first, BandData second)
    {
        const double diff = first.freq - second.freq;
        return diff < 0 ? -1 : diff > 0 ? 1 : 0;
    }


    bool enabled = false;
    double freq = 0;
    double q = 0;
    double gain = 0;
    int index = -1;
};

void sortBands(TinyQ* tinyq)
{
    juce::Array<BandData> bands;
    std::function<BandData(int)> getBand = [tinyq](int idx) {
        const int offset = idx * kLoMidEnable;
        BandData band;
        band.index = idx;
        band.enabled = tinyq->GetParam(offset+kLowEnable)->Bool();
        band.freq = tinyq->GetParam(offset + kLowFreq)->Value();
        band.q = tinyq->GetParam(offset + kLowQ)->Value();
        band.gain = tinyq->GetParam(offset + kLowGain)->Value();
        return band;
    };

    const bool useLoBand = ! tinyq->GetParam(kLoShelfSwitch)->Bool();
    const bool useHiBand = ! tinyq->GetParam(kHiShelfSwitch)->Bool();

    std::function<void(int, const BandData&)> setBand = [tinyq] (int idx, const BandData& band) {
        const int offset = idx * kLoMidEnable;
        const int enIdx = offset+kLowEnable;
        const int freqIdx = offset+kLowFreq;
        const int qIdx = offset+kLowQ;
        const int gainIdx = offset+kLowGain;
        const double enNorm = band.enabled ? 1. : 0.;
        const double freqNorm = tinyq->GetParam(offset+kLowFreq)->GetNormalized(band.freq);
        const double qNorm = tinyq->GetParam(offset+kLowQ)->GetNormalized(band.q);
        const double gainNorm = tinyq->GetParam(offset+kLowGain)->GetNormalized(band.gain);
        tinyq->SetParameterFromGUI(freqIdx, freqNorm);
        tinyq->SetParameterFromGUI(qIdx, qNorm);
        tinyq->SetParameterFromGUI(gainIdx, gainNorm);
        tinyq->SetParameterFromGUI(enIdx, enNorm);

        if (IControl* c = tinyq->getControlForParameter(freqIdx))
            c->SetValueFromPlug(freqNorm);
        if (IControl* c = tinyq->getControlForParameter(qIdx))
            c->SetValueFromPlug(qNorm);
        if (IControl* c = tinyq->getControlForParameter(gainIdx))
            c->SetValueFromPlug(gainNorm);
        if (IControl* c = tinyq->getControlForParameter(enIdx))
            c->SetValueFromPlug(enNorm);
    };
    if (useLoBand)
        bands.add(getBand(0));

    bands.add(getBand(1));
    bands.add(getBand(2));

    if (useHiBand)
        bands.add(getBand(3));

    const int startIdx = useLoBand ? 0 : 1;

    bands.sort(bands.getReference(0), true);

    for (int i=0; i<bands.size(); ++i)
        setBand(startIdx + i, bands.getReference(i));
}
}