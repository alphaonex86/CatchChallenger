// Copyright (C) 2015 Rémi Bèges
// This file is part of ZNoise - a C++ noise library
// For conditions of distribution and use, see LICENSE file

#ifndef HYBRIDMULTIFRACTAL_HPP
#define HYBRIDMULTIFRACTAL_HPP

#include "MixerBase.hpp"

class HybridMultiFractal : public MixerBase
{
    public:
        HybridMultiFractal(const NoiseBase & source);
        HybridMultiFractal(const HybridMultiFractal&) = delete;
        ~HybridMultiFractal() = default;

        float Get(std::initializer_list<float> coordinates, float scale) const;

        HybridMultiFractal & operator=(const HybridMultiFractal&) = delete;

    protected:
    private:
        const NoiseBase & m_source;
        float m_value;
        float m_remainder;
        float m_offset;
        float m_weight;
        float m_signal;
};

#endif // HYBRIDMULTIFRACTAL_HPP
