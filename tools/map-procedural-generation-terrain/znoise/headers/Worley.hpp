// Copyright (C) 2015 Rémi Bèges
// This file is part of ZNoise - a C++ noise library
// For conditions of distribution and use, see LICENSE file

#ifndef WORLEY_HPP
#define WORLEY_HPP

#include "NoiseBase.hpp"
#include "Enums.hpp"
#include <initializer_list>
#include <map>

class Worley : public NoiseBase
{
    private:
      struct vec2
      {
          float x;
          float y;
      };
    public:
      Worley();
      Worley(unsigned int seed);
      ~Worley() = default;

      void Set(WorleyFunction func);

      float Get(std::initializer_list<float> coordinates, float scale) const;

    protected:
      float _2D(std::initializer_list<float> coordinates, float scale) const;
      float _3D(std::initializer_list<float> coordinates, float scale) const;
      float _4D(std::initializer_list<float> coordinates, float scale) const;
      void _SquareTest(int xi, int yi, float x, float y, std::map<float,vec2> & featurePoints) const;

    private:
      const float scales[4];
      WorleyFunction function;
};

#endif // WORLEY_HPP
