// Copyright (C) 2015 Rémi Bèges
// This file is part of ZNoise - a C++ noise library
// For conditions of distribution and use, see LICENSE file

#ifndef SIMPLEX_HPP
#define SIMPLEX_HPP

#include "NoiseBase.hpp"
#include <initializer_list>

class Simplex : public NoiseBase
{
    public:
      Simplex();
      Simplex(unsigned int seed);
      ~Simplex() = default;

      float Get(std::initializer_list<float> coordinates, float scale) const;

    protected:
      float _2D(std::initializer_list<float> coordinates, float scale) const;
      float _3D(std::initializer_list<float> coordinates, float scale) const;
      float _4D(std::initializer_list<float> coordinates, float scale) const;

    private:
      const float gradient2[8][2];
      const float gradient3[16][3];
      const float gradient4[32][4];
      const float UnskewCoeff2D;
      const float SkewCoeff2D;
      const float UnskewCoeff3D;
      const float SkewCoeff3D;
      const float UnskewCoeff4D;
      const float SkewCoeff4D;
      const int lookupTable4D[64][4];
};

#endif // SIMPLEX_HPP
