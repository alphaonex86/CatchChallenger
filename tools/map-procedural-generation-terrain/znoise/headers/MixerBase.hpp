// Copyright (C) 2015 Rémi Bèges
// This file is part of ZNoise - a C++ noise library
// For conditions of distribution and use, see LICENSE file

#ifndef MIXERBASE_HPP
#define MIXERBASE_HPP

#include "NoiseBase.hpp"
#include <array>

class MixerBase
{
    public:
        MixerBase();
        ~MixerBase() = default;

        float GetHurstParameter() const;
        float GetLacunarity() const;
        float GetOctaveNumber() const;
        void SetParameters(float hurst, float lacunarity, float octaves);

    protected:
        void _recompute();
        float m_lacunarity;
        float m_hurst;
        float m_octaves;
        std::vector<float> m_exponent_array;
        float m_sum;
};

#endif // MIXERBASE_HPP
