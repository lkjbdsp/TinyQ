#include "BandAnalyserView.h"

BandAnalyserView::BandAnalyserView(BandAnalyser& analyser)
    : bandAnalyser(analyser)
{

}

void BandAnalyserView::setFreqMapping(std::function<float(float)> f2x, std::function<float(float)> x2f)
{
    freqToX = f2x;
    xToFreq = x2f;
    updateNumCurvePoints();
}

void BandAnalyserView::setMagnitudePositions(float yTop, float yBot)
{
    magPosRange = juce::Range<float> (yTop, yBot);
}

void BandAnalyserView::setMagRange(const juce::Range<float>& newRange)
{
    magRange = newRange;
}

juce::Range<float> BandAnalyserView::getMagRange() const
{
    return magRange;
}

bool BandAnalyserView::IsDirty()
{
    const double refreshFreq = 20;
    std::function<void(std::vector<float>&)> magToY = [this] (std::vector<float>& vec) {
        for (auto& e: vec)
            e = getYFromMag(20*std::log10(juce::jmax(1e-10f, e)));
    };

    if (! bandAnalyser.getEnabled())
        return false;

    if (juce::RelativeTime(juce::Time::getCurrentTime() - lastDrawTime).inSeconds() > 1/refreshFreq)
    {
        bandAnalyser.getData(peakCurve, holdCurve);
        magToY(peakCurve);
        magToY(holdCurve);
        bandDataExists = true;
        return true;
    }

    return false;
}

void BandAnalyserView::Draw(IGraphics* pGraphics)
{
    if (bandFreqs.size() == 0 || ! bandAnalyser.getEnabled())
        return;

    lastDrawTime = juce::Time::getCurrentTime();
    
    drawCurve(pGraphics, peakCurve, IColor::fromArgb(0xFFFFAAAA));
    drawCurve(pGraphics, holdCurve, IColor::fromArgb(0xFFDDFFDD));
}

float BandAnalyserView::getYFromMag(float mag) const
{
    mag = magRange.clipValue(mag);
    return magPosRange.getStart() + magPosRange.getLength() * (1.f - (mag - magRange.getStart()) / magRange.getLength());
}

float BandAnalyserView::getMagFromY(float y) const
{
    return (1.f - (y - magPosRange.getStart()) / magPosRange.getLength()) * magRange.getLength() + magRange.getStart();
}

void BandAnalyserView::updateNumCurvePoints()
{
    const size_t numBands = bandAnalyser.getNumBands();

    if (bandFreqs.size() != numBands)
    {
        bandFreqs.resize(numBands);
        peakCurve.resize(numBands);
        holdCurve.resize(numBands);
        bandDataExists = false;
    }

    bandAnalyser.getFreqs(bandFreqs);
}

void BandAnalyserView::drawCurve(IGraphics* pGraphics, std::vector<float>& data, IColor col)
{
    jassert(data.size() == bandFreqs.size() && freqToX != nullptr && data.size() > 0);

    if (freqToX == nullptr || data.size() != bandFreqs.size() || data.size() == 0 || ! bandDataExists)
        return;

    const size_t numBands = data.size();
    float lastX = freqToX(bandFreqs[0]);
    float lastY = data[0];

    for (size_t i=1; i<numBands; ++i)
    {
        float x = freqToX(bandFreqs[i]);
        float y = data[i];
        pGraphics->DrawLine(&col, lastX, lastY, x, y);
        lastX = x;
        lastY = y;
    }
}
