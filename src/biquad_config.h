#pragma once

/*
    miniaudio has a set of useful functions for getting the biquad coefficients (ma_biquad_config) 
    for any one of its 2nd degree filters, however they are internal functions to miniaudio!

    the following definitions are copy-pasted from miniaudio.h to expose them, as well as simple
    #define replacements for even more internal miniaudio stuff
    
    lazy solution, but it works
    
    biquad_config also contains a Type enum and a corresponding type_to_string function
*/

#include "miniaudio.h"

#include <cassert>
#include <cmath>

#include <godot_cpp/variant/string.hpp>

#define MA_PI_D M_PI
#define MA_ASSERT assert
#define ma_sind std::sin
#define ma_cosd std::cos
#define ma_powd std::pow
#define ma_sqrtd std::sqrt

namespace rhythm::dsp::bqcfg
{
    enum struct Type : int32_t { biquad = 0, lpf2 = 1, hpf2 = 2, bpf2 = 3, notch2 = 4, peak2 = 5, loshelf2 = 6, hishelf2 = 7 };
    godot::String type_to_string(const Type type)
    {
        switch( type )
        {
            case Type::biquad: return "biquadratic filter";
            case Type::lpf2: return "low pass filter";
            case Type::hpf2: return "high pass filter";
            case Type::bpf2: return "band pass filter";
            case Type::notch2: return "notch filter";
            case Type::peak2: return "peak filter";
            case Type::loshelf2: return "low shelf filter";
            case Type::hishelf2: return "high shelf filter";
        }
    }

    ma_biquad_config lpf2(const ma_lpf2_config* pConfig)
    {
        ma_biquad_config bqConfig;
        double q;
        double w;
        double s;
        double c;
        double a;

        MA_ASSERT(pConfig != NULL);

        q = pConfig->q;
        w = 2 * MA_PI_D * pConfig->cutoffFrequency / pConfig->sampleRate;
        s = ma_sind(w);
        c = ma_cosd(w);
        a = s / (2*q);

        bqConfig.b0 = (1 - c) / 2;
        bqConfig.b1 =  1 - c;
        bqConfig.b2 = (1 - c) / 2;
        bqConfig.a0 =  1 + a;
        bqConfig.a1 = -2 * c;
        bqConfig.a2 =  1 - a;

        bqConfig.format   = pConfig->format;
        bqConfig.channels = pConfig->channels;

        return bqConfig;
    }

    ma_biquad_config hpf2(const ma_hpf2_config* pConfig)
    {
        ma_biquad_config bqConfig;
        double q;
        double w;
        double s;
        double c;
        double a;

        MA_ASSERT(pConfig != NULL);

        q = pConfig->q;
        w = 2 * MA_PI_D * pConfig->cutoffFrequency / pConfig->sampleRate;
        s = ma_sind(w);
        c = ma_cosd(w);
        a = s / (2*q);

        bqConfig.b0 =  (1 + c) / 2;
        bqConfig.b1 = -(1 + c);
        bqConfig.b2 =  (1 + c) / 2;
        bqConfig.a0 =   1 + a;
        bqConfig.a1 =  -2 * c;
        bqConfig.a2 =   1 - a;

        bqConfig.format   = pConfig->format;
        bqConfig.channels = pConfig->channels;

        return bqConfig;
    }

    ma_biquad_config bpf2(const ma_bpf2_config* pConfig)
    {
        ma_biquad_config bqConfig;
        double q;
        double w;
        double s;
        double c;
        double a;

        MA_ASSERT(pConfig != NULL);

        q = pConfig->q;
        w = 2 * MA_PI_D * pConfig->cutoffFrequency / pConfig->sampleRate;
        s = ma_sind(w);
        c = ma_cosd(w);
        a = s / (2*q);

        bqConfig.b0 =  q * a;
        bqConfig.b1 =  0;
        bqConfig.b2 = -q * a;
        bqConfig.a0 =  1 + a;
        bqConfig.a1 = -2 * c;
        bqConfig.a2 =  1 - a;

        bqConfig.format   = pConfig->format;
        bqConfig.channels = pConfig->channels;

        return bqConfig;
    }

    ma_biquad_config notch2(const ma_notch2_config* pConfig)
    {
        ma_biquad_config bqConfig;
        double q;
        double w;
        double s;
        double c;
        double a;

        MA_ASSERT(pConfig != NULL);

        q = pConfig->q;
        w = 2 * MA_PI_D * pConfig->frequency / pConfig->sampleRate;
        s = ma_sind(w);
        c = ma_cosd(w);
        a = s / (2*q);

        bqConfig.b0 =  1;
        bqConfig.b1 = -2 * c;
        bqConfig.b2 =  1;
        bqConfig.a0 =  1 + a;
        bqConfig.a1 = -2 * c;
        bqConfig.a2 =  1 - a;

        bqConfig.format   = pConfig->format;
        bqConfig.channels = pConfig->channels;

        return bqConfig;
    }

    ma_biquad_config peak2(const ma_peak2_config* pConfig)
    {
        ma_biquad_config bqConfig;
        double q;
        double w;
        double s;
        double c;
        double a;
        double A;

        MA_ASSERT(pConfig != NULL);

        q = pConfig->q;
        w = 2 * MA_PI_D * pConfig->frequency / pConfig->sampleRate;
        s = ma_sind(w);
        c = ma_cosd(w);
        a = s / (2*q);
        A = ma_powd(10, (pConfig->gainDB / 40));

        bqConfig.b0 =  1 + (a * A);
        bqConfig.b1 = -2 * c;
        bqConfig.b2 =  1 - (a * A);
        bqConfig.a0 =  1 + (a / A);
        bqConfig.a1 = -2 * c;
        bqConfig.a2 =  1 - (a / A);

        bqConfig.format   = pConfig->format;
        bqConfig.channels = pConfig->channels;

        return bqConfig;
    }

    ma_biquad_config loshelf2(const ma_loshelf2_config* pConfig)
    {
        ma_biquad_config bqConfig;
        double w;
        double s;
        double c;
        double A;
        double S;
        double a;
        double sqrtA;

        MA_ASSERT(pConfig != NULL);

        w = 2 * MA_PI_D * pConfig->frequency / pConfig->sampleRate;
        s = ma_sind(w);
        c = ma_cosd(w);
        A = ma_powd(10, (pConfig->gainDB / 40));
        S = pConfig->shelfSlope;
        a = s/2 * ma_sqrtd((A + 1/A) * (1/S - 1) + 2);
        sqrtA = 2*ma_sqrtd(A)*a;

        bqConfig.b0 =  A * ((A + 1) - (A - 1)*c + sqrtA);
        bqConfig.b1 =  2 * A * ((A - 1) - (A + 1)*c);
        bqConfig.b2 =  A * ((A + 1) - (A - 1)*c - sqrtA);
        bqConfig.a0 =  (A + 1) + (A - 1)*c + sqrtA;
        bqConfig.a1 = -2 * ((A - 1) + (A + 1)*c);
        bqConfig.a2 =  (A + 1) + (A - 1)*c - sqrtA;

        bqConfig.format   = pConfig->format;
        bqConfig.channels = pConfig->channels;

        return bqConfig;
    }

    ma_biquad_config hishelf2(const ma_hishelf2_config* pConfig)
    {
        ma_biquad_config bqConfig;
        double w;
        double s;
        double c;
        double A;
        double S;
        double a;
        double sqrtA;

        MA_ASSERT(pConfig != NULL);

        w = 2 * MA_PI_D * pConfig->frequency / pConfig->sampleRate;
        s = ma_sind(w);
        c = ma_cosd(w);
        A = ma_powd(10, (pConfig->gainDB / 40));
        S = pConfig->shelfSlope;
        a = s/2 * ma_sqrtd((A + 1/A) * (1/S - 1) + 2);
        sqrtA = 2*ma_sqrtd(A)*a;

        bqConfig.b0 =  A * ((A + 1) + (A - 1)*c + sqrtA);
        bqConfig.b1 = -2 * A * ((A - 1) + (A + 1)*c);
        bqConfig.b2 =  A * ((A + 1) + (A - 1)*c - sqrtA);
        bqConfig.a0 =  (A + 1) - (A - 1)*c + sqrtA;
        bqConfig.a1 =  2 * ((A - 1) - (A + 1)*c);
        bqConfig.a2 =  (A + 1) - (A - 1)*c - sqrtA;

        bqConfig.format   = pConfig->format;
        bqConfig.channels = pConfig->channels;

        return bqConfig;
    }
} // rhythm::dsp::bqcfg