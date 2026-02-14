//
// Created by lovro on 14/02/2026.
// Copyright (c) 2026 lovro. All rights reserved.
//

#include "audiomath.hpp"

namespace AudioMath
{
    float linearToLogFlatStart(float pA, float pB, float pY)
    {
        auto base = pB + 1 - pA;
        return std::lerp(pA, pB, 1.0F - logf(pB - pY + 1) / logf(base));
    }

    float linearToLogFlatEnd(float pA, float pB, float pY)
    {
        auto base = pB + 1 - pA;
        return std::lerp(pA, pB, logf(pY + 1 - pA) / logf(base));
    }

    float easeInOutCubic(float pX)
    {
        return pX < 0.5F ? (4 * pX * pX * pX) : (1.0F - powf(-2 * pX + 2, 3) / 2);
    }

    float linearToEasedInOutCubic(float pA, float pB, float pY)
    {
        return std::lerp(pA, pB, easeInOutCubic((pY - pA) / (pB - pA)));
    }
}
