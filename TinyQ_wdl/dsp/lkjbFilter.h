#pragma once

#include <vector>
#include <memory>
#include <complex>
#include <array>

namespace lkjb
{

    enum FilterType
    {
        filterTypePeak,
        filterTypeLoShelf,
        filterTypeHiShelf,
        filterTypeLoPass,
        filterTypeLoPass6,
        filterTypeHiPass,
        filterTypeHiPass6,
        filterTypeBandPass,
        filterTypeNotch,

        filterNumTypes
    };

    enum ButterworthType
    {
        bwLowPass,
        bwHighPass,

        bwNumTypes
    };

    struct Quad
    {
        double x0 = 1., x1 = 0., x2 = 0.;
        void normalizeX0()
        {
            x2 /= x0; x1 /= x0; x0 = 1.;
        }
        void normalizeX2()
        {
            x0 /= x2; x1 /= x2; x2 = 1.;
        }
        void multiply(double factor)
        {
            x0 *= factor; x1 *= factor; x2 *= factor;
        }

        double response(double normalizedFreq) const
        {
            auto j_pi = -std::complex<double>(0., 3.14159265358979323846);
            std::complex<double> z = std::exp(-2. * j_pi * normalizedFreq);
            return std::abs(x0 + x1 * z + x2 * z * z);
        }
    };

    struct ClassicBiquad
    {
        double b0 = 1., b1 = 0., b2 = 0.;
        double a0 = 1., a1 = 0., a2 = 0.;

        double response(double normalizedFreq) const
        {
            auto j = -std::complex<double>(0., 2* 3.14159265358979323846);
            std::complex<double> z = std::exp(-j * normalizedFreq);
            auto b = b0 + b1 * z + b2 * z * z;
            auto a = a0 + a1 * z + a2 * z * z;
            return std::abs(b / a);
        }

        double responseAnalog(double freq) const
        {
            auto j_pi = std::complex<double>(0., 2 * 3.14159265358979323846);
            std::complex<double> s = freq * j_pi;
            auto b = b0 + b1 * s + b2 * s * s;
            auto a = a0 + a1 * s + a2 * s * s;
            return std::abs(b / a);
        }

        void normalize()
        {
            b0 /= a0; b1 /= a0; b2 /= a0;
            a1 /= a0; a2 /= a0;
            a0 = 1.;
        }

        void setNumerator(const Quad& q)
        {
            b0 = q.x0; b1 = q.x1; b2 = q.x2;
        }

        void setDenominator(const Quad& q)
        {
            a0 = q.x0; a1 = q.x1; a2 = q.x2;
        }

        Quad getNumerator() const
        {
            return Quad{ b0, b1, b2 };
        }

        Quad getDenominator() const
        {
            return Quad{ a0, a1, a2 };
        }

        static ClassicBiquad createAnalogPeak(double freq, double Q, double gain)
        {
            ClassicBiquad bq;
            auto A = std::pow(10., 0.025 * gain);
            const auto w = 2 * 3.14159265358979323846 * freq;
            bq.b0 = 1; bq.b1 = A / (Q * w); bq.b2 = 1 / (w * w);
            bq.a0 = 1; bq.a1 = 1 / (w * A * Q); bq.a2 = 1 / (w * w);
            return bq;
        }

        static ClassicBiquad createAnalogLowShelf(double freq, double Q, double gain)
        {
            ClassicBiquad bq;
            auto A = std::pow(10., 0.025 * gain);
            const auto w = 2 * 3.14159265358979323846 * freq;
            bq.b0 = A*A; bq.b1 = A*std::sqrt(A) / (Q * w); bq.b2 = A / (w * w);
            bq.a0 = 1; bq.a1 = std::sqrt(A) / (w * Q); bq.a2 = A / (w * w);
            return bq;
        }
        
        static ClassicBiquad createAnalogHighShelf(double freq, double Q, double gain)
        {
            ClassicBiquad bq;
            auto A = std::pow(10., 0.025 * gain);
            const auto w = 2 * 3.14159265358979323846 * freq;
            bq.b0 = A; bq.b1 = A * std::sqrt(A) / (Q * w); bq.b2 = A * A / (w * w);
            bq.a0 = A; bq.a1 = std::sqrt(A) / (w * Q); bq.a2 = 1 / (w * w);
            return bq;
        }

        static ClassicBiquad createAnalogLowPass(double freq, double Q)
        {
            ClassicBiquad bq;
            const auto w = 2 * 3.14159265358979323846 * freq;
            bq.b0 = 1; bq.b1 = 0; bq.b2 = 0;
            bq.a0 = 1; bq.a1 = 1 / (w * Q); bq.a2 = 1 / (w * w);
            return bq;
        }

        static ClassicBiquad createAnalogLowPass6(double freq, double Q)
        {
            ClassicBiquad bq;
            const auto w = 2 * 3.14159265358979323846 * freq;
            bq.b0 = 1; bq.b1 = w; bq.b2 = 0;
            bq.a0 = 1; bq.a1 = 1 / (std::sqrt(0.5) * w * Q); bq.a2 = 1 / (w * w);
            return bq;
        }

        static ClassicBiquad createAnalogLowPass6(double freq)
        {
            ClassicBiquad bq;
            const auto w = 2 * 3.14159265358979323846 * freq;
            bq.b0 = 1; bq.b1 = 0; bq.b2 = 0;
            bq.a0 = 1; bq.a1 = 1 / w; bq.a2 = 0;
            return bq;
        }

        static ClassicBiquad createAnalogHighPass(double freq, double Q)
        {
            ClassicBiquad bq;
            const auto w = 2 * 3.14159265358979323846 * freq;
            bq.b0 = 0; bq.b1 = 0; bq.b2 = 1/(w*w);
            bq.a0 = 1; bq.a1 = 1 / (w * Q); bq.a2 = 1 / (w * w);
            return bq;
        }

        static ClassicBiquad createAnalogHighPass6(double freq, double Q)
        {
            ClassicBiquad bq;
            const auto w = 2 * 3.14159265358979323846 * freq;
            bq.b0 = 0; bq.b1 = 1 / w; bq.b2 = 1 / (w * w);
            bq.a0 = 1; bq.a1 = 1 / (std::sqrt(0.5) * w * Q); bq.a2 = 1 / (w * w);
            return bq;
        }

        static ClassicBiquad createAnalogHighPass6(double freq)
        {
            ClassicBiquad bq;
            const auto w = 2 * 3.14159265358979323846 * freq;
            bq.b0 = 0; bq.b1 = 1 / w; bq.b2 = 0;
            bq.a0 = 1; bq.a1 = 1 / w; bq.a2 = 0;
            return bq;
        }

        static ClassicBiquad createAnalogBandPass(double freq, double Q)
        {
            ClassicBiquad bq;
            const auto w = 2 * 3.14159265358979323846 * freq;
            bq.b0 = 0; bq.b1 = 1 / (w * Q); bq.b2 = 0;
            bq.a0 = 1; bq.a1 = 1 / (w * Q); bq.a2 = 1 / (w * w);
            return bq;
        }

        static ClassicBiquad createAnalogNotch(double freq, double Q)
        {
            ClassicBiquad bq;
            const auto w = 2 * 3.14159265358979323846 * freq;
            bq.b0 = 1; bq.b1 = 0; bq.b2 = 1 / (w * w);
            bq.a0 = 1; bq.a1 = 1 / (w * Q); bq.a2 = 1 / (w * w);
            return bq;
        }
    };

    template<class DataType = float>
    struct FilterParams
    {
        struct State
        {
            DataType z1 = 0.;
            DataType z2 = 0.;
            void clear()
            {
                z1 = 0.; z2 = 0.;
            }
        };

        void setFromBiquad(const ClassicBiquad& bq)
        {
            auto a0 = bq.a0;
            auto a1 = bq.a1 / a0;
            auto a2 = bq.a2 / a0;
            auto b0 = bq.b0 / a0;
            auto b1 = bq.b1 / a0;
            auto b2 = bq.b2 / a0;
            f = static_cast<DataType> (a2);
            e = static_cast<DataType> (std::sqrt(1 + a1 + a2));
            t0 = static_cast<DataType> (b0 / e);
            t1 = static_cast<DataType> (-b2 / e);
            t2 = static_cast<DataType> ((b2 + b1 + b0) / (e * e));
        }

        DataType process(State& s, DataType in) const
        {
            DataType tmp = s.z1 * f + (in - s.z2) * e;
            DataType out = tmp * t0 + s.z1 * t1 + s.z2 * t2;

            s.z2 = tmp * e + s.z2;
            s.z1 = tmp;
            return out;
        }

        DataType e = 1, f = 0, t0 = 1, t1 = 0, t2 = 0;        // Parameters
    };

    template<class DataType = float>
    struct MZTiParams
    {
        struct State
        {
            DataType x1 = 0, x2 = 0;
            void clear()
            {
                x1 = 0;
                x2 = 0;
            }
        };

        MZTiParams() : b0(static_cast<DataType>(1)), b1(static_cast<DataType>(0)), b2(static_cast<DataType>(0)) {}

        void setFilter(Quad q)
        {
            b0 = static_cast<DataType>(q.x0);
            b1 = static_cast<DataType>(q.x1);
            b2 = static_cast<DataType>(q.x2);
        }

        DataType process(State& s, DataType in) const
        {
            DataType out = b0 * in + b1 * s.x1 + b2 * s.x2;
            s.x2 = s.x1; s.x1 = in;
            return out;
        }

        DataType b0 = 1, b1 = 0, b2 = 0;  // MZTi Correction
    };

    template<typename DataType = float>
    struct MZTIFilterParams
    {
        struct State
        {
            typename FilterParams<DataType>::State bqState;
            typename MZTiParams<DataType>::State corState;
            void clear()
            {
                bqState.clear();
                corState.clear();
            }
        };

        DataType process(State& s, DataType in) const
        {
            auto x = procParams.process(s.bqState, in);
            return procCor.process(s.corState, x);
        }

        void setParams(const ClassicBiquad& bq, const Quad& cor)
        {
            params = bq;
            correction = cor;
            procParams.setFromBiquad(bq);
            procCor.setFilter(cor);
        }

        double response(double normalizedFreq) const
        {
            auto resMzt = params.response(normalizedFreq);
            auto resCor = correction.response(normalizedFreq);
            return resMzt * resCor;
        }

        ClassicBiquad params;
        Quad correction;

    private:
        FilterParams<DataType> procParams;
        MZTiParams<DataType> procCor;
    };

    template<class DataType = float>
    struct ButterWorthParams
    {
        struct State
        {
            State(int maxOrder) : maxNumBiquads((maxOrder + 1) / 2), state(maxNumBiquads)
            {
            }

            void clear()
            {
                for (auto& s : state)
                    s.clear();
            }

            const int maxNumBiquads;
            std::vector<typename MZTIFilterParams<DataType>::State> state;
        };

        ButterWorthParams(int maxOrder) : maxNumBiquads((maxOrder + 1) / 2), params(maxNumBiquads)
        {
        }

        void setFilter(int filterOrder, float freq, ButterworthType filterType, float sr)
        {
            order = filterOrder;
            auto type = filterType == bwLowPass ? filterTypeLoPass : filterTypeHiPass;
            auto type6 = filterType == bwLowPass ? filterTypeLoPass6 : filterTypeHiPass6;
            auto numBiquadsToProcess = std::min((order + 1) / 2, maxNumBiquads);

            for (int i = 0; i < order / 2; ++i)
            {
                auto Q = -0.5 / std::cos(3.14159265358979323846 * (2 * i + order + 1) / (2 * order));
                MatchedZTransform::setMZTiFilter(params[i], freq, Q, 0., type, sr);
            }

            if ((order & 1) == 1 && numBiquadsToProcess <= maxNumBiquads)
                MatchedZTransform::setMZTiFilter(params[numBiquadsToProcess - 1], freq, 0., 0., type6, sr);
        }

        DataType process(State& s, DataType in)
        {
            auto numBiquadsToProcess = std::min((order + 1) / 2, maxNumBiquads);

            for (auto i = 0; i < numBiquadsToProcess; i++)
                in = params[i].process(s.state[i], in);

            return in;
        }

        double response(double normalizedFreq) const
        {
            auto numBiquadsToProcess = std::min((order + 1) / 2, maxNumBiquads);
            double res = 1.;

            for (auto i = 0; i < numBiquadsToProcess; i++)
                res *= params[i].response(normalizedFreq);

            return res;
        }

        const int maxNumBiquads;
        std::vector<MZTIFilterParams<DataType> > params;
        int order = 0;
    };

    namespace MatchedZTransform
    {
    struct ZeroPair
    {
        std::complex<double> pz0, pz1;
        int numZeros = 0;

        void fromQuad(const Quad& q)
        {
            numZeros = q.x2 == 0 ? (q.x1 == 0 ? 0 : 1) : 2;
            if (numZeros == 2)
            {
                std::complex<double> a = q.x2, b = q.x1, c = q.x0;
                pz0 = (-b + std::sqrt(b * b - 4. * a * c)) / (2. * a);
                pz1 = (-b - std::sqrt(b * b - 4. * a * c)) / (2. * a);
            }
            else if (numZeros == 1)
            {
                pz0 = q.x0 == 0 ? 0. : q.x0 / q.x1;
            }
        }

        Quad toQuad() const
        {
            Quad q;
            if (numZeros == 2)
            {
                auto x1 = pz0 + pz1;
                auto x2 = pz0 * pz1;
                q.x0 = 1.;
                q.x1 = - x1.real();
                q.x2 = x2.real();
            }
            else if (numZeros == 1)
            {
                q.x2 = 0;
                q.x1 = - 1. / pz0.real();
                q.x0 = 1.;
            }
            else
            {
                q.x2 = 0;
                q.x1 = 0;
                q.x0 = 1.;
            }
            return q;
        }

        ZeroPair transformS2Z(double sampleRate) const
        {
            ZeroPair zp;

            zp.pz0 = std::exp(pz0 / sampleRate);
            zp.pz1 = std::exp(pz1 / sampleRate);
            zp.numZeros = numZeros;
            return zp;
        }
    };

    inline ClassicBiquad transformFromAnalog(const ClassicBiquad& bq, double matchFreq, double sampleRate)
    {
        ZeroPair pAn, zAn;
        zAn.fromQuad(bq.getNumerator());
        pAn.fromQuad(bq.getDenominator());
        auto qa = zAn.toQuad();
        auto qa2 = pAn.toQuad();
        auto zDig = zAn.transformS2Z(sampleRate);
        auto pDig = pAn.transformS2Z(sampleRate);
        ClassicBiquad dig;
        dig.setNumerator(zDig.toQuad());
        dig.setDenominator(pDig.toQuad());

        auto magAn = bq.responseAnalog(matchFreq);
        auto magDig = dig.response(matchFreq / sampleRate);
        auto factor = magAn / magDig;
        auto bDig = dig.getNumerator();
        bDig.multiply(factor);
        dig.setNumerator(bDig);

        return dig;
    }

    inline Quad getCorrection(const ClassicBiquad& analog, const ClassicBiquad& digital, double sampleRate)
    {
        Quad q;
        std::array<double, 3> f = { 0.01, sampleRate / 6., sampleRate / 3. };
        std::array<double, 3> h = { 0., 0., 0. };

        for (int i = 0; i < 3; ++i)
        {
            const auto hDig = digital.response(f[i] / sampleRate);
            const auto hAn = analog.responseAnalog(f[i]);
            h[i] = hAn / hDig;
        }

        q.x1 = static_cast<float> (0.5 * (h[0] - std::sqrt(h[0] * h[0] - 2 * h[1] * h[1] + 2 * h[2] * h[2])));
        q.x2 = static_cast<float> ((3 * (h[0] - q.x1) - std::sqrt(-3 * h[0] * h[0] + 12 * h[1] * h[1] - 6 * h[0] * q.x1 - 3 * q.x1 * q.x1)) / 6);
        q.x0 = static_cast<float> (h[0] - q.x1 - q.x2);

        return q;
    }

    inline ClassicBiquad createMZTiFilter(double freq, double Q, double gain, FilterType type, double sampleRate, ClassicBiquad* bqp = nullptr)
    {
        ClassicBiquad params;
        ClassicBiquad bq;

        if (type == filterTypePeak)
        {
            bq = ClassicBiquad::createAnalogPeak(freq, Q, gain);
            params = transformFromAnalog(bq, freq, sampleRate);
        }
        else if (type == filterTypeLoShelf)
        {
            bq = ClassicBiquad::createAnalogLowShelf(freq, Q, gain);
            params = transformFromAnalog(bq, 0.5*sampleRate, sampleRate);
        }
        else if (type == filterTypeHiShelf)
        {
            bq = ClassicBiquad::createAnalogHighShelf(freq, Q, gain);
            params = transformFromAnalog(bq, 0, sampleRate);
        }
        else if (type == filterTypeLoPass)
        {
            bq = ClassicBiquad::createAnalogLowPass(freq, Q);
            params = transformFromAnalog(bq, 0, sampleRate);
        }
        else if (type == filterTypeLoPass6)
        {
            bq =  Q > 0 ? ClassicBiquad::createAnalogLowPass6(freq, Q) : ClassicBiquad::createAnalogLowPass6(freq);
            params = transformFromAnalog(bq, 0, sampleRate);
        }
        else if (type == filterTypeHiPass)
        {
            bq = ClassicBiquad::createAnalogHighPass(freq, Q);
            params = transformFromAnalog(bq, 0.5 * sampleRate, sampleRate);
        }
        else if (type == filterTypeHiPass6)
        {
            bq = Q > 0 ? ClassicBiquad::createAnalogHighPass6(freq, Q) : ClassicBiquad::createAnalogHighPass6(freq);;
            params = transformFromAnalog(bq, 0.5 * sampleRate, sampleRate);
        }
        else if (type == filterTypeBandPass)
        {
            bq = ClassicBiquad::createAnalogBandPass(freq, Q);
            params = transformFromAnalog(bq, freq, sampleRate);
        }
        else if (type == filterTypeNotch)
        {
            bq = ClassicBiquad::createAnalogNotch(freq, Q);
            params = transformFromAnalog(bq, 0, sampleRate);
        }

        if (bqp != nullptr)
            *bqp = bq;

        return params;
    }


    template<class DataType = float>
    inline void setMZTiFilter(MZTIFilterParams<DataType>& filter, double freq, double Q, double gain, FilterType type, double sampleRate)
    {
        ClassicBiquad analog;
        auto digital = createMZTiFilter(freq, Q, gain, type, sampleRate, &analog);
        auto correction = getCorrection(analog, digital, sampleRate);
        filter.setParams(digital, correction);
    }


    }

}
