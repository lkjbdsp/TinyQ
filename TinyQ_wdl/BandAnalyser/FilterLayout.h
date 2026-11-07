#pragma once

#include <complex>
#include "../JuceIncludes.h"

// from https://github.com/vinniefalco/DSPFilters with modification

typedef std::complex<double> complex_t;
typedef std::pair<complex_t, complex_t> complex_pair_t;

namespace LayoutHelper
{

static const double doublePi_2 = 1.5707963267948966192313216916397514420986;
static const double doublePi = 3.1415926535897932384626433832795028841971;

template <typename Ty>
inline bool is_nan (Ty v)
{
    return !(v == v);
}

template <>
inline bool is_nan<complex_t> (complex_t v)
{
    return is_nan (v.real()) || is_nan (v.imag());
}

inline const complex_t infinity()
{
    return complex_t (std::numeric_limits<double>::infinity());
}

template <typename Ty, typename To>
inline std::complex<Ty> addmul(const std::complex<Ty>& c,
    Ty v,
    const std::complex<To>& c1)
{
    return std::complex <Ty>(
        c.real() + v * c1.real(), c.imag() + v * c1.imag());
}
}

struct ComplexPair : complex_pair_t
{
    ComplexPair ()
    {
    }

    explicit ComplexPair (const complex_t& c1)
        : complex_pair_t (c1, 0.)
    {
        jassert (isReal());
    }

    ComplexPair (const complex_t& c1,
        const complex_t& c2)
        : complex_pair_t (c1, c2)
    {
    }

    bool isConjugate () const
    {
        return second == std::conj (first);
    }

    bool isReal () const
    {
        return first.imag() == 0 && second.imag() == 0;
    }

    bool isMatchedPair () const
    {
        if (first.imag() != 0)
            return second == std::conj (first);
        else
            return second.imag () == 0 &&
            second.real () != 0 &&
            first.real () != 0;
    }

    bool is_nan () const
    {
        return LayoutHelper::is_nan (first) || LayoutHelper::is_nan (second);
    }
};


struct PoleZeroPair
{
    ComplexPair poles;
    ComplexPair zeros;

    PoleZeroPair () { }

    PoleZeroPair (const complex_t& p, const complex_t& z)
        : poles (p), zeros (z)
    {
    }

    PoleZeroPair (const complex_t& p1, const complex_t& z1,
        const complex_t& p2, const complex_t& z2)
        : poles (p1, p2)
        , zeros (z1, z2)
    {
    }

    bool isSinglePole () const
    {
        return poles.second == 0. && zeros.second == 0.;
    }

    bool is_nan () const
    {
        return poles.is_nan() || zeros.is_nan();
    }
};



class LayoutBase
{
public:
    LayoutBase ()
        : m_numPoles (0)
    {
    }

    void reset ()
    {
        m_numPoles = 0;
        m_pair.clear();
    }

    int getNumPoles () const
    {
        return m_numPoles;
    }

    void add (const complex_t& pole, const complex_t& zero)
    {
        jassert (!(m_numPoles&1)); // single comes last
        jassert (!LayoutHelper::is_nan (pole));
        m_pair.push_back(PoleZeroPair(pole, zero));
        ++m_numPoles;
    }

    void addPoleZeroConjugatePairs (const complex_t pole,
        const complex_t zero)
    {
        jassert (!(m_numPoles&1));
        jassert (!LayoutHelper::is_nan (pole));
        m_pair.push_back(PoleZeroPair(pole, zero, std::conj(pole), std::conj(zero)));
        m_numPoles += 2;
    }

    void add (const ComplexPair& poles, const ComplexPair& zeros)
    {
        jassert (!(m_numPoles&1));
        jassert (poles.isMatchedPair ());
        jassert (zeros.isMatchedPair ());
        m_pair.push_back(PoleZeroPair(poles.first, zeros.first, poles.second, zeros.second));
        m_numPoles += 2;
    }

    const PoleZeroPair& getPair (int pairIndex) const
    {
        jassert(pairIndex >= 0 && pairIndex < (m_numPoles + 1) / 2);
        jassert(pairIndex >= 0 && pairIndex < static_cast<int> (m_pair.size()));
        return m_pair[pairIndex];
    }

    const PoleZeroPair& operator[] (int pairIndex) const
    {
        return getPair (pairIndex);
    }

    double getNormalW () const
    {
        return m_normalW;
    }

    double getNormalGain () const
    {
        return m_normalGain;
    }

    void setNormal (double w, double g)
    {
        m_normalW = w;
        m_normalGain = g;
    }

private:
    int m_numPoles;
    std::vector<PoleZeroPair> m_pair;
    double m_normalW = 0.;
    double m_normalGain = 0.;
};

class AnalogLowPass : public LayoutBase
{
public:
    AnalogLowPass ()
    : m_numPoles (-1)
    {
        setNormal (0, 1);
    }

    void design (const int numPoles)
    {
        if (m_numPoles != numPoles)
        {
            m_numPoles = numPoles;

            reset ();

            const double n2 = 2 * numPoles;
            const int pairs = numPoles / 2;
            for (int i = 0; i < pairs; ++i)
            {
                complex_t c = std::polar (1., LayoutHelper::doublePi_2 + (2 * i + 1) * LayoutHelper::doublePi / n2);
                addPoleZeroConjugatePairs (c, LayoutHelper::infinity());
            }

            if (numPoles & 1)
                add (-1, LayoutHelper::infinity());
        }
    }
private:
    int m_numPoles;
};