#ifndef __TINYQ__
#define __TINYQ__

#include "IPlug_include_in_plug_hdr.h"
#include <memory>
#include "JuceIncludes.h"
#include "BandAnalyser/BandAnalyserView.h"
#include "dsp/TQFilterStage.h"
#include "dsp/lkjbFilter.h"

enum EParams
{
    kLowEnable,
    kLowFreq,
    kLowQ,
    kLowGain,
    kLoMidEnable,
    kLoMidFreq,
    kLoMidQ,
    kLoMidGain,
    kHiMidEnable,
    kHiMidFreq,
    kHiMidQ,
    kHiMidGain,
    kHighEnable,
    kHighFreq,
    kHighQ,
    kHighGain,

    kLoShelfSwitch,
    kHiShelfSwitch,

    kHPEnable,
    kHPFreq,
    kHPOrder,

    kLPEnable,
    kLPFreq,
    kLPOrder,

    kGain,

    kNumParams
};

class IEditorControl;
class IGUIResizeButton;
class FrequencyDisplay;

class TinyQ : public IPlug
{
public:
    enum PluginSize
    {
        kTiny,
        kLarge,
        kHuge
    };

    TinyQ(IPlugInstanceInfo instanceInfo);
    ~TinyQ();
    bool SerializeState(ByteChunk* pChunk) override;
    int UnserializeState(ByteChunk* pChunk, int startPos) override;

    void SetBlockSize(int blockSize) override;
    void SetSampleRate(double sampleRate) override;
    void Reset() override;
    void OnWindowResize() override;

    void CreateControls(IGraphics* pGraphics);

    void UpdateParameters();
    void ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames) override;
    int getSupposedWidth() const;
    int getSupposedHeight() const;
    void toggleResize(bool increase);
    void OnGUIOpen() override;
    void OnGUIClose() override;
    IControl* getControlForParameter(int paramIdx);
    float getFreqViewMagRange() const;
    BandAnalyser& getBandAnalyser();

    IPopupMenu* getSizeMenu();
    IPopupMenu* getDefaultsMenu();
    IPopupMenu* getFreqViewMenu();
    IPopupMenu* getSettingsMenu();
    IPopupMenu* getAnalyserMenu();
    float getSizeFactor() const;
    bool getShowBackgroundText() const;

    PluginSize pluginSize = kTiny;
    TQFilterStage::PassInstance passDsp[2];
    TQFilterStage::FilterInstance bandDsp[4];

private:
    int getParamIndexForName(const juce::String& name);
    juce::File getSettingsFile() const;
    juce::XmlElement* getSettingsXml() const;
    void setSettingsXml(juce::XmlElement* xml);
    void updateShowGain();
    void updateAnalyserRange();
    void processAnalyser(const double* inL, const double* inR, int numSamples);

    struct Layout
    {
        enum XPosition
        {
            kXHiPass,
            kXRed,
            kXYellowEnable,
            kXYellow,
            kXSize,
            kXGreenEnable,
            kXGreen,
            kXLoPass,
            kXBlue,

            kNumXPositions
        };
          
        enum YPosition
        {
            kYEnable,
            kYFreq,
            kYFreqEditor,
            kYQFactor,
            kYQEditor,
            kYLevel,
            kYLevelEditor,
            kYShelveToggle,
            kYOrder,
            kYPassFreq,
            kYPassEditor,
            kYPassButton,

            kNumYPositions
        };

        Layout(TinyQ* tinyq);
        float getSizeFactor() const;
        int getKnobSize() const;
        int getEditorHeight() const;
        int getX(XPosition pos) const;
        int getY(YPosition pos) const;
        IRECT getKnobPosition(XPosition xPos, YPosition yPos) const;
        IRECT getSizeButtonRect(IBitmap* buttonImg) const;
        IRECT getFreqDisplayRect() const;
        TinyQ* tinyQ;
    };

    struct DefaultValues
    {
        DefaultValues(TinyQ* tinyQ = nullptr);
        bool bandEn[4] = {false, false, false, false};
        double bandFreq[4] = {100., 750., 3000., 10000. };
        double bandQ[4] = { 0.707, 0.707, 0.707, 0.707 };
        double bandGain[4] = {0., 0., 0., 0.};
        bool loShelf = false;
        bool hiShelf = false;
        bool cutEn[2] = {false, false};
        double cutFreq[2] = {20., 20e3};
        int cutOrder[2] = {0, 0};
        double gain = 0.f;
        bool showGain = false;
        bool showBackgroundText = true;
        float magRange = 24.f;
        bool showAnalyser = false;
        bool processAnalyserPre = true;
        float analyserMagTop = 10.f;
        float analyserMagRange = 60.f;

        juce::XmlElement* save() const;
        void load(const juce::XmlElement* xml);
    };

    Layout layout;
    BandAnalyser bandAnalyser;
    bool processAnalyserIn = true;
    bool guiOpen = false;
    bool showGain = false;
    bool showBackgroundText = true;
    double gainFactor = 1.;
    float freqViewMagRange = 24;
    float analyserMagTop = 10.f;
    float analyserMagRange = 60.f;

    IEditorControl* gainControl = nullptr;
    IGUIResizeButton* resizeButton = nullptr;
    FrequencyDisplay* freqDisplay = nullptr;
    BandAnalyserView* bandAnalyserView = nullptr;
};

#endif
