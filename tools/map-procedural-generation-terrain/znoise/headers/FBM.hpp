// Copyright (C) 2015 Rémi Bèges
// This file is part of ZNoise - a C++ noise library
// For conditions of distribution and use, see LICENSE file

#ifndef FBM_HPP
#define FBM_HPP

#include "MixerBase.hpp"
#include "Enums.hpp"

class FBM : public MixerBase
{
    public:
        FBM(const NoiseBase & source);
        FBM(const FBM&) = delete;
        ~FBM() = default;

        float Get(std::initializer_list<float> coordinates, float scale) const;

        FBM & operator=(const FBM&) = delete;

    protected:
    private:
        const NoiseBase & m_source;
};

#endif // FBM_HPP
