// Copyright (C) 2015 Rémi Bèges
// This file is part of ZNoise - a C++ noise library
// For conditions of distribution and use, see LICENSE file

#ifndef NOISEBASE_HPP
#define NOISEBASE_HPP

#include <random>

class NoiseBase
{
    public:
        NoiseBase(unsigned int seed = 0);
        ~NoiseBase() = default;

        virtual float Get(std::initializer_list<float> coordinates, float scale) const = 0;

        float GetScale();

        void SetSeed(unsigned int seed);

        void SetScale(float scale);

        void Shuffle();

        void Shuffle(unsigned int amount);

    protected:
        unsigned int perm[512];
        float m_scale;
    private:
        std::default_random_engine generator;

};

#endif // NOISEBASE_HPP
