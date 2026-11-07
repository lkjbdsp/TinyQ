#pragma once

#include "IControl.h"
#include "../JuceIncludes.h"
#include "BandAnalyser.h"

class BandAnalyserView
{
public:

    BandAnalyserView(BandAnalyser& analyser);
    void setFreqMapping(std::function<float(float)> f2x, std::function<float(float)> x2f);    
    void setMagnitudePositions(float yTop, float yBot);
    void setMagRange(const juce::Range<float>& newRange);
    juce::Range<float> getMagRange() const;

    bool IsDirty();
    void Draw(IGraphics* pGraphics);

    float getYFromMag(float mag) const;
    float getMagFromY(float y) const;
    
    void updateNumCurvePoints();

private:

    void drawCurve(IGraphics* pGraphics, std::vector<float>& data, IColor col);

    BandAnalyser& bandAnalyser;    
    std::function<float(float)> freqToX;
    std::function<float(float)> xToFreq;
    juce::Range<float> magPosRange;
    juce::Range<float> magRange = juce::Range<float> (-60.f, 6.f);

    std::vector<float> peakCurve;
    std::vector<float> holdCurve;
    std::vector<float> bandFreqs;
    bool bandDataExists = false;

    juce::Time lastDrawTime;
};


