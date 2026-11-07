#pragma once

#include "IControl.h"
#include "JuceIncludes.h"

class IEditorControl : public IControl
{
public:
    IEditorControl(IPlugBase* pPlug, IRECT pR, int paramIdx, IBitmap* fontImage, IChannelBlend blendMethod = IChannelBlend::kBlendNone)
        : IControl(pPlug, pR, paramIdx, blendMethod), font(*fontImage)
    {
        mDisablePrompt = false;

        const int h = pR.H();
        const int size = h < 11 ? 12 : 13;

        mText = IText(size, &mText.mColor, mText.mFont, mText.mStyle, mText.mAlign, mText.mOrientation, mText.mQuality, &mText.mTextEntryBGColor, &mText.mTextEntryFGColor);
    }

    void OnMouseDown(int, int, IMouseMod*) override
    {
        PromptUserInput();
    }
    
    void OnMouseWheel(int , int , IMouseMod* pMod, int d) override
    {
        if (IParam* p = GetParam())
        {
            const double precFactor = p->GetPrecision() > 0 ? std::pow(10., p->GetPrecision()) : 1.;
            const double delta = 1. / ((p->GetMax() - p->GetMin()) * precFactor);
            mValue += d * delta * (pMod->S ? 1. : 5.);
            SetDirty(true);
        }
    }

    bool Draw(IGraphics* pGraphics) override
    {
        const juce::String text(GetDisplayText());
        const int numChars = text.getNumBytesAsUTF8();
        const char* textBytes = text.toRawUTF8();
        const int chW = font.W;
        const int chH = font.H / font.N;
        const int x0 = mRECT.L + (mRECT.W() - numChars*chW)/2;
        const int y0 = mRECT.T + (mRECT.H() - chH) / 2;

        for (int i=0; i<numChars; ++i)
        {
            const int idx = GetIndexForChar(textBytes[i]);
            int x = x0 + i * chW;
            IRECT rect(x, y0, &font);

            if (idx >= 0)
                pGraphics->DrawBitmap(&font, &rect, idx + 1);
        }

        return true;
    }

    void SetIsFreq()
    {
        std::function<double(const char*)> convertTextToValue = [] (const char* txt) {
            juce::String s(txt);
            s = s.replace(",", ".").replace("k", ".");

            if (s.toLowerCase().contains("."))
                return s.getDoubleValue() * 1000;

            return s.getDoubleValue();
        };

        GetParam()->setConverTextToValueFunction(convertTextToValue);
        isFreq = true;
    }

    void setNumDecimals(int decimals)
    {
        numDecimals = decimals;
    }

    bool IsDirty()
    {
        if (IParam* p = GetParam())
            SetValueFromPlug(p->GetNormalized());

        return IControl::IsDirty();
    }

    void SetDirty(bool pushParamToPlug) override
    {
        IControl::SetDirty(pushParamToPlug);

        if (pushParamToPlug && paramChanged != nullptr)
            paramChanged();
    }

    void onParamChanged(std::function<void()> f)
    {
        paramChanged = f;
    }

private:

    juce::String GetDisplayText()
    {
        const double value = GetParam()->Value();
        juce::String text;

        if (isFreq)
            text = value < 1000 ? juce::String(static_cast<int> (value + 0.5)) : juce::String(value * 0.001, 2) + "k";
        else
            text = numDecimals == 0 ? juce::String(static_cast<int> (value + 0.5)) : juce::String(value, numDecimals);

        return text.retainCharacters("0123456789k.- ").trim().toLowerCase();
    }

    int GetIndexForChar(char c) const
    {
        if (c >= '0' && c <= '9')
            return c - '0';

        if (c == 'k')
            return 10;
        if (c == '-')
            return 11;
        if (c == '.')
            return 12;

        return -1;
    }
    
    std::function<void()> paramChanged = nullptr;

    IBitmap font;
    bool isFreq = false;
    int numDecimals = 1;
};
