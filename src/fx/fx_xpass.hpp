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

    typedef struct
    {
        float cutoffFreq;
    } FxParamsLowPassFirstOrder;

    class FxDescriptorLowPassFilterFirstOrder : public FxDescriptor
    {
        public:
        explicit FxDescriptorLowPassFilterFirstOrder(FxInstanceId pInput, float pCutoffFreq = 22000.0F) : FxDescriptor(pInput)
        {
            auto par        = new FxParamsLowPassFirstOrder();
            par->cutoffFreq = pCutoffFreq;

            params = par;

            processor = [](std::vector<FxInstanceId> &pInputs, float *pOut, int pBufSize, int pSampleRate, void *pParams, void **pUsrData, int pCh, std::map<int, std::array<float *, 2> > *pOutputMap)
            {
                auto pIn = pOutputMap->at(pInputs[0])[pCh];

                auto params = (FxParamsLowPassFirstOrder *) pParams;

                auto samplingPeriod = 1.0F / (float) pSampleRate;
                auto b              = 2 * (float) M_PI * samplingPeriod * params->cutoffFreq;
                auto a              = b / (b + 1);

                if (!*pUsrData)
                {
                    *pUsrData            = new float;
                    *(float *) *pUsrData = pIn[0];
                }

                auto lastState = *(float *) *pUsrData;
                pOut[0]        = a * pIn[0] + (1 - a) * lastState;

                for (int i = 1; i < pBufSize; ++i)
                {
                    pOut[i] = a * pIn[i] + (1 - a) * pOut[i - 1];
                }

                *(float *) *pUsrData = pOut[pBufSize - 1];
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
    typedef struct
    {
        float cutoffFreq;
    } FxParamsHighPassFirstOrder;

    class FxDescriptorHighPassFilterFirstOrder : public FxDescriptor
    {
        public:
        explicit FxDescriptorHighPassFilterFirstOrder(FxInstanceId pInput, float pCutoffFreq = 22000.0F) : FxDescriptor(pInput)
        {
            auto par        = new FxParamsHighPassFirstOrder();
            par->cutoffFreq = pCutoffFreq;

            params = par;

            processor = [](std::vector<FxInstanceId> &pInputs, float *pOut, int pBufSize, int pSampleRate, void *pParams, void **pUsrData, int pCh, std::map<int, std::array<float *, 2> > *pOutputMap)
            {
                auto pIn = pOutputMap->at(pInputs[0])[pCh];

                auto params = (FxParamsHighPassFirstOrder *) pParams;

                auto samplingPeriod = 1.0F / (float) pSampleRate;
                auto a              = 1.0F / (1 + 2 * (float) M_PI * samplingPeriod * params->cutoffFreq);

                if (!*pUsrData)
                {
                    *pUsrData            = new float;
                    *(float *) *pUsrData = pIn[0];
                }

                auto lastState = *(float *) *pUsrData;
                pOut[0]        = lastState;

                for (int i = 1; i < pBufSize; ++i)
                {
                    pOut[i] = a * pOut[i - 1] + a * (pIn[i] - pIn[i - 1]);
                }

                *(float *) *pUsrData = pOut[pBufSize - 1];
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
