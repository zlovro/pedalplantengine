//
// Created by lovro on 02/02/2026.
// Copyright (c) 2026 lovro. All rights reserved.
//

#ifndef FX_XPASS_HPP
#define FX_XPASS_HPP

#include <fx/fx.hpp>

namespace Fx
{
    // 1st order lowpass

    typedef struct FxParamsLowPassFirstOrder
    {
        // dont serialize
        float lastSample;

        float cutoffFreq;

        explicit FxParamsLowPassFirstOrder(float pCutoffFreq)
        {
            cutoffFreq = pCutoffFreq;
            lastSample = 0;
        }
    } FxParamsLowPassFirstOrder;

    class FxDescriptorLowPassFilterFirstOrder : public FxDescriptor
    {
        public:
        explicit FxDescriptorLowPassFilterFirstOrder(FxInstanceId pInput, float pCutoffFreq = 22000.0F) : FxDescriptor(pInput)
        {
            params = new FxParamsLowPassFirstOrder(pCutoffFreq);

            processor = [](const std::vector<float*>& pInputs, float *pOut, int pBufSz, int pSampleRate, const void *pParams, int pCh)
            {
                auto pIn = pInputs[0];

                auto params = (FxParamsLowPassFirstOrder *) pParams;

                auto samplingPeriod = 1.0F / (float) pSampleRate;
                auto b              = 2 * (float) M_PI * samplingPeriod * params->cutoffFreq;
                auto a              = b / (b + 1);

                pOut[0] = a * pIn[0] + (1 - a) * params->lastSample;

                for (int i = 1; i < pBufSz; ++i)
                {
                    pOut[i] = a * pIn[i] + (1 - a) * pOut[i - 1];
                }

                params->lastSample = pOut[pBufSz - 1];
            };
        }

        int getExpectedInputCount() override
        {
            return 1;
        }

        FxParamsLowPassFirstOrder *getParams()
        {
            return (FxParamsLowPassFirstOrder *) params;
        }

        FxId getId() override
        {
            return FX_ID_LOPASS_FIRST_ORDER;
        }

        const char *getName() override
        {
            return "Low pass filter (1st order)";
        }

        ~FxDescriptorLowPassFilterFirstOrder() override
        {
            delete getParams();
        }
    };

    // 1st order hipass
    typedef struct FxParamsHighPassFirstOrder
    {
        // dont serialize
        float lastSample;

        float cutoffFreq;

        explicit FxParamsHighPassFirstOrder(float pCutoff)
        {
            lastSample = 0;
            cutoffFreq = pCutoff;
        }
    } FxParamsHighPassFirstOrder;

    class FxDescriptorHighPassFilterFirstOrder : public FxDescriptor
    {
        public:
        explicit FxDescriptorHighPassFilterFirstOrder(FxInstanceId pInput, float pCutoffFreq = 22000.0F) : FxDescriptor(pInput)
        {
            params = new FxParamsHighPassFirstOrder(pCutoffFreq);

            processor = [](const std::vector<float*>& pInputs, float *pOut, int pBufSz, int pSampleRate, const void *pParams, int pCh)
            {
                auto pIn = pInputs[0];

                auto params = (FxParamsHighPassFirstOrder *) pParams;

                auto samplingPeriod = 1.0F / (float) pSampleRate;
                auto a              = 1.0F / (1 + 2 * (float) M_PI * samplingPeriod * params->cutoffFreq);

                pOut[0] = params->lastSample;

                for (int i = 1; i < pBufSz; ++i)
                {
                    pOut[i] = a * pOut[i - 1] + a * (pIn[i] - pIn[i - 1]);
                }

                params->lastSample = pOut[pBufSz - 1];
            };
        }

        int getExpectedInputCount() override
        {
            return 1;
        }

        FxParamsHighPassFirstOrder *getParams()
        {
            return (FxParamsHighPassFirstOrder *) params;
        }

        FxId getId() override
        {
            return FX_ID_HIPASS_FIRST_ORDER;
        }

        const char *getName() override
        {
            return "High pass filter (1st order)";
        }

        ~FxDescriptorHighPassFilterFirstOrder() override
        {
            delete getParams();
        }
    };
}

#endif //FX_XPASS_HPP
