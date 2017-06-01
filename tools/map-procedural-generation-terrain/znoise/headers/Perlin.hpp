// Copyright (C) 2015 Rémi Bèges
// This file is part of ZNoise - a C++ noise library
// For conditions of distribution and use, see LICENSE file

#ifndef PERLIN_HPP
#define PERLIN_HPP

#include "NoiseBase.hpp"
#include <initializer_list>

class Perlin : public NoiseBase
{
    public:
      Perlin();
      Perlin(unsigned int seed);
      ~Perlin() = default;

      float Get(std::initializer_list<float> coordinates, float scale) const;

    protected:
      float _2D(std::initializer_list<float> coordinates, float scale) const;
      float _3D(std::initializer_list<float> coordinates, float scale) const;
      float _4D(std::initializer_list<float> coordinates, float scale) const;

    private:
      const float gradient2[8][2];
      const float gradient3[16][3];
      const float gradient4[32][4];
};

#endif // PERLIN_HPP
