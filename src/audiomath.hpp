//
// Created by lovro on 14/02/2026.
// Copyright (c) 2026 lovro. All rights reserved.
//

#ifndef AUDIOMATH_HPP
#define AUDIOMATH_HPP
#include <cmath>

namespace AudioMath
{
    // starts off flat, ends off steep
    float linearToLogFlatStart(float pA, float pB, float pY);

    // starts off steep, ends off flat
    float linearToLogFlatEnd(float pA, float pB, float pY);

    float easeInOutCubic(float pX);
    float linearToEasedInOutCubic(float pA, float pB, float pY);
}

#endif //AUDIOMATH_HPP
