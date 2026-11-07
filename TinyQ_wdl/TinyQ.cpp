#include "TinyQ.h"
#include "IPlug_include_in_plug_src.h"
#include "IControl.h"
#include "IKnobMultiReversedControl.h"
#include "IEditorControl.h"
#include "FrequencyDisplay.h"
#include "resource.h"
#include "bandsorter.h"
#include <functional>

namespace
{
const juce::uint32 magicXmlNumber = 0x21324356;

void copyXmlToBinary (const juce::XmlElement& xml, juce::MemoryBlock& destData)
{
    {
        juce::MemoryOutputStream out (destData, false);
        out.writeInt (magicXmlNumber);
        out.writeInt (0);
        xml.writeToStream (out, "", true, false);
        out.writeByte (0);
    }

    static_cast<juce::uint32*> (destData.getData())[1] = juce::ByteOrder::swapIfBigEndian ((juce::uint32) destData.getSize() - 9);
}

juce::XmlElement* getXmlFromBinary (const void* data, const int sizeInBytes)
{
    if (sizeInBytes > 8 && juce::ByteOrder::littleEndianInt (data) == magicXmlNumber)
    {
        const int stringLength = (int) juce::ByteOrder::littleEndianInt (juce::addBytesToPointer (data, 4));

        if (stringLength > 0)
            return juce::XmlDocument::parse (juce::String::fromUTF8 (static_cast<const char*> (data) + 8, juce::jmin ((sizeInBytes - 8), stringLength)));
    }

    return nullptr;
}
}

const int kNumPrograms = 1;

class IGUIResizeButton : public IControl
{
private:
    TinyQ* tinyQ;
    IBitmap sizeButton;

public:
    IGUIResizeButton(TinyQ* pPlug, IRECT pR, IBitmap* button)
        :	IControl(pPlug, pR), tinyQ(pPlug), sizeButton(*button)
    {
    }

    ~IGUIResizeButton() {}

    bool Draw(IGraphics* pGraphics)
    {
        const int state = tinyQ->pluginSize == TinyQ::kTiny ? 1 : tinyQ->pluginSize == TinyQ::kLarge ? 2 : 3;         
        return pGraphics->DrawBitmap(&sizeButton, &mRECT, state);
    }

    void OnMouseDown(int x, int y, IMouseMod* pMod)
    {
        if (pMod->L)
            tinyQ->toggleResize(true);
        else if (pMod->R)
            IPlugBase::ShowMenu(tinyQ->getSizeMenu(), tinyQ->GetGUI(), IRECT(x, y, x, y));
    }
};

TinyQ::DefaultValues::DefaultValues(TinyQ* tinyQ)
{
    if (tinyQ != nullptr)
    {
        for (int i=0; i<4; ++i)
        {
            int baseIdx = i * kLoMidEnable;
            bandEn[i] = tinyQ->GetParam(baseIdx + kLowEnable)->Bool();
            bandFreq[i] = tinyQ->GetParam(baseIdx + kLowFreq)->Value();
            bandQ[i] = tinyQ->GetParam(baseIdx + kLowQ)->Value();
            bandGain[i] = tinyQ->GetParam(baseIdx + kLowGain)->Value();
        }
        loShelf = tinyQ->GetParam(kLoShelfSwitch)->Bool();
        hiShelf = tinyQ->GetParam(kHiShelfSwitch)->Bool();

        for (int i=0; i<2; ++i)
        {
            const int baseIdx = i * (kLPEnable-kHPEnable);
            cutEn[i] = tinyQ->GetParam(baseIdx + kHPEnable)->Bool();
            cutFreq[i] = tinyQ->GetParam(baseIdx + kHPFreq)->Value();
            cutOrder[i] = tinyQ->GetParam(baseIdx + kHPOrder)->Int();
        }

        gain = tinyQ->GetParam(kGain)->Value();
        magRange = tinyQ->freqViewMagRange;
        showGain = tinyQ->showGain;
        showBackgroundText = tinyQ->showBackgroundText;
        showAnalyser = tinyQ->bandAnalyser.getEnabled();
        processAnalyserPre = tinyQ->processAnalyserIn;
        analyserMagTop = tinyQ->analyserMagTop;
        analyserMagRange = tinyQ->analyserMagRange;
    }
}

juce::XmlElement* TinyQ::DefaultValues::save() const
{
    juce::XmlElement* xml = new juce::XmlElement("DEFAULTS");
    
    {
        const char* bandNames[4] = {"lo", "lomid", "himid", "hi"};
        const juce::String en("en_");
        const juce::String freq("freq_");
        const juce::String q("q_");
        const juce::String gain("gain_");

        for (int i=0; i<4; ++i)
        {
            xml->setAttribute(en + bandNames[i], bandEn[i]);
            xml->setAttribute(freq + bandNames[i], bandFreq[i]);
            xml->setAttribute(q + bandNames[i], bandQ[i]);
            xml->setAttribute(gain + bandNames[i], bandGain[i]);
        }
    }

    xml->setAttribute("loshelf", loShelf);
    xml->setAttribute("hishelf", hiShelf);

    {
        const char* cutNames[2] = {"locut", "hicut"};
        const juce::String en("cuten_");
        const juce::String freq("cutfreq_");
        const juce::String order("cutorder");

        for (int i=0; i<2; ++i)
        {
            xml->setAttribute(en + cutNames[i], cutEn[i]);
            xml->setAttribute(freq + cutNames[i], cutFreq[i]);
            xml->setAttribute(order + cutNames[i], cutOrder[i]);
        }
    }

    xml->setAttribute("gain", gain);
    xml->setAttribute("showgain", showGain);
    xml->setAttribute("showbgtext", showBackgroundText);
    xml->setAttribute("magrange", magRange);
    xml->setAttribute("showanalyser", showAnalyser);
    xml->setAttribute("annalyserpre", processAnalyserPre);
    xml->setAttribute("analysertop", analyserMagTop);
    xml->setAttribute("analyserrange", analyserMagRange);

    return xml;
}

void TinyQ::DefaultValues::load(const juce::XmlElement* xml)
{
    if (xml != nullptr && ! xml->hasTagName("DEFAULTS"))
        xml = xml->getChildByName("DEFAULTS");

    if (xml != nullptr && xml->hasTagName("DEFAULTS"))
    {
        {
            const char* bandNames[4] = {"lo", "lomid", "himid", "hi"};
            const juce::String en("en_");
            const juce::String freq("freq_");
            const juce::String q("q_");
            const juce::String gain("gain_");

            for (int i=0; i<4; ++i)
            {
                bandEn[i] = xml->getBoolAttribute(en + bandNames[i], bandEn[i]);
                bandFreq[i] = xml->getDoubleAttribute(freq + bandNames[i], bandFreq[i]);
                bandQ[i] = xml->getDoubleAttribute(q + bandNames[i], bandQ[i]);
                bandGain[i] = xml->getDoubleAttribute(gain + bandNames[i], bandGain[i]);
            }
        }

        loShelf = xml->getBoolAttribute("loshelf", loShelf);
        hiShelf = xml->getBoolAttribute("hishelf", hiShelf);

        {
            const char* cutNames[2] = {"locut", "hicut"};
            const juce::String en("cuten_");
            const juce::String freq("cutfreq_");
            const juce::String order("cutorder");

            for (int i=0; i<2; ++i)
            {
                cutEn[i] = xml->getBoolAttribute(en + cutNames[i], cutEn[i]);
                cutFreq[i] = xml->getDoubleAttribute(freq + cutNames[i], cutFreq[i]);
                cutOrder[i] = xml->getIntAttribute(order + cutNames[i], cutOrder[i]);
            }
        }

        gain = xml->getDoubleAttribute("gain", gain);
        showGain = xml->getBoolAttribute("showgain", showGain);
        showBackgroundText = xml->getBoolAttribute("showbgtext", showBackgroundText);
        magRange = static_cast<float> (xml->getDoubleAttribute("magrange", magRange));
        showAnalyser = xml->getBoolAttribute("showanalyser", showAnalyser);
        processAnalyserPre = xml->getBoolAttribute("annalyserpre", processAnalyserPre);
        analyserMagTop = static_cast<float> (xml->getDoubleAttribute("analysertop", analyserMagTop));
        analyserMagRange = static_cast<float> (xml->getDoubleAttribute("analyserrange", analyserMagRange));
    }
}

TinyQ::Layout::Layout(TinyQ* tinyq)
    : tinyQ(tinyq)
{
}

float TinyQ::Layout::getSizeFactor() const
{
    return tinyQ->pluginSize == kTiny ? 1.f : tinyQ->pluginSize == kLarge ? 1.5f : 2.f;
}

int TinyQ::Layout::getKnobSize() const
{
    return static_cast<int> (28 * getSizeFactor());
}

int TinyQ::Layout::getEditorHeight() const
{
    return static_cast<int> (10 * getSizeFactor());
}

int TinyQ::Layout::getX(XPosition pos) const
{
    const float sizeFactor = getSizeFactor();
    const int defPos[kNumXPositions] = { 12, 15, 60, 63, 85, 108, 111, 156, 159 };
    return static_cast<int> (defPos[pos] * sizeFactor + 0.5f);
}

int TinyQ::Layout::getY(YPosition pos) const
{
    const float sizeFactor = getSizeFactor();
    const int defPos[kNumYPositions] = { 120, 141, 172, 189, 220, 237, 268, 281, 303, 315, 346, 360 };
    return static_cast<int> (defPos[pos] * sizeFactor + 0.5f);
}

IRECT TinyQ::Layout::getKnobPosition(XPosition xPos, YPosition yPos) const
{
    const int x = getX(xPos);
    const int y = getY(yPos);
    const int w = getKnobSize();
    return IRECT(x, y, x+w, y+w);
}

IRECT TinyQ::Layout::getSizeButtonRect(IBitmap* buttonImg) const
{
    const int x = getX(Layout::kXSize);
    const int y = getY(Layout::kYPassButton);
    return buttonImg != nullptr ? IRECT(x, y, x+buttonImg->W, y+buttonImg->H/buttonImg->N) : IRECT(x, y, x, y);
}

IRECT TinyQ::Layout::getFreqDisplayRect() const
{
    const float sizeMul = getSizeFactor();
    const int x = static_cast<int> (5 * sizeMul + 0.5f);
    const int y = static_cast<int> (5 * sizeMul + 0.5f);
    const int w = static_cast<int> (194 * sizeMul + 0.5f);
    const int h = static_cast<int> (109 * sizeMul + 0.5f);
    return IRECT(x, y, x+w, y+h);
}

TinyQ::TinyQ(IPlugInstanceInfo instanceInfo)
    :	IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo), layout(this)
{
    TRACE;
    DefaultValues defaults;

    {
        std::unique_ptr<juce::XmlElement> xmlSettings(getSettingsXml());
        defaults.load(xmlSettings.get());
        bandAnalyser.setEnabled(defaults.showAnalyser);
        processAnalyserIn = defaults.processAnalyserPre;
        analyserMagTop = defaults.analyserMagTop;
        analyserMagRange = defaults.analyserMagRange;
    }

    GetParam(kLowEnable)->InitBool("Band_Low", defaults.bandEn[0]);
    GetParam(kLowFreq)->InitDouble("Freq_Low", defaults.bandFreq[0], 20., 20e3, 1, "Hz");
    GetParam(kLowQ)->InitDouble("Q_Low", defaults.bandQ[0], 0.3, 12, 0.01);
    GetParam(kLowGain)->InitDouble("Gain_Low", defaults.bandGain[0], -24., 24., 0.1, "dB");

    GetParam(kLoMidEnable)->InitBool("Band_LoMid", defaults.bandEn[1]);
    GetParam(kLoMidFreq)->InitDouble("Freq_LoMid", defaults.bandFreq[1], 20., 20e3, 1, "Hz");
    GetParam(kLoMidQ)->InitDouble("Q_LoMid", defaults.bandQ[1], 0.3, 12, 0.01);
    GetParam(kLoMidGain)->InitDouble("Gain_LoMid", defaults.bandGain[1], -24., 24., 0.1, "dB");

    GetParam(kHiMidEnable)->InitBool("Band_HiMid", defaults.bandEn[2]);
    GetParam(kHiMidFreq)->InitDouble("Freq_HiMid", defaults.bandFreq[2], 20., 20e3, 1, "Hz");
    GetParam(kHiMidQ)->InitDouble("Q_HiMid", defaults.bandQ[2], 0.3, 12, 0.01);
    GetParam(kHiMidGain)->InitDouble("Gain_HiMid", defaults.bandGain[2], -24., 24., 0.1, "dB");

    GetParam(kHighEnable)->InitBool("Band_High", defaults.bandEn[3]);
    GetParam(kHighFreq)->InitDouble("Freq_High", defaults.bandFreq[3], 20., 20e3, 1, "Hz");
    GetParam(kHighQ)->InitDouble("Q_High", defaults.bandQ[3], 0.3, 12, 0.01);
    GetParam(kHighGain)->InitDouble("Gain_High", defaults.bandGain[3], -24., 24., 0.1, "dB");

    GetParam(kLoShelfSwitch)->InitBool("LoShelf", defaults.loShelf);
    GetParam(kHiShelfSwitch)->InitBool("HiShelf", defaults.hiShelf);
    GetParam(kHPEnable)->InitBool("Band_HP", defaults.cutEn[0]);
    GetParam(kHPFreq)->InitDouble("Freq_HP", defaults.cutFreq[0], 20., 20e3, 1, "Hz");
    GetParam(kHPOrder)->InitInt("Order_HP", defaults.cutOrder[0], 0, 7, "dB/Oct");
    GetParam(kLPEnable)->InitBool("Band_LP", defaults.cutEn[1]);
    GetParam(kLPFreq)->InitDouble("Freq_LP", defaults.cutFreq[1], 20., 20e3, 1, "Hz");
    GetParam(kLPOrder)->InitInt("Order_LP", defaults.cutOrder[1], 0, 7, "dB/Oct");

    GetParam(kGain)->InitDouble("Gain", defaults.gain, -24.f, 24.f, 0.1, "dB");

    freqViewMagRange = defaults.magRange;
    showGain = defaults.showGain;
    showBackgroundText = defaults.showBackgroundText;

    {
        const EParams logParams[10] = { kLowFreq, kLowQ, kLoMidFreq, kLoMidQ, kHiMidFreq, kHiMidQ, kHighFreq, kHighQ, kHPFreq, kLPFreq};

        for (int i=0; i<10; ++i)
            GetParam(logParams[i])->SetShape(2.);
    }

    {
        std::unique_ptr<juce::XmlElement> xml(getSettingsXml());

        if (xml != nullptr && xml->hasAttribute("size"))
            pluginSize = static_cast<PluginSize> (juce::jlimit(0, 2, xml->getIntAttribute("size")));
    }

    IGraphics* pGraphics = MakeGraphics(this, getSupposedWidth(), getSupposedHeight());

    CreateControls(pGraphics);

    AttachGraphics(pGraphics);
    pGraphics->HandleMouseOver(true);
    updateAnalyserRange();
}

TinyQ::~TinyQ() 
{
}

bool TinyQ::SerializeState(ByteChunk* pChunk)
{
    juce::XmlElement xml("TINYQ");
    const double sizeFactor = pluginSize == kTiny ? 1.0 : pluginSize == kLarge ? 1.5 : 2.0;
    xml.setAttribute("guisize", sizeFactor);
    xml.setAttribute("wdl", true);
    xml.setAttribute("version", PLUG_VER); 
    xml.setAttribute("showgain", showGain);
    xml.setAttribute("showbgtext", showBackgroundText);
    xml.setAttribute("freqviewmagrange", freqViewMagRange);
    xml.setAttribute("analyser", bandAnalyser.getEnabled());
    xml.setAttribute("analyserinput", processAnalyserIn);
    xml.setAttribute("analysertop", analyserMagTop);
    xml.setAttribute("analyserrange", analyserMagRange);

    const int numParams = NParams();

    for (int i=0; i<numParams; ++i)
    {
        IParam* p = GetParam(i);
        juce::XmlElement* e = xml.createNewChildElement("PARAM");
        e->setAttribute("id", juce::String(p->GetNameForHost()));
        e->setAttribute("value", p->Value());
    }

    juce::MemoryBlock mb;
    copyXmlToBinary(xml, mb);
    pChunk->PutBytes(mb.getData(), static_cast<int> (mb.getSize()));
    return true;
}

int TinyQ::UnserializeState(ByteChunk* pChunk, int startPos)
{
    juce::MemoryBlock mb(pChunk->GetBytes() + startPos, static_cast<size_t> (pChunk->Size() - startPos));
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(mb.getData(), mb.getSize()));

    if (xml.get() != nullptr)
    {
        const PluginSize curSize = pluginSize;
        const bool wdl = xml->getBoolAttribute("wdl");

        const double sizeFactor = xml->getDoubleAttribute("guisize");
        pluginSize = sizeFactor < 1.25 ? kTiny : sizeFactor < 1.75 ? kLarge : kHuge;
        showGain = xml->getBoolAttribute("showgain", showGain);
        showBackgroundText = xml->getBoolAttribute("showbgtext", showBackgroundText);

        bandAnalyser.setEnabled(xml->getBoolAttribute("analyser", bandAnalyser.getEnabled()));
        processAnalyserIn = xml->getBoolAttribute("analyserinput", processAnalyserIn);
        analyserMagTop = static_cast<float> (xml->getDoubleAttribute("analysertop", analyserMagTop));
        analyserMagRange = static_cast<float> (xml->getDoubleAttribute("analyserrange", analyserMagRange));

        const float newMagRange = static_cast<float> (xml->getDoubleAttribute("freqviewmagrange", freqViewMagRange));

        forEachXmlChildElementWithTagName(*xml, e, "PARAM")
        {
            const juce::String name(e->getStringAttribute("id"));
            const int idx = getParamIndexForName(name);

            if (IParam* p = GetParam(idx))
            {
                double val = e->getDoubleAttribute("value", p->GetNormalized());

                if (! wdl && idx == kLoShelfSwitch)
                    val = val > 0.5 ? 0 : 1;

                p->Set(val);
            }
        }

        if (curSize != pluginSize)
            toggleResize(false);

        if (newMagRange != freqViewMagRange)
        {
            freqViewMagRange = newMagRange;
            freqDisplay->SetDirty(false);
        }

        updateShowGain();
        updateAnalyserRange();
        OnParamReset();
    }

    return static_cast<int> (mb.getSize());
}

void TinyQ::CreateControls(IGraphics* pGraphics)
{
    IBitmap knobRed;
    IBitmap knobYellow;
    IBitmap knobGreen;
    IBitmap knobBlue;
    IBitmap knobGrey;
    IBitmap knobHPOrder;
    IBitmap knobLPOrder;
    IBitmap switchRed;
    IBitmap switchYellow;
    IBitmap switchGreen;
    IBitmap switchBlue;
    IBitmap switchHP;
    IBitmap switchLP;
    IBitmap switchSize;
    IBitmap editorFont;
    IBitmap switchShelf;

    const int numKnobFrames = 240;
    const int numOrderFrames = 8;
    const int numEnableFrames = 2;
    const int numFontFrames = 13;

    switch(pluginSize)
    {
    default:
    case kTiny:
        pGraphics->AttachBackground(BACKGROUND_1_0_ID, BACKGROUND_1_0_FN);
        knobRed = pGraphics->LoadIBitmap(KNOB_RED_1_0_ID, KNOB_RED_1_0_FN, numKnobFrames);
        knobYellow = pGraphics->LoadIBitmap(KNOB_YELLOW_1_0_ID, KNOB_YELLOW_1_0_FN, numKnobFrames);
        knobGreen = pGraphics->LoadIBitmap(KNOB_GREEN_1_0_ID, KNOB_GREEN_1_0_FN, numKnobFrames);
        knobBlue = pGraphics->LoadIBitmap(KNOB_BLUE_1_0_ID, KNOB_BLUE_1_0_FN, numKnobFrames);
        knobGrey = pGraphics->LoadIBitmap(KNOB_GREY_1_0_ID, KNOB_GREY_1_0_FN, numKnobFrames);
        knobHPOrder = pGraphics->LoadIBitmap(KNOB_HPORDER_1_0_ID, KNOB_HPORDER_1_0_FN, numOrderFrames);
        knobLPOrder = pGraphics->LoadIBitmap(KNOB_LPORDER_1_0_ID, KNOB_LPORDER_1_0_FN, numOrderFrames);
        switchRed = pGraphics->LoadIBitmap(BUTTON_LOW_1_0_ID, BUTTON_LOW_1_0_FN, numEnableFrames);
        switchYellow = pGraphics->LoadIBitmap(BUTTON_LOMID_1_0_ID, BUTTON_LOMID_1_0_FN, numEnableFrames);
        switchGreen = pGraphics->LoadIBitmap(BUTTON_HIMID_1_0_ID, BUTTON_HIMID_1_0_FN, numEnableFrames);
        switchBlue = pGraphics->LoadIBitmap(BUTTON_HIGH_1_0_ID, BUTTON_HIGH_1_0_FN, numEnableFrames);
        switchHP = pGraphics->LoadIBitmap(BUTTON_HP_1_0_ID, BUTTON_HP_1_0_FN, numEnableFrames);
        switchLP = pGraphics->LoadIBitmap(BUTTON_LP_1_0_ID, BUTTON_LP_1_0_FN, numEnableFrames);
        switchSize = pGraphics->LoadIBitmap(BUTTON_SIZE_1_0_ID, BUTTON_SIZE_1_0_FN, 3);
        editorFont = pGraphics->LoadIBitmap(EDITOR_FONT_1_0_ID, EDITOR_FONT_1_0_FN, numFontFrames);
        switchShelf = pGraphics->LoadIBitmap(BUTTON_SHELF_1_0_ID, BUTTON_SHELF_1_0_FN, 2);
        break;
    case kLarge:
        pGraphics->AttachBackground(BACKGROUND_1_5_ID, BACKGROUND_1_5_FN);
        knobRed = pGraphics->LoadIBitmap(KNOB_RED_1_5_ID, KNOB_RED_1_5_FN, numKnobFrames);
        knobYellow = pGraphics->LoadIBitmap(KNOB_YELLOW_1_5_ID, KNOB_YELLOW_1_5_FN, numKnobFrames);
        knobGreen = pGraphics->LoadIBitmap(KNOB_GREEN_1_5_ID, KNOB_GREEN_1_5_FN, numKnobFrames);
        knobBlue = pGraphics->LoadIBitmap(KNOB_BLUE_1_5_ID, KNOB_BLUE_1_5_FN, numKnobFrames);
        knobGrey = pGraphics->LoadIBitmap(KNOB_GREY_1_5_ID, KNOB_GREY_1_5_FN, numKnobFrames);
        knobHPOrder = pGraphics->LoadIBitmap(KNOB_HPORDER_1_5_ID, KNOB_HPORDER_1_5_FN, numOrderFrames);
        knobLPOrder = pGraphics->LoadIBitmap(KNOB_LPORDER_1_5_ID, KNOB_LPORDER_1_5_FN, numOrderFrames);
        switchRed = pGraphics->LoadIBitmap(BUTTON_LOW_1_5_ID, BUTTON_LOW_1_5_FN, numEnableFrames);
        switchYellow = pGraphics->LoadIBitmap(BUTTON_LOMID_1_5_ID, BUTTON_LOMID_1_5_FN, numEnableFrames);
        switchGreen = pGraphics->LoadIBitmap(BUTTON_HIMID_1_5_ID, BUTTON_HIMID_1_5_FN, numEnableFrames);
        switchBlue = pGraphics->LoadIBitmap(BUTTON_HIGH_1_5_ID, BUTTON_HIGH_1_5_FN, numEnableFrames);
        switchHP = pGraphics->LoadIBitmap(BUTTON_HP_1_5_ID, BUTTON_HP_1_5_FN, numEnableFrames);
        switchLP = pGraphics->LoadIBitmap(BUTTON_LP_1_5_ID, BUTTON_LP_1_5_FN, numEnableFrames);
        switchSize = pGraphics->LoadIBitmap(BUTTON_SIZE_1_5_ID, BUTTON_SIZE_1_5_FN, 3);
        editorFont = pGraphics->LoadIBitmap(EDITOR_FONT_1_5_ID, EDITOR_FONT_1_5_FN, numFontFrames);
        switchShelf = pGraphics->LoadIBitmap(BUTTON_SHELF_1_5_ID, BUTTON_SHELF_1_5_FN, 2);
        break;
    case kHuge:
        pGraphics->AttachBackground(BACKGROUND_2_0_ID, BACKGROUND_2_0_FN);
        knobRed = pGraphics->LoadIBitmap(KNOB_RED_2_0_ID, KNOB_RED_2_0_FN, numKnobFrames);
        knobYellow = pGraphics->LoadIBitmap(KNOB_YELLOW_2_0_ID, KNOB_YELLOW_2_0_FN, numKnobFrames);
        knobGreen = pGraphics->LoadIBitmap(KNOB_GREEN_2_0_ID, KNOB_GREEN_2_0_FN, numKnobFrames);
        knobBlue = pGraphics->LoadIBitmap(KNOB_BLUE_2_0_ID, KNOB_BLUE_2_0_FN, numKnobFrames);
        knobGrey = pGraphics->LoadIBitmap(KNOB_GREY_2_0_ID, KNOB_GREY_2_0_FN, numKnobFrames);
        knobHPOrder = pGraphics->LoadIBitmap(KNOB_HPORDER_2_0_ID, KNOB_HPORDER_2_0_FN, numOrderFrames);
        knobLPOrder = pGraphics->LoadIBitmap(KNOB_LPORDER_2_0_ID, KNOB_LPORDER_2_0_FN, numOrderFrames);
        switchRed = pGraphics->LoadIBitmap(BUTTON_LOW_2_0_ID, BUTTON_LOW_2_0_FN, numEnableFrames);
        switchYellow = pGraphics->LoadIBitmap(BUTTON_LOMID_2_0_ID, BUTTON_LOMID_2_0_FN, numEnableFrames);
        switchGreen = pGraphics->LoadIBitmap(BUTTON_HIMID_2_0_ID, BUTTON_HIMID_2_0_FN, numEnableFrames);
        switchBlue = pGraphics->LoadIBitmap(BUTTON_HIGH_2_0_ID, BUTTON_HIGH_2_0_FN, numEnableFrames);
        switchHP = pGraphics->LoadIBitmap(BUTTON_HP_2_0_ID, BUTTON_HP_2_0_FN, numEnableFrames);
        switchLP = pGraphics->LoadIBitmap(BUTTON_LP_2_0_ID, BUTTON_LP_2_0_FN, numEnableFrames);
        switchSize = pGraphics->LoadIBitmap(BUTTON_SIZE_2_0_ID, BUTTON_SIZE_2_0_FN, 3);
        editorFont = pGraphics->LoadIBitmap(EDITOR_FONT_2_0_ID, EDITOR_FONT_2_0_FN, numFontFrames);
        switchShelf = pGraphics->LoadIBitmap(BUTTON_SHELF_2_0_ID, BUTTON_SHELF_2_0_FN, 2);
        break;
    }

    const EParams freqs[4] = { kLowFreq, kLoMidFreq, kHiMidFreq, kHighFreq };
    const EParams Qs[4] = { kLowQ, kLoMidQ, kHiMidQ, kHighQ };
    const EParams gains[4] = { kLowGain, kLoMidGain, kHiMidGain, kHighGain };
    IBitmap* bandKnobs[4] = { &knobRed, &knobYellow, &knobGreen, &knobBlue };
    Layout::XPosition knobXPos[4] = { Layout::kXRed, Layout::kXYellow, Layout::kXGreen, Layout::kXBlue };
    Layout::XPosition edXPos[4] = { Layout::kXHiPass, Layout::kXYellowEnable, Layout::kXGreenEnable, Layout::kXLoPass };
    IKnobPollingControl* gainParams[4];
    IEditorControl* gainEditors[4];
    ISwitchControl* bandEnable[4];

    {
        const int wEd = switchRed.W;
        const int hEd = layout.getEditorHeight();
        const int yFrq = layout.getY(Layout::kYFreq);
        const int yQ = layout.getY(Layout::kYQFactor);
        const int yLev = layout.getY(Layout::kYLevel);
        const int yFrqEd = layout.getY(Layout::kYFreqEditor);
        const int yQEd = layout.getY(Layout::kYQEditor);
        const int yLevEd = layout.getY(Layout::kYLevelEditor);


        for (int i=0; i<4; ++i)
        {
            const int xKnob = layout.getX(knobXPos[i]);
            const int xEd = layout.getX(edXPos[i]);
            pGraphics->AttachControl(new IKnobPollingControl(this, xKnob, yFrq, freqs[i], bandKnobs[i]));
            pGraphics->AttachControl(new IKnobPollingControl(this, xKnob, yQ, Qs[i], bandKnobs[i]));
            pGraphics->AttachControl(gainParams[i] = new IKnobPollingControl(this, xKnob, yLev, gains[i], bandKnobs[i]));
            IEditorControl* freqEd = new IEditorControl(this, IRECT(xEd, yFrqEd, xEd + wEd, yFrqEd + hEd), freqs[i], &editorFont);
            IEditorControl* qEd = new IEditorControl(this, IRECT(xEd, yQEd, xEd + wEd, yQEd + hEd), Qs[i], &editorFont);
            pGraphics->AttachControl(freqEd);
            pGraphics->AttachControl(qEd);
            pGraphics->AttachControl(gainEditors[i] = new IEditorControl(this, IRECT(xEd, yLevEd, xEd + wEd, yLevEd + hEd), gains[i], &editorFont));
            freqEd->SetIsFreq();
            qEd->setNumDecimals(2);
        }

        const int xHpEd = layout.getX(Layout::kXHiPass);
        const int xLpEd = layout.getX(Layout::kXLoPass);
        const int yPassEd = layout.getY(Layout::kYPassEditor);
        IEditorControl* hpFreqEd = new IEditorControl(this, IRECT(xHpEd, yPassEd, xHpEd + wEd, yPassEd + hEd), kHPFreq, &editorFont);
        IEditorControl* lpFreqEd = new IEditorControl(this, IRECT(xLpEd, yPassEd, xLpEd + wEd, yPassEd + hEd), kLPFreq, &editorFont);
        pGraphics->AttachControl(hpFreqEd);
        pGraphics->AttachControl(lpFreqEd);
        hpFreqEd->SetIsFreq();
        lpFreqEd->SetIsFreq();
    }

    IKnobPollingControl* orderControl = nullptr;

    pGraphics->AttachControl(new IKnobPollingControl(this, layout.getX(Layout::kXRed), layout.getY(Layout::kYPassFreq), kHPFreq, &knobGrey));
    pGraphics->AttachControl(new IKnobPollingControl(this, layout.getX(Layout::kXBlue), layout.getY(Layout::kYPassFreq), kLPFreq, &knobGrey));
    pGraphics->AttachControl(orderControl = new IKnobMultiReversedControl(this, layout.getX(Layout::kXYellow), layout.getY(Layout::kYOrder), kHPOrder, &knobHPOrder));
    orderControl->setSteppedMouseWheel(true);
    pGraphics->AttachControl(orderControl = new IKnobPollingControl(this, layout.getX(Layout::kXGreen), layout.getY(Layout::kYOrder), kLPOrder, &knobLPOrder));
    orderControl->setSteppedMouseWheel(true);

    pGraphics->AttachControl(bandEnable[0] = new ISwitchControl(this, layout.getX(Layout::kXHiPass), layout.getY(Layout::kYEnable), kLowEnable, &switchRed));
    pGraphics->AttachControl(bandEnable[1] = new ISwitchControl(this, layout.getX(Layout::kXYellowEnable), layout.getY(Layout::kYEnable), kLoMidEnable, &switchYellow));
    pGraphics->AttachControl(bandEnable[2] = new ISwitchControl(this, layout.getX(Layout::kXGreenEnable), layout.getY(Layout::kYEnable), kHiMidEnable, &switchGreen));
    pGraphics->AttachControl(bandEnable[3] = new ISwitchControl(this, layout.getX(Layout::kXLoPass), layout.getY(Layout::kYEnable), kHighEnable, &switchBlue));
    pGraphics->AttachControl(new ISwitchControl(this, layout.getX(Layout::kXHiPass), layout.getY(Layout::kYPassButton), kHPEnable, &switchHP));
    pGraphics->AttachControl(new ISwitchControl(this, layout.getX(Layout::kXLoPass), layout.getY(Layout::kYPassButton), kLPEnable, &switchLP));

    pGraphics->AttachControl(new ISwitchReversedControl(this, layout.getX(Layout::kXHiPass), layout.getY(Layout::kYShelveToggle), kLoShelfSwitch, &switchShelf));
    pGraphics->AttachControl(new ISwitchControl(this, layout.getX(Layout::kXLoPass), layout.getY(Layout::kYShelveToggle), kHiShelfSwitch, &switchShelf));

    pGraphics->AttachControl(gainControl = new IEditorControl(this, layout.getSizeButtonRect(&switchSize), kGain, &editorFont));
    pGraphics->AttachControl(resizeButton = new IGUIResizeButton(this, layout.getSizeButtonRect(&switchSize), &switchSize));    

    updateShowGain();

    pGraphics->AttachControl(freqDisplay = new FrequencyDisplay(this, layout.getFreqDisplayRect()));
    bandAnalyserView = new BandAnalyserView(bandAnalyser);

    {
        std::function<float(float)> f2x = [this] (float f) {
            return freqDisplay->freqToX(f);
        };
        std::function<float(float)> x2f = [this] (float x) {
            return freqDisplay->xToFreq(x);
        };

        const juce::Range<float> magRange(freqDisplay->getYRange());
        bandAnalyserView->setFreqMapping(f2x, x2f);
        bandAnalyserView->setMagnitudePositions(magRange.getStart(), magRange.getEnd());
        freqDisplay->setBandAnalyserView(bandAnalyserView);
    }

    for (int i=0; i<pGraphics->GetNControls(); ++i)
    {
        IControl* control = pGraphics->GetControl(i);
        if (control != nullptr && control->GetParam() != nullptr)
            control->SetValueFromPlug(control->GetParam()->Value());
    }

    {
        const int enableIdx[4] = {kLowEnable, kLoMidEnable, kHiMidEnable, kHighEnable};

        for (int i=0; i<4; ++i)
        {
            const int paramIdx = enableIdx[i];
            ISwitchControl* bandEn = bandEnable[i];

            std::function<void()> f = [this, paramIdx, bandEn]() { 
                    if (IParam* p = GetParam(paramIdx))
                    {
                        if (! p->Bool())
                        {
                            bandEn->SetValueFromPlug(1);
                            SetParameterFromGUI(paramIdx, 1.);
                        }
                    }
                };
            gainParams[i]->onParamChanged(f);
            gainEditors[i]->onParamChanged(f);
        }
    }
}

void TinyQ::UpdateParameters()
{
    const float sampleRate = static_cast<float> (GetSampleRate());
    const int hpOrder = static_cast<int> (GetParam(kHPOrder)->Value() + 1.5);
    const int lpOrder = static_cast<int> (GetParam(kLPOrder)->Value() + 1.5);
    const float hpFreq = static_cast<float> (GetParam(kHPFreq)->Value());
    const float lpFreq = static_cast<float> (GetParam(kLPFreq)->Value());

    passDsp[0].setFilter(hpOrder, hpFreq, TQFilterStage::passHiPass, sampleRate);
    passDsp[1].setFilter(lpOrder, lpFreq, TQFilterStage::passLoPass, sampleRate);

    TQFilterStage::FilterType filterType[4] = { TQFilterStage::filterTypePeak, TQFilterStage::filterTypePeak, TQFilterStage::filterTypePeak, TQFilterStage::filterTypePeak };

    if (GetParam(kLoShelfSwitch)->Bool())
        filterType[0] = TQFilterStage::filterTypeLoShelf;

    if (GetParam(kHiShelfSwitch)->Bool())
        filterType[3] = TQFilterStage::filterTypeHiShelf;

    for (int i=0; i<4; ++i)
    {
        const float freq = static_cast<float> (GetParam(4*i + 1)->Value());
        const float Q = static_cast<float> (GetParam(4*i + 2)->Value());
        const float gain = static_cast<float> (GetParam(4*i + 3)->Value());
        bandDsp[i].setFilter(freq, Q, gain, filterType[i], sampleRate);
    }

    gainFactor = showGain ? DBToAmp(GetParam(kGain)->Value()) : 1.f;
}

void TinyQ::ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames)
{
    UpdateParameters();
    SimdBandFilter::ScopedCSR csr;

    double* in1 = inputs[0];
    double* in2 = inputs[1] != nullptr ? inputs[1] : in1;
    double* out1 = outputs[0];
    double* out2 = outputs[1];

    const int blockSize = 128;
    float chL[blockSize];
    float chR[blockSize];

    std::function<void(float*, const double*, int, double)> convertToFloat = [] (float* out, const double* in, int numSamples, double gain) { 
        for (int i=0; i<numSamples; ++i)
            out[i] = static_cast<float> (in[i] * gain);
    };
    std::function<void(double*, const float*, int)> convertToDouble = [] (double* out, const float* in, int numSamples) { 
        for (int i=0; i<numSamples; ++i)
            out[i] = in[i];
    };

    const EParams bandEnableId[4] = { kLowEnable, kLoMidEnable, kHiMidEnable, kHighEnable };

    while (nFrames > 0)
    {
        const int numSamples = juce::jmin(blockSize, nFrames);

        if (processAnalyserIn)
            processAnalyser(in1, in2, numSamples);

        convertToFloat(chL, in1, numSamples, gainFactor);
        convertToFloat(chR, in2, numSamples, gainFactor);
        in1 += numSamples;
        in2 += numSamples;

        {
            if (GetParam(kHPEnable)->Bool())
                passDsp[0].processBlock(chL, chR, numSamples);

            if (GetParam(kLPEnable)->Bool())
                passDsp[1].processBlock(chL, chR, numSamples);
        }

        for (int i=0; i<4; ++i)
        {
            if (GetParam(bandEnableId[i])->Bool())
                bandDsp[i].processBlock(chL, chR, numSamples);
        }

        convertToDouble(out1, chL, numSamples);

        if (out2 != nullptr)
            convertToDouble(out2, chR, numSamples);

        if (! processAnalyserIn)
            processAnalyser(out1, out2, numSamples);

        out1 += numSamples;
        out2 += out2 != nullptr ? numSamples : 0;

        nFrames -= numSamples;
    }
}

int TinyQ::getSupposedWidth() const
{
    const int widths[3] = { 204, 306, 408 };
    const int sizeIdx = pluginSize == kTiny ? 0 : pluginSize == kLarge ? 1 : 2;
    return widths[sizeIdx];
}

int TinyQ::getSupposedHeight() const
{
    const int heights[3] = { 379, 569, 758 };
    const int sizeIdx = pluginSize == kTiny ? 0 : pluginSize == kLarge ? 1 : 2;
    return heights[sizeIdx];
}

void TinyQ::toggleResize(bool increase)
{
    if (increase)
        pluginSize = pluginSize == kTiny ? kLarge : pluginSize == kLarge ? kHuge : kTiny;

    if (guiOpen)
        GetGUI()->Resize(getSupposedWidth(), getSupposedHeight());
}

void TinyQ::OnGUIOpen()
{
    GetGUI()->Resize(getSupposedWidth(), getSupposedHeight());
    guiOpen = true;
}

void TinyQ::OnGUIClose()
{
    guiOpen = false;
}

IControl* TinyQ::getControlForParameter(int paramIdx)
{
    IParam* compareParam = GetParam(paramIdx);

    if (compareParam == nullptr)
        return nullptr;

    for (int i=0; i<GetGUI()->GetNControls(); ++i)
        if (compareParam == GetGUI()->GetControl(i)->GetParam())
            return GetGUI()->GetControl(i);

    return nullptr;
}

float TinyQ::getFreqViewMagRange() const
{
    return freqViewMagRange;
}

BandAnalyser& TinyQ::getBandAnalyser()
{
    return bandAnalyser;
}

IPopupMenu* TinyQ::getSizeMenu()
{
    IPopupMenu* menu = new IPopupMenu();

    std::function<void()> setDefSize = [this] () {
        std::unique_ptr<juce::XmlElement> xml(getSettingsXml());
        xml->setAttribute("size", static_cast<int> (pluginSize));
        setSettingsXml(xml.get()); 
    };

    std::function<void()> setTiny = [this] () { pluginSize = kTiny; toggleResize(false); };
    std::function<void()> setLarge = [this] () { pluginSize = kLarge; toggleResize(false); };
    std::function<void()> setHuge = [this] () { pluginSize = kHuge; toggleResize(false); };

    menu->AddItem(new IPopupMenuItem("Set Current Size as Default Size", IPopupMenuItem::kNoFlags, setDefSize));
    menu->AddItem(new IPopupMenuItem("", IPopupMenuItem::kSeparator));
    menu->AddItem(new IPopupMenuItem("Tiny", pluginSize == kTiny ? IPopupMenuItem::kChecked : IPopupMenuItem::kNoFlags, setTiny));
    menu->AddItem(new IPopupMenuItem("Large", pluginSize == kLarge ? IPopupMenuItem::kChecked : IPopupMenuItem::kNoFlags, setLarge));
    menu->AddItem(new IPopupMenuItem("Huge", pluginSize == kHuge ? IPopupMenuItem::kChecked : IPopupMenuItem::kNoFlags, setHuge));

    return menu;
}

IPopupMenu* TinyQ::getDefaultsMenu()
{
    IPopupMenu* menu = new IPopupMenu();
    std::function<void()> setDefaults = [this] () {
        DefaultValues defaults(this);
        std::unique_ptr<juce::XmlElement> xml(getSettingsXml());
        juce::XmlElement* defXml = defaults.save();

        if (juce::XmlElement* e = xml->getChildByName("DEFAULTS"))
            xml->replaceChildElement(e, defXml);
        else
            xml->addChildElement(defXml);

        xml->writeToFile(getSettingsFile(), "");
    };

    std::function<void()> resetDefaults = [this] () {
        std::unique_ptr<juce::XmlElement> xml(getSettingsXml());

        while (juce::XmlElement* e = xml->getChildByName("DEFAULTS"))
            xml->removeChildElement(e, true);

        xml->writeToFile(getSettingsFile(), "");
    };

    menu->AddItem(new IPopupMenuItem("Setting default parameters will only affect loading new instances!", IPopupMenuItem::kDisabled));
    menu->AddItem(new IPopupMenuItem("Set Current Parameters as Default", IPopupMenuItem::kNoFlags, setDefaults));
    menu->AddItem(new IPopupMenuItem("Reset Factory Default Parameters", IPopupMenuItem::kNoFlags, resetDefaults));
    return menu;
}

IPopupMenu* TinyQ::getFreqViewMenu()
{
    std::function<void()> set6Db = [this] () { freqViewMagRange = 6.f; freqDisplay->SetDirty(false); };
    std::function<void()> set12Db = [this] () { freqViewMagRange = 12.f; freqDisplay->SetDirty(false); };
    std::function<void()> set18Db = [this] () { freqViewMagRange = 18.f; freqDisplay->SetDirty(false); };
    std::function<void()> set24Db = [this] () { freqViewMagRange = 24.f; freqDisplay->SetDirty(false); };

    IPopupMenu* menu = new IPopupMenu();
    menu->AddItem("Gain Range", -1, IPopupMenuItem::kDisabled);
    menu->AddItem(new IPopupMenuItem("+/-  6 dB", freqViewMagRange == 6.f ? IPopupMenuItem::kChecked : IPopupMenuItem::kNoFlags, set6Db));
    menu->AddItem(new IPopupMenuItem("+/- 12 dB", freqViewMagRange == 12.f ? IPopupMenuItem::kChecked : IPopupMenuItem::kNoFlags, set12Db));
    menu->AddItem(new IPopupMenuItem("+/- 18 dB", freqViewMagRange == 18.f ? IPopupMenuItem::kChecked : IPopupMenuItem::kNoFlags, set18Db));
    menu->AddItem(new IPopupMenuItem("+/- 24 dB", freqViewMagRange == 24.f ? IPopupMenuItem::kChecked : IPopupMenuItem::kNoFlags, set24Db));
    return menu;
}

IPopupMenu* TinyQ::getSettingsMenu()
{
    std::function<void()> toggleGain = [this] () {
            showGain = ! showGain;
            updateShowGain();
        };

    std::function<void()> sortBands = [this] () {
        IMutexLock lock(this);
        BandSorter::sortBands(this);
        UpdateParameters();
        freqDisplay->SetDirty(false);
        freqDisplay->Redraw();
    };

    std::function<void()> showBgText = [this] () {
        showBackgroundText = ! showBackgroundText;
        freqDisplay->Redraw();
    };

    IPopupMenu* menu = new IPopupMenu();
    const juce::String version(juce::String("Version ") + VST3_VER_STR);
    menu->AddItem(new IPopupMenuItem(version.toRawUTF8(), IPopupMenuItem::kDisabled));
    menu->AddSeparator();
    menu->AddItem("Default Parameters", getDefaultsMenu());
    menu->AddItem("Size", getSizeMenu());
    menu->AddItem("Frequency View", getFreqViewMenu());
    menu->AddItem("Analyser", getAnalyserMenu());
    menu->AddSeparator();
    menu->AddItem(new IPopupMenuItem("Show Gain Instead of Size Toggle", showGain ? IPopupMenuItem::kChecked : IPopupMenuItem::kNoFlags, toggleGain));
    menu->AddItem(new IPopupMenuItem("Sort Peak Bands", IPopupMenuItem::kNoFlags, sortBands));
    menu->AddItem(new IPopupMenuItem("Show \"TinyQ\" Text in Frequency Display", showBackgroundText ? IPopupMenuItem::kChecked : IPopupMenuItem::kNoFlags, showBgText));
    
    return menu;
}

IPopupMenu* TinyQ::getAnalyserMenu()
{
    IPopupMenu* menu = new IPopupMenu();
    std::function<void(void)> toggleAnalyser = [this] () {
        bandAnalyser.setEnabled(! bandAnalyser.getEnabled());
    };

    std::function<void(void)> togglePreProc = [this] () {
        processAnalyserIn = ! processAnalyserIn;
    };

    IPopupMenu* magTop = new IPopupMenu();
    for (int i=0; i<6; ++i)
    {
        const int mag = 20 - 10*i;
        std::function<void(void)> f = [this, mag] () {
            analyserMagTop = static_cast<float> (mag);
            updateAnalyserRange();
        };
        const int flag = std::abs(analyserMagTop - mag) < 1 ? IPopupMenuItem::kChecked : IPopupMenuItem::kNoFlags;
        magTop->AddItem(new IPopupMenuItem(juce::String(juce::String(mag) + " dB").toRawUTF8(), flag, f));
    }
    
    IPopupMenu* magRng = new IPopupMenu();
    for (int i=0; i<6; ++i)
    {
        const int rng = 40 + 10*i;
        std::function<void(void)> f = [this, rng] () {
            analyserMagRange = static_cast<float> (rng);
            updateAnalyserRange();
        };
        const int flag = std::abs(analyserMagRange - rng) < 1 ? IPopupMenuItem::kChecked : IPopupMenuItem::kNoFlags;
        magRng->AddItem(new IPopupMenuItem(juce::String(juce::String(rng) + " dB").toRawUTF8(), flag, f));
    }


    const bool en = bandAnalyser.getEnabled();    
    menu->AddItem(new IPopupMenuItem("Enable", en ? IPopupMenuItem::kChecked : IPopupMenuItem::kNoFlags, toggleAnalyser));
    menu->AddSeparator();
    menu->AddItem("Analyser Top", magTop);
    menu->AddItem("Analyser Range", magRng);

    menu->AddSeparator();
    menu->AddItem(new IPopupMenuItem("Analyse Input", processAnalyserIn ? IPopupMenuItem::kChecked : IPopupMenuItem::kNoFlags, togglePreProc));
    menu->AddItem(new IPopupMenuItem("Analyse Output", processAnalyserIn ? IPopupMenuItem::kNoFlags : IPopupMenuItem::kChecked, togglePreProc));

    return menu;
}

float TinyQ::getSizeFactor() const
{
    return layout.getSizeFactor();
}

bool TinyQ::getShowBackgroundText() const
{
    return showBackgroundText;
}

int TinyQ::getParamIndexForName(const juce::String& name)
{
    const int numParams = NParams();
    
    for (int i=0; i<numParams; ++i)
        if (name.equalsIgnoreCase(GetParam(i)->GetNameForHost()))
            return i;

    return -1;
}

juce::File TinyQ::getSettingsFile() const
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("lkjb").getChildFile("tinyqsettings.xml");
}

juce::XmlElement* TinyQ::getSettingsXml() const
{
    juce::XmlElement* xml = juce::XmlDocument::parse(getSettingsFile());

    if (xml == nullptr)
        xml = new juce::XmlElement("TINYQ");

    return xml;
}

void TinyQ::setSettingsXml(juce::XmlElement* xml)
{
    if (xml != nullptr)
    {
        getSettingsFile().getParentDirectory().createDirectory();
        xml->writeToFile(getSettingsFile(), "");
    }
}

void TinyQ::updateShowGain()
{
    if (resizeButton != nullptr)
    {
        resizeButton->Hide(showGain);
        resizeButton->SetDirty(false);
    }

    if (gainControl != nullptr)
    {
        gainControl->Hide(! showGain);
        gainControl->SetDirty(false);
    }
}

void TinyQ::updateAnalyserRange()
{
    if (bandAnalyserView != nullptr)
        bandAnalyserView->setMagRange(juce::Range<float> (analyserMagTop - analyserMagRange, analyserMagTop));
}

void TinyQ::processAnalyser(const double* inL, const double* inR, int numSamples)
{
    if (inL == inR)
    {
        bandAnalyser.processBlock(inL, numSamples);
        return;
    }

    const int intBlockSize = 64;
    double tmp[intBlockSize];

    while (numSamples > 0)
    {
        const int curNumSamples = juce::jmin(numSamples, intBlockSize);
        for (int i=0; i<curNumSamples; ++i)
            tmp[i] = 0.5f * (inL[i] + inR[i]);

        bandAnalyser.processBlock(tmp, intBlockSize);
        numSamples -= curNumSamples;
        inL += curNumSamples;
        inR += curNumSamples;
    }
}

void TinyQ::SetBlockSize(int blockSize)
{
    IPlug::SetBlockSize(blockSize);
    bandAnalyser.setBlockSize(blockSize);
}

void TinyQ::SetSampleRate(double sampleRate)
{
    IPlug::SetSampleRate(sampleRate);
    const int numFilters = 32;
    const double minFreq = 20.f;
    const double maxFreq = 20e3f;
    const int order = 4;
    bandAnalyser.setup(numFilters, minFreq, maxFreq, sampleRate, order);

    if (bandAnalyserView != nullptr)
        bandAnalyserView->updateNumCurvePoints();
}

void TinyQ::Reset()
{
    TRACE;
    IMutexLock lock(this);
}

void TinyQ::OnWindowResize()
{
    CreateControls(GetGUI());
}