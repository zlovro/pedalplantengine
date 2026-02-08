//
// Created by lovro on 03/02/2026.
// Copyright (c) 2026 lovro. All rights reserved.
//

#ifndef FX_MIX_HPP
#define FX_MIX_HPP

#include <array>
#include <algorithm>

#include <fx/fx.hpp>

namespace Fx
{
    // sum
    typedef struct
    {
        float weightA, weightB;
        float outGain;
    } FxParamsSum;

    class FxDescriptorSum : public FxDescriptor
    {
        public:
        FxDescriptorSum(int pAInstanceId, int pBInstanceId, float pWeightA, float pWeightB, float pOutGain) : FxDescriptor()
        {
            inputs = std::vector{pAInstanceId, pBInstanceId};

            auto par = new FxParamsSum();

            par->weightA = pWeightA;
            par->weightB = pWeightB;
            par->outGain = pOutGain;

            params = par;

            processor = [](std::vector<FxInstanceId> &pInputs, float *pOut, int pBufSize, int pSampleRate, void *pParams, void **pUsrData, int pCh, std::map<int, std::array<float *, 2> > *pOutputMap)
            {
                auto params = (FxParamsSum *) pParams;

                auto weightA = params->weightA;
                auto weightB = params->weightB;
                auto outGain = params->outGain;

                auto aData = pOutputMap->at(pInputs[0])[pCh];
                auto bData = pOutputMap->at(pInputs[1])[pCh];

                for (int i = 0; i < pBufSize; ++i)
                {
                    pOut[i] = std::clamp((weightA * aData[i] + weightB * bData[i]) * outGain, -1.0F, 1.0F);
                }
            };
        }

        int getExpectedInputCount() override
        {
            return 2;
        }

        FxParamsSum *getParams()
        {
            return (FxParamsSum *) params;
        }

        FxId getId() override
        {
            return FX_ID_SUM;
        }

        const char *getName() override
        {
            return "Sum";
        }

        ~FxDescriptorSum() override
        {
            delete getParams();
        }
    };

    // dry/wet
    typedef struct
    {
        float balance;
        float outGain;
    } FxParamsDryWet;

    class FxDescriptorDryWet : public FxDescriptor
    {
        public:
        FxDescriptorDryWet(int pAInstanceId, int pBInstanceId, float pBalance, float pOutGain) : FxDescriptor()
        {
            inputs = std::vector{pAInstanceId, pBInstanceId};

            auto par = new FxParamsDryWet();

            par->balance     = pBalance;
            par->outGain     = pOutGain;

            params = par;

            processor = [](std::vector<FxInstanceId> &pInputs, float *pOut, int pBufSize, int pSampleRate, void *pParams, void **pUsrData, int pCh, std::map<int, std::array<float *, 2> > *pOutputMap)
            {
                auto params = (FxParamsDryWet *) pParams;

                auto weightA = 1.0F - params->balance;
                auto weightB = params->balance;
                auto outGain = params->outGain;

                auto aData = pOutputMap->at(pInputs[0])[pCh];
                auto bData = pOutputMap->at(pInputs[1])[pCh];

                for (int i = 0; i < pBufSize; ++i)
                {
                    pOut[i] = std::clamp((weightA * aData[i] + weightB * bData[i]) * outGain, -1.0F, 1.0F);
                }
            };
        }

        int getExpectedInputCount() override
        {
            return 2;
        }

        FxParamsDryWet *getParams()
        {
            return (FxParamsDryWet *) params;
        }

        FxId getId() override
        {
            return FX_ID_DRYWET;
        }

        const char *getName() override
        {
            return "Dry/Wet";
        }

        ~FxDescriptorDryWet() override
        {
            delete getParams();
        }
    };
}

#endif //FX_MIX_HPP
