#include "FrequencyDisplay.h"
#include "TinyQ.h"

namespace FloatVectorOperations
{
template <class DataType>
void clear(DataType* data, int numSamples)
{
    for (int i=0; i<numSamples; ++i)
        data[i] = static_cast<DataType> (0);
}

void add(float* y, const float* x, int numSamples)
{
    for (int i=0; i<numSamples; ++i)
        y[i] += x[i];
}

void convertMagsToDb(float* data, int numPoints)
{
    for (int i=0; i<numPoints; ++i)
        data[i] = 20.f * std::log10(juce::jmax(1e-10f, data[i]));
}
};

void FrequencyDisplay::WatchParameters::clear()
{
    FloatVectorOperations::clear(freq, 6);
    FloatVectorOperations::clear(q, 4);
    FloatVectorOperations::clear(gain, 4);
    FloatVectorOperations::clear(order, 2);
    FloatVectorOperations::clear(shelfSwitch, 2);
}

bool FrequencyDisplay::WatchParameters::isDifferentTo(WatchParameters& other) const
{
    for (int i=0; i<6; ++i)
        if (std::abs(enabled[i] - other.enabled[i]) > 0.1)
            return true;

    for (int i=0; i<6; ++i)
        if (std::abs(freq[i] - other.freq[i]) > 0.1)
            return true;

    for (int i=0; i<4; ++i)
        if (std::abs(q[i] - other.q[i]) > 0.001)
            return true;

    for (int i=0; i<4; ++i)
        if (std::abs(gain[i] - other.gain[i]) > 0.05)
            return true;

    for (int i=0; i<2; ++i)
        if (std::abs(order[i] - other.order[i]) > 0.001)
            return true;

    for (int i=0; i<2; ++i)
        if (std::abs(shelfSwitch[i] - other.shelfSwitch[i]) > 0.001)
            return true;

    return false;
}
void FrequencyDisplay::WatchParameters::updateFrom(WatchParameters& other)
{
    for (int i=0; i<6; ++i)
        enabled[i] = other.enabled[i];

    for (int i=0; i<6; ++i)
        freq[i] = other.freq[i];

    for (int i=0; i<4; ++i)
        q[i] = other.q[i];

    for (int i=0; i<4; ++i)
        gain[i] = other.gain[i];

    for (int i=0; i<2; ++i)
        order[i] = other.order[i];

    for (int i=0; i<2; ++i)
        shelfSwitch[i] = other.shelfSwitch[i];
}

FrequencyDisplay::FrequencyDisplay(TinyQ* pPlug, IRECT pR)
    : IControl(pPlug, pR), tinyQ(pPlug)
{
    DisablePrompt(true);
    const int w = getWidth();
    freqs.realloc(w);
    normFreqs.realloc(w);
    magnitude.realloc(w);

    for (int i=0; i<6; ++i)
        bandMag[i].realloc(w);

    {
        const unsigned int colours[5] = {0xFFCC5555, 0xFFCC9911, 0xFF55CCAA, 0xFF1177CC, 0xFFBBBBBB };
        for (int i=0; i<4; ++i)
            handles[i].color = IColor::fromArgb(colours[i]);

        handles[4].color = IColor::fromArgb(colours[4]);
        handles[5].color = IColor::fromArgb(colours[4]);
        handles[4].isBand = false;
        handles[5].isBand = false;
    }

    updateNormalizedFreqs();
    updateFreqRes();
}

void FrequencyDisplay::setBandAnalyserView(BandAnalyserView* view)
{
    bandAnalyserView = view;
}

bool FrequencyDisplay::Draw(IGraphics* pGraphics)
{
    IColor bgCol(IColor::fromArgb(0xFF333333));
    pGraphics->FillIRect(&bgCol, &mRECT);

    if (bandAnalyserView != nullptr)
        bandAnalyserView->Draw(pGraphics);

    drawGrid(pGraphics);

    if (tinyQ->getShowBackgroundText())
    {
        const char* str = "TinyQ";
        IColor col(IColor::fromArgb(0x55AAAAAA));

        IText text(&col);
        const int y0 = mRECT.H() / 2;
        IRECT rect(mRECT);
        rect.T = y0/2;
        rect.B = y0*3/2;
        text.mSize = y0;
        pGraphics->DrawIText(&text, str, &rect);
    }

    drawCurve(pGraphics, magnitude.get(), IColor::fromArgb(0xFFFFFFFF));

    if (curHandleIndex >= 0)
        drawCurve(pGraphics, bandMag[curHandleIndex].get(), handles[curHandleIndex].color);

    {
        IColor gridCol(IColor::fromArgb(0xFFCCCCCC));
        const float xl = getXRange().getStart();
        const float xr = getXRange().getEnd();
        const float yt = getYRange().getStart();
        const float yb = getYRange().getEnd();
        IRECT rect(static_cast<int> (xl), static_cast<int> (yt), static_cast<int> (xr), static_cast<int> (yb));
        pGraphics->DrawRect(&gridCol, &rect);
    }

    drawHandles(pGraphics);
    return true;
}

bool FrequencyDisplay::IsDirty()
{
    updateProcessorParameters();
    const bool sampleRateChanged = tinyQ->GetSampleRate() != sampleRate;
    const bool bandAnalyserDirty = bandAnalyserView != nullptr && bandAnalyserView->IsDirty();

    if (sampleRateChanged)
    {
        sampleRate = tinyQ->GetSampleRate();
        updateNormalizedFreqs();
    }

    if (guiParams.isDifferentTo(procParams) || sampleRateChanged)
    {
        guiParams.updateFrom(procParams);
        updateFreqRes();

        for (int i=0; i<4; ++i)
        {
            handles[i].freq = guiParams.freq[i];
            handles[i].gain = guiParams.gain[i];
            handles[i].enabled = guiParams.enabled[i];
        }

        for (int i=4; i<6; ++i)
        {
            handles[i].freq = guiParams.freq[i];
            handles[i].enabled = guiParams.enabled[i];
        }

        SetDirty(false);
        return true;
    }

    return bandAnalyserDirty || IControl::IsDirty();
}

void FrequencyDisplay::OnMouseOver(int x, int y, IMouseMod* pMod)
{
    const bool wasMouseOver = isMouseOver;
    isMouseOver = true;
    const Position newPos(x, y);
    const int lastHandleIdx = curHandleIndex;
    curHandleIndex = getHandleUnderMouse(newPos);

    if (wasMouseOver || curHandleIndex != lastHandleIdx)
        SetDirty(false);
}

void FrequencyDisplay::OnMouseOut()
{
    curHandleIndex = -1;
    isMouseOver = false;
    SetDirty(false);
}

void FrequencyDisplay::OnMouseDown(int x, int y, IMouseMod* pMod)
{
    if (pMod->L)
    {
        const Position newPos(x, y);
        fineDrag.x = static_cast<float> (x);
        fineDrag.y = static_cast<float> (y);
        fineDrag.fineDrag = pMod->S;

        dragHandleIndex = getHandleUnderMouse(newPos);
        const EParams enabledParams[6] = { kLowEnable, kLoMidEnable, kHiMidEnable, kHighEnable, kHPEnable, kLPEnable };
        const int paramIdx = enabledParams[dragHandleIndex];

        if (IParam* p = dragHandleIndex >= 0 ? tinyQ->GetParam(paramIdx) : nullptr)
        {
            if (! p->Bool())
            {
                tinyQ->SetParameterFromGUI(paramIdx, 1.);

                if (IControl* sw = tinyQ->getControlForParameter(paramIdx))
                    sw->SetValueFromPlug(1.);

                SetDirty(false);
            }
        }

        if (fineDrag.fineDrag && dragHandleIndex >= 0)
            tinyQ->GetGUI()->HideMouseCursor();
        
        if (dragHandleIndex < 0)
            tinyQ->getBandAnalyser().clearHold();

    }
    else if (pMod->R)
    {
        IPlugBase::ShowMenu(tinyQ->getSettingsMenu(), tinyQ->GetGUI(), IRECT(x, y, x, y));
    }
}

void FrequencyDisplay::OnMouseUp(int x, int y, IMouseMod* pMod)
{
    dragHandleIndex = -1;

    if (fineDrag.fineDrag)
        tinyQ->GetGUI()->ShowMouseCursor();
}

void FrequencyDisplay::OnMouseDrag(int x, int y, int dX, int dY, IMouseMod* pMod)
{
    const float magRange = tinyQ->getFreqViewMagRange();
    if (dragHandleIndex >= 0)
    {
        fineDrag.x += 0.1f * dX;
        fineDrag.y += 0.1f * dY;

        const float fx = fineDrag.fineDrag ? fineDrag.x : static_cast<float> (x);
        const float fy = fineDrag.fineDrag ? fineDrag.y : static_cast<float> (y);

        const float freq = freqRange.clipValue(xToFreq(fx));
        const float gain = juce::jlimit(-magRange, magRange, yToMag(fy));

        const EParams freqParams[6] = { kLowFreq, kLoMidFreq, kHiMidFreq, kHighFreq, kHPFreq, kLPFreq };
        const EParams gainParams[4] = { kLowGain, kLoMidGain, kHiMidGain, kHighGain };

        tinyQ->SetParameterFromGUI(freqParams[dragHandleIndex], tinyQ->GetParam(freqParams[dragHandleIndex])->GetNormalized(freq));

        if (dragHandleIndex < 4)
            tinyQ->SetParameterFromGUI(gainParams[dragHandleIndex], tinyQ->GetParam(gainParams[dragHandleIndex])->GetNormalized(gain));

        SetDirty(false);
    }
}

void FrequencyDisplay::OnMouseWheel(int x, int y, IMouseMod* pMod, int d)
{
    const Position newPos(x, y);
    const int handleIdx = getHandleUnderMouse(newPos);
    const EParams qParams[6] = { kLowQ, kLoMidQ, kHiMidQ, kHighQ, kHPOrder, kLPOrder };

    if (IParam* p = tinyQ->GetParam(qParams[handleIdx]))
    {
        const double curValue = p->Value();
        if (handleIdx < 4)
        {
            const double fine = pMod->S ? 0.1 : 1;
            double newNorm = juce::jlimit(0., 1., p->GetNormalized(curValue) + 0.05 * fine * d);
            tinyQ->SetParameterFromGUI(qParams[handleIdx], newNorm);
        }
        else
        {
            const double newValue = juce::jlimit(p->GetMin(), p->GetMax(), curValue + d);
            tinyQ->SetParameterFromGUI(qParams[handleIdx], p->GetNormalized(newValue));
        }
    }
}

float FrequencyDisplay::freqToX(float freq) const
{    
    const float d = getDistX();
    return mRECT.L + d + (getWidth() -2*d) * std::log(freq / freqRange.getStart()) / std::log(freqRange.getEnd() / freqRange.getStart()) + 0.5f;
}

float FrequencyDisplay::xToFreq(float x)  const
{
    const float d = getDistX();
    return freqRange.getStart() * std::exp((x - mRECT.L - d - 0.5f) / (getWidth()-2*d) * std::log(freqRange.getEnd() / freqRange.getStart()));;
}

juce::Range<float> FrequencyDisplay::getXRange() const
{
    return juce::Range<float> (std::floor(freqToX(freqRange.getStart())), std::ceil(freqToX(freqRange.getEnd())));
}

juce::Range<float> FrequencyDisplay::getYRange() const
{
    const float magRange = tinyQ->getFreqViewMagRange();
    return juce::Range<float> (std::floor(magToY(magRange)), std::ceil(magToY(-magRange)));
}

void FrequencyDisplay::drawCurve(IGraphics* pGraphics, const float* mags, IColor col)
{
    const int numPoints = getNumFreqPoints();
    const juce::Range<float> yRange(getYRange());
    float lastX = getXRange().getStart();
    float lastY = yRange.clipValue(magToY(mags[0]));

    for (int i=1; i<numPoints; ++i)
    {
        const float x = lastX + 1.f;
        const float y = yRange.clipValue(magToY(mags[i]));
        pGraphics->DrawLine(&col, lastX, lastY, x, y);
        lastX = x;
        lastY = y;
    }
}

void FrequencyDisplay::drawGrid(IGraphics* pGraphics)
{
    IColor gridColBase(IColor::fromArgb(0xFF666666));
    IColor gridColLight(IColor::fromArgb(0xFF999999));
    const int numMags = 9;
    float gridMags[numMags] = { -1.f, -0.75f, -0.5f, -0.25f, 0.f, 0.25f, 0.5f, 0.75f, 1.f };
    const float xl = getXRange().getStart();
    const float xr = getXRange().getEnd();
    IText text;
    const float sizeFactor = tinyQ->getSizeFactor();
    const float dist = 3.f * sizeFactor;
    const float textHeight = sizeFactor * 7;
    const float textWidth = 100.f * sizeFactor;
    text.mTextEntryBGColor = COLOR_TRANSPARENT;
    text.mTextEntryFGColor = COLOR_TRANSPARENT;
    text.mColor = IColor::fromArgb(0xFFCCCCCC);
    text.mSize = static_cast<int> (textHeight);
    text.mQuality = IText::kQualityAntiAliased;
    const float magRange = tinyQ->getFreqViewMagRange();

    for (int i=0; i<numMags; ++i)
    {
        const float mag = gridMags[i] * magRange;
        const float y = magToY(mag) + 0.5f;
        IColor gridCol(gridColBase);

        if ((i & 1) == 0)
        {
            IRECT rl(0, static_cast<int> (y - 0.5f*textHeight), static_cast<int> (xl - dist),  static_cast<int> (y + 0.5f*textHeight));
            IRECT rr(static_cast<int> (xr + dist), static_cast<int> (y - 0.5f*textHeight), static_cast<int> (textWidth), static_cast<int> (y+0.5f*textHeight));
            const juce::String t(static_cast<int> (mag));
            const char* str = t.toRawUTF8();
            text.mAlign = IText::kAlignFar;
            pGraphics->DrawIText(&text, str, &rl);
            text.mAlign = IText::kAlignNear;
            pGraphics->DrawIText(&text, str, &rr);
            gridCol = gridColLight;
        }

        pGraphics->DrawLine(&gridCol, xl, y, xr, y, nullptr, false);
    }

    const int numFreqs = 10;
    const float gridFreqs[numFreqs] = { 20.f, 50.f, 100.f, 200.f, 500.f, 1e3f, 2e3f, 5e3f, 10e3f, 20e3f };
    const juce::Array<float> labelFreqs({ 100.f, 1e3f, 10e3f });

    const float yt = getYRange().getStart();
    const float yb = getYRange().getEnd();
    text.mAlign = IText::kAlignCenter;

    for (int i=0; i<numFreqs; ++i)
    {
        const float freq = gridFreqs[i];
        const float x = freqToX(freq);
        IColor gridCol(gridColBase);

        if (labelFreqs.contains(freq))
        {
            const juce::String t(freq < 1000 ? juce::String(static_cast<int> (freq)) : juce::String(static_cast<int> (freq*0.001f)) + "k");
            IRECT rt(static_cast<int> (x - 0.5f*textWidth), static_cast<int> (yt - dist - textHeight), static_cast<int> (x + 0.5f*textWidth), static_cast<int> (yt - dist));
            IRECT rb(static_cast<int> (x - 0.5f*textWidth), static_cast<int> (yb + dist), static_cast<int> (x + 0.5f*textWidth), static_cast<int> (yt - dist + textHeight));
            const char* str = t.toRawUTF8();
            pGraphics->DrawIText(&text, str, &rt);
            pGraphics->DrawIText(&text, str, &rb);
            gridCol = gridColLight;
        }

        pGraphics->DrawLine(&gridCol, x, yt, x, yb, nullptr, false);
    }
}

void FrequencyDisplay::drawHandles(IGraphics* pGraphics)
{
    if (! isMouseOver)
        return;

    const float r = tinyQ->getSizeFactor() * 4 + 0.5f;

    for (int i=0; i<6; ++i)
    {
        const int x = static_cast<int> (freqToX(static_cast<float> (handles[i].freq)) + 0.5f);
        const int y = static_cast<int> (magToY(static_cast<float> (handles[i].gain)) + 0.5f);

        if (handles[i].enabled)
            pGraphics->FillCircle(&handles[i].color, x, y, r);
        else
            pGraphics->DrawCircle(&handles[i].color, static_cast<float> (x), static_cast<float> (y), r);
    }
}

int FrequencyDisplay::getHandleUnderMouse(Position pos) const
{
    const float r0 = tinyQ->getSizeFactor() * 4 + 0.5f;
    int idx = -1;
    float minR = r0;

    for (int i=0; i<6; ++i)
    {
        const float x = freqToX(static_cast<float> (handles[i].freq));
        const float y = magToY(static_cast<float> (handles[i].gain));
        const float r = std::sqrt((x - pos.x)*(x-pos.x) + (y-pos.y)*(y-pos.y));
        if (r < minR)
        {
            idx = i;
            minR = r;
        }
    }

    return idx;
}

void FrequencyDisplay::updateProcessorParameters()
{
    const EParams enabledParams[6] = { kLowEnable, kLoMidEnable, kHiMidEnable, kHighEnable, kHPEnable, kLPEnable };
    const EParams freqParams[6] = { kLowFreq, kLoMidFreq, kHiMidFreq, kHighFreq, kHPFreq, kLPFreq };
    const EParams qParams[4] = { kLowQ, kLoMidQ, kHiMidQ, kHighQ };
    const EParams gainParams[4] = { kLowGain, kLoMidGain, kHiMidGain, kHighGain };
    const EParams orderParams[2] = { kHPOrder, kLPOrder };
    const EParams shelfParams[2] = { kLoShelfSwitch, kHiShelfSwitch };

    for (int i=0; i<6; ++i)
        procParams.enabled[i] = tinyQ->GetParam(enabledParams[i])->Value();

    for (int i=0; i<6; ++i)
        procParams.freq[i] = tinyQ->GetParam(freqParams[i])->Value();

    for (int i=0; i<4; ++i)
        procParams.q[i] = tinyQ->GetParam(qParams[i])->Value();

    for (int i=0; i<4; ++i)
        procParams.gain[i] = tinyQ->GetParam(gainParams[i])->Value();

    for (int i=0; i<2; ++i)
        procParams.order[i] = tinyQ->GetParam(orderParams[i])->Value();

    for (int i=0; i<2; ++i)
        procParams.shelfSwitch[i] = tinyQ->GetParam(shelfParams[i])->Value();
}

int FrequencyDisplay::getWidth() const
{
    return mRECT.W();
}

int FrequencyDisplay::getHeight() const
{
    return mRECT.H();
}

float FrequencyDisplay::getDistX() const
{
    return distX * tinyQ->getSizeFactor();
}

float FrequencyDisplay::getDistY() const
{
    return distY * tinyQ->getSizeFactor();
}

int FrequencyDisplay::getNumFreqPoints() const
{
    return static_cast<int> (getXRange().getLength() + 1e-5f);
}

float FrequencyDisplay::magToY(float mag) const
{ 
    const float d = getDistY();
    const float magRange = tinyQ->getFreqViewMagRange();
    return mRECT.T + d + (getHeight() - 2*d) * (1.f - (mag + magRange) / (2.f * magRange)) + 0.5f; 
}

float FrequencyDisplay::yToMag(float y) const
{ 
    const float d = getDistY(); 
    const float magRange = tinyQ->getFreqViewMagRange();
    return 2*magRange * (1.f - (y - mRECT.T - d - 0.5f) / (getHeight()-2*d)) - magRange;
}

void FrequencyDisplay::updateNormalizedFreqs()
{
    const int numFreqs = getNumFreqPoints();
    createLogFreqs(freqs.get(), freqRange.getStart(), freqRange.getEnd(), numFreqs);

    for (int i = 0; i < numFreqs; ++i)
        normFreqs[i] = static_cast<float> (freqs[i] / sampleRate);
}

void FrequencyDisplay::updateFreqRes()
{
    if (getWidth() < 1)
        return;

    const int numFreqs = getNumFreqPoints();

    bool enabled[6];

    enabled[4] = tinyQ->GetParam(kHPEnable)->Bool();
    enabled[5] = tinyQ->GetParam(kLPEnable)->Bool();
    const EParams enParams[4] = { kLowEnable, kLoMidEnable, kHiMidEnable, kHighEnable };

    for (int i=0; i<4; ++i)
    {
        auto& filter = tinyQ->bandDsp[i];
        resetMags(bandMag[i], numFreqs);
        enabled[i] = tinyQ->GetParam(enParams[i])->Bool();

        filter.getResponse(bandMag[i], normFreqs.get(), numFreqs);

        if (filter.getFilterType() == TQFilterStage::filterTypePeak)
        {
            const float freq = filter.getFreq();
            const int freqIdx = static_cast<int> (freqToX(freq) - getDistX() - mRECT.L + 0.5f);        
            bandMag[i][freqIdx] = std::pow(10.f, 0.05f * static_cast<float> (filter.getGain()));
        }

    }

    for (int i = 0; i < 2; ++i)
    {
        const auto idx = 4 + i;
        resetMags(bandMag[idx], numFreqs);
        tinyQ->passDsp[i].getResponse(bandMag[idx].get(), normFreqs.get(), numFreqs);
    }

    FloatVectorOperations::clear(magnitude.get(),numFreqs);

    for (int i=0; i<6; ++i)
    {
        FloatVectorOperations::convertMagsToDb(bandMag[i], numFreqs);

        if (enabled[i])
            FloatVectorOperations::add(magnitude.get(), bandMag[i].get(), numFreqs);
    }
}

void FrequencyDisplay::createLogFreqs(float* freqs, float startFreq, float endFreq, int numPoints) const
{
    const float logEndStart = std::log(endFreq / startFreq);

    for (int i = 0; i < numPoints; ++i)
        freqs[i] = startFreq * std::exp(logEndStart * i / (numPoints - 1.f));
}

void FrequencyDisplay::resetMags(float* mags, int numFreqs) const
{
    for (int i = 0; i < numFreqs; ++i)
        mags[i] = 1.f;
}