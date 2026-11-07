#pragma once

#include "IControl.h"

class IKnobPollingControl : public IKnobMultiControl
{
public:
    IKnobPollingControl(IPlugBase* pPlug, int x, int y, int paramIdx, IBitmap* pBitmap,
        EDirection direction = kVertical, double gearing = DEFAULT_GEARING)
        : IKnobMultiControl(pPlug, x, y, paramIdx, pBitmap, direction, gearing)
    {
    }

    bool Draw(IGraphics* pGraphics) override
    {
        return IKnobMultiControl::Draw(pGraphics);
    }

    bool IsDirty() override
    {
        if (IParam* p = GetParam())
            SetValueFromPlug(p->GetNormalized());

        return IControl::IsDirty();
    }

    void OnMouseWheel(int x, int y, IMouseMod* pMod, int d) override
    {
        if (steppedMouseWheelMul != 0)
        {
            mValue += d * steppedMouseWheelMul;
            SetDirty(true);
        }
        else
        {
            IKnobMultiControl::OnMouseWheel(x, y, pMod, d);
        }
    }
    
    void OnMouseDown(int x, int y, IMouseMod* pMod) override
    {
        mouseDownX = x;
        mouseDownY = y;
        IKnobMultiControl::OnMouseDown(x, y, pMod);
    }

    void OnMouseDrag(int x, int y, int dX, int dY, IMouseMod* pMod) override
    {
        if (steppedMouseWheelMul != 0)
        {
            dY = mouseDownY - y;
            const int thresh = static_cast<int> (4 * mGearing);

            if (std::abs(dY) > std::abs(thresh))
            {
                mValue += std::abs(steppedMouseWheelMul) * (dY / thresh);
                mouseDownY = y;
                SetDirty(true);
            }
        }
        else
        {
            IKnobMultiControl::OnMouseDrag(x, y, dX, dY, pMod);
        }
    }

    virtual void setSteppedMouseWheel(bool enable)
    {
        IParam* param = GetParam();
        steppedMouseWheelMul = enable && param != nullptr && param->GetMin() == 0 && param->GetMax() > 1 ? 1 / (param->GetMax() - 1) : 0;    
    }

    void SetDirty(bool pushParamToPlug) override
    {
        IKnobMultiControl::SetDirty(pushParamToPlug);

        if (pushParamToPlug && paramChanged != nullptr)
            paramChanged();
    }

    void onParamChanged(std::function<void()> f)
    {
        paramChanged = f;
    }

protected:

    double steppedMouseWheelMul = 0;
    std::function<void()> paramChanged = nullptr;
    int mouseDownX = 0;
    int mouseDownY = 0;
};

class IKnobMultiReversedControl : public IKnobPollingControl
{
public:

    IKnobMultiReversedControl(IPlugBase* pPlug, int x, int y, int paramIdx, IBitmap* pBitmap,
                              EDirection direction = kVertical, double gearing = -DEFAULT_GEARING)
        : IKnobPollingControl(pPlug, x, y, paramIdx, pBitmap, direction, gearing)
    {
    }

    bool Draw(IGraphics* pGraphics) override
    {
        int i = 1 + int(0.5 + (1. - mValue) * (double) (mBitmap.N - 1));
        i = BOUNDED(i, 1, mBitmap.N);
        return pGraphics->DrawBitmap(&mBitmap, &mRECT, i, &mBlend);
    }
    
    void setSteppedMouseWheel(bool enable) override
    {
        IKnobPollingControl::setSteppedMouseWheel(enable);
        steppedMouseWheelMul = -steppedMouseWheelMul;
    }
};

class ISwitchReversedControl : public ISwitchControl
{
public:
    ISwitchReversedControl(IPlugBase* pPlug, int x, int y, int paramIdx, IBitmap* pBitmap, 
                           IChannelBlend::EBlendMethod blendMethod = IChannelBlend::kBlendNone)
        : ISwitchControl(pPlug, x, y, paramIdx, pBitmap, blendMethod)
    {
    }

    bool Draw(IGraphics* pGraphics) override
    {
        int i = 1;
        if (mBitmap.N > 1)
        {
            i = 1 + int(0.5 + (1 - mValue) * (double) (mBitmap.N - 1));
            i = BOUNDED(i, 1, mBitmap.N);
        }

        return pGraphics->DrawBitmap(&mBitmap, &mRECT, i, &mBlend);   
    }
};