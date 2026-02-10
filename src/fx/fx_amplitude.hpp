//
// Created by lovro on 03/02/2026.
// Copyright (c) 2026 lovro. All rights reserved.
//

#ifndef FX_AMPLITUDE_HPP
#define FX_AMPLITUDE_HPP

#include <algorithm>
#include <fx/fx.hpp>

namespace Fx
{
    // gain
    typedef struct
    {
        float gain;
    } FxParamsGain;

    class FxDescriptorGain : public FxDescriptor
    {
        public:
        explicit FxDescriptorGain(FxInstanceId pInput, float pGain = 1.0F) : FxDescriptor(pInput)
        {
            auto par  = new FxParamsGain();
            par->gain = pGain;

            params = par;

            processor = [](const std::vector<float *> &pInputs, float *pOut, int pBufSz, int pSampleRate, const void *pParams, int pCh)
            {
                auto pIn = pInputs[0];

                auto params = (FxParamsGain *) pParams;
                auto gain   = params->gain;

                for (int i = 0; i < pBufSz; ++i)
                {
                    pOut[i] = std::clamp(pIn[i] * gain, -1.0F, 1.0F);
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
