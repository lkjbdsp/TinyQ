#include "BandPassFilterCreator.h"

// from https://github.com/vinniefalco/DSPFilters with modification
namespace BandPassFilterCreator
{

void BiquadParams::setOnePole(complex_t pole, complex_t zero)
{
    jassert(pole.imag() == 0);
    jassert(zero.imag() == 0);

    const double a0 = 1;
    a1 = -pole.real();
    a2 = 0;
    b0 = -zero.real();
    b1 = 1;
    b2 = 0;
}

void BiquadParams::setTwoPole(complex_t pole1, complex_t zero1, complex_t pole2, complex_t zero2)
{
    if (pole1.imag() != 0)
    {
        jassert(pole2 == std::conj(pole1));

        a1 = -2 * pole1.real();
        a2 = std::norm(pole1);
    }
    else
    {
        jassert(pole2.imag() == 0);

        a1 = -(pole1.real() + pole2.real());
        a2 = pole1.real() * pole2.real();
    }

    b0 = 1;

    if (zero1.imag() != 0)
    {
        jassert(zero2 == std::conj(zero1));

        b1 = -2 * zero1.real();
        b2 = std::norm(zero1);
    }
    else
    {
        jassert(zero2.imag() == 0);

        b1 = -(zero1.real() + zero2.real());
        b2 = zero1.real() * zero2.real();
    }
}

void BiquadParams::setPoleZeroPair(const PoleZeroPair& pair)
{
    if (pair.isSinglePole())
        setOnePole(pair.poles.first, pair.zeros.first);
    else
        setTwoPole(pair.poles.first, pair.zeros.first,
            pair.poles.second, pair.zeros.second);
}

void BiquadParams::applyScale(double scale)
{
    b0 *= scale;
    b1 *= scale;
    b2 *= scale;
}


class BandPassTransform
{

public:
    BandPassTransform(double fc, double fw, LayoutBase& digital, LayoutBase const& analog)
    {
        digital.reset();

        const double wCenter = 2 * LayoutHelper::doublePi * fc;
        const double wBw = 2 * LayoutHelper::doublePi * fw;

        const double wLow = (std::sqrt(wBw*wBw + 4 * wCenter * wCenter) - wBw) / 2.;

        const double ww = wBw;

        wc2 = wLow;
        wc = wc2 + ww;

        if (wc2 < 1e-8)
            wc2 = 1e-8;
        if (wc > LayoutHelper::doublePi * 0.999999f)
            wc = LayoutHelper::doublePi * 0.999999f;

        a = cos((wc + wc2) * 0.5) /
            cos((wc - wc2) * 0.5);
        b = 1 / tan((wc - wc2) * 0.5);
        a2 = a * a;
        b2 = b * b;
        ab = a * b;
        ab_2 = 2 * ab;

        const int numPoles = analog.getNumPoles();
        const int pairs = numPoles / 2;
        for (int i = 0; i < pairs; ++i)
        {
            const PoleZeroPair& pair = analog[i];
            ComplexPair p1 = transform(pair.poles.first);
            ComplexPair z1 = transform(pair.zeros.first);

            digital.addPoleZeroConjugatePairs(p1.first, z1.first);
            digital.addPoleZeroConjugatePairs(p1.second, z1.second);
        }

        if (numPoles & 1)
        {
            ComplexPair poles = transform(analog[pairs].poles.first);
            ComplexPair zeros = transform(analog[pairs].zeros.first);

            digital.add(poles, zeros);
        }

        double wn = analog.getNormalW();
        digital.setNormal(2 * atan(sqrt(tan((wc + wn)* 0.5) * tan((wc2 + wn)* 0.5))),
            analog.getNormalGain());
    }

private:

    ComplexPair transform(complex_t c)
    {
        if (c == LayoutHelper::infinity())
            return ComplexPair(-1, 1);

        c = (1. + c) / (1. - c); // bilinear

        complex_t v = 0;
        v = LayoutHelper::addmul(v, 4 * (b2 * (a2 - 1) + 1), c);
        v += 8 * (b2 * (a2 - 1) - 1);
        v *= c;
        v += 4 * (b2 * (a2 - 1) + 1);
        v = std::sqrt(v);

        complex_t u = -v;
        u = LayoutHelper::addmul(u, ab_2, c);
        u += ab_2;

        v = LayoutHelper::addmul(v, ab_2, c);
        v += ab_2;

        complex_t d = 0;
        d = LayoutHelper::addmul(d, 2 * (b - 1), c) + 2 * (1 + b);

        return ComplexPair(u / d, v / d);
    }

    double wc;
    double wc2;
    double a;
    double b;
    double a2;
    double b2;
    double ab;
    double ab_2;
};

complex_t response(const BiquadCascade& cascade, double normalizedFrequency)
{
    double w = 2 * LayoutHelper::doublePi * normalizedFrequency;
    const complex_t czn1 = std::polar(1., -w);
    const complex_t czn2 = std::polar(1., -2 * w);
    complex_t ch(1);
    complex_t cbot(1);

    for (auto b : cascade)
    {
        complex_t cb(1);

        complex_t ct(b.b0 / 1);
        ct = LayoutHelper::addmul(ct, b.b1 / 1, czn1);
        ct = LayoutHelper::addmul(ct, b.b2 / 1, czn2);
        cb = LayoutHelper::addmul(cb, b.a1 / 1, czn1);
        cb = LayoutHelper::addmul(cb, b.a2 / 1, czn2);

        ch *= ct;
        cbot *= cb;
    }

    return ch / cbot;
}

void applyScale(BiquadCascade& cascade, double scale)
{
    if (cascade.size() > 0)
        cascade[0].applyScale(scale);
}

BiquadCascade createCascade(const LayoutBase& layout)
{
    BiquadCascade ret;
    const int numPoles = layout.getNumPoles();
    const int numStages = (numPoles + 1) / 2;
    

    for (int i = 0; i < numStages; ++i)
    {
        BiquadParams stage;
        stage.setPoleZeroPair(layout[i]);
        ret.push_back(stage);
    }

    applyScale(ret, layout.getNormalGain() / std::abs(response(ret, layout.getNormalW() / (2 * LayoutHelper::doublePi))));
    return ret;
}

BiquadCascade createBandPass(int order, double freq, double bw, double sampleRate)
{
    AnalogLowPass analogLowPass;
    LayoutBase digitalBandPass;
    analogLowPass.design(order);

    BandPassTransform(freq / sampleRate, bw / sampleRate, digitalBandPass, analogLowPass);

    return createCascade(digitalBandPass);
}


}


