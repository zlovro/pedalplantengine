//
// Created by lovro on 03/02/2026.
// Copyright (c) 2026 lovro. All rights reserved.
//

#ifndef FX_AMPLITUDE_HPP
#define FX_AMPLITUDE_HPP

#include <algorithm>
#include <fx/fx.hpp>

#include "main.hpp"

namespace Fx
{
    // gain
    class FxDescriptorGain : public FxDescriptor
    {
        public:
        typedef struct FxParamsGain
        {
            levelUnit displayUnit;
            float     gain;

            explicit FxParamsGain(float pGain, levelUnit pDisplayUnit)
            {
                gain        = pGain;
                displayUnit = pDisplayUnit;
            }
        } FxParamsGain;

        explicit FxDescriptorGain(FxInstanceId pInput, float pGain = 1.0F, levelUnit pLvlUnit = LEVEL_UNIT_LINEAR) : FxDescriptorGain(pGain, pLvlUnit)
        {
            inputs = std::vector{pInput};
        }

        explicit FxDescriptorGain(float pGain = 1.0F, levelUnit pLvlUnit = LEVEL_UNIT_LINEAR) : FxDescriptor()
        {
            params = new FxParamsGain(pGain, pLvlUnit);

            processor = [](const std::vector<float *> &pInputs, float *pOut, int pBufSz, int pSampleRate, const void *pParams, int pCh)
            {
                auto pIn = pInputs[0];

                auto params = (FxParamsGain *) pParams;
                auto gain   = params->gain;

                for (int i = 0; i < pBufSz; ++i)
                {
                    pOut[i] = std::clamp(pIn[i] * gain, -FX_MAX_AUDIO_VALUE, FX_MAX_AUDIO_VALUE);
                }
            };
        }

        int getExpectedInputCount() override
        {
            return 1;
        }

        FxParamsGain *getParams()
        {
            return (FxParamsGain *) params;
        }

        FxId getId() override
        {
            return FX_ID_GAIN;
        }

        const char *getName() override
        {
            return "Gain";
        }

        ~FxDescriptorGain() override
        {
            delete getParams();
        }
    };
}
#endif //FX_AMPLITUDE_HPP
