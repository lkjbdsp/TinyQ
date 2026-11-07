#pragma once

#include <memory>
#include <vector>
#include "FilterLayout.h"

// from https://github.com/vinniefalco/DSPFilters with modification

namespace BandPassFilterCreator
{

struct BiquadParams
{
    void setOnePole(complex_t pole, complex_t zero);
    void setTwoPole(complex_t pole1, complex_t zero1, complex_t pole2, complex_t zero2);
    void setPoleZeroPair(const PoleZeroPair& pair);
    void applyScale(double scale);

    double a1=0, a2=0;
    double b0=1, b1=0, b2=0;
};

class BasicBiquadImpl
{
public:

    void clear()
    {
        x1 = 0; x2 = 0; y1 = 0; y2 = 0;
    }

    void setParams(const BiquadParams& newParams)
    {
        params = newParams;
    }

    void processBlock(float* out, const float* in, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const double x0 = in[i];
            const double y0 = x0 * params.b0 + x1 * params.b1 + x2 * params.b2 - y1 * params.a1 - y2 * params.a2;
            out[i] = static_cast<float> (y0);
            y2 = y1; y1 = y0;
            x2 = x1; x1 = x0;
        }
    }

private:

    double x1 = 0, x2 = 0, y1 = 0, y2 = 0;
    BiquadParams params;
};

typedef std::vector<BiquadParams> BiquadCascade;

BiquadCascade createBandPass(int order, double freq, double bw, double sampleRate);


}