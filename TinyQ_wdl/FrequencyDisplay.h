#pragma once

#include "IControl.h"
#include "JuceIncludes.h"

class TinyQ;
class BandAnalyserView;

class FrequencyDisplay : public IControl
{
public:
    FrequencyDisplay(TinyQ* pPlug, IRECT pR);
    void setBandAnalyserView(BandAnalyserView* view);

    bool Draw(IGraphics* pGraphics);
    bool IsDirty();
    void OnMouseOver(int x, int y, IMouseMod* pMod) override;
    void OnMouseOut() override;
    void OnMouseDown(int x, int y, IMouseMod* pMod) override;
    void OnMouseUp(int x, int y, IMouseMod* pMod) override;
    void OnMouseDrag(int x, int y, int dX, int dY, IMouseMod* pMod) override;
    void OnMouseWheel(int x, int y, IMouseMod* pMod, int d) override;
    float freqToX(float freq) const;
    float xToFreq(float x)  const;
    juce::Range<float> getXRange() const;
    juce::Range<float> getYRange() const;

private:

    struct WatchParameters
    {
        WatchParameters() { clear(); }
        void clear();
        bool isDifferentTo(WatchParameters& other) const;
        void updateFrom(WatchParameters& other);
        double enabled[6];
        double freq[6];
        double q[4];
        double gain[4];
        double order[2];
        double shelfSwitch[2];
    };

    struct Handle
    {
        double freq = 0;
        double gain = -3.;
        bool enabled = false;
        IColor color;
        bool isBand = true;
    };

    struct Position
    {
        Position(int x0 = 0, int y0 = 0) : x(x0), y(y0) {}
        int x;
        int y;
    };
    struct FineDragPosition
    {
        float x;
        float y;
        bool fineDrag = false;
    };

    void drawCurve(IGraphics* pGraphics, const float* mags, IColor col);
    void drawGrid(IGraphics* pGraphics);
    void drawHandles(IGraphics* pGraphics);
    int getHandleUnderMouse(Position pos) const;
    void updateProcessorParameters();
    int getWidth() const;
    int getHeight() const;
    float getDistX() const;
    float getDistY() const;
    int getNumFreqPoints() const;
    float magToY(float mag) const;
    float yToMag(float y) const;
    void updateNormalizedFreqs();
    void updateFreqRes();
    void createLogFreqs(float* freqs, float startFreq, float endFreq, int numPoints) const;
    void resetMags(float* mags, int numFreqs) const;

    TinyQ* tinyQ;
    BandAnalyserView* bandAnalyserView = nullptr;
    double sampleRate = 44100;
    int distX = 14;
    int distY = 14;
    juce::Range<float> freqRange = juce::Range<float> (20.f, 20e3f);
    int changedFlag = 0;
    bool isMouseOver = false;
    int curHandleIndex = -1;
    int dragHandleIndex = -1;
    FineDragPosition fineDrag;

    Handle handles[6];
    juce::HeapBlock<float> freqs;
    juce::HeapBlock<float> normFreqs;
    juce::HeapBlock<float> magnitude;
    juce::HeapBlock<float> bandMag[6];

    WatchParameters procParams;
    WatchParameters guiParams;
};