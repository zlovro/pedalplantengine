//
// Created by lovro on 03/02/2026.
// Copyright (c) 2026 lovro. All rights reserved.
//

#ifndef FX_MIX_HPP
#define FX_MIX_HPP

#include <array>
#include <algorithm>

#include <fx/fx.hpp>

// gain
typedef struct
{
    int aInstanceId, bInstanceId;

    float weightA, weightB;
    float outGain;
} FxParamsSum;

class FxDescriptorSum : public FxDescriptor
{
    public:
    FxDescriptorSum(int pAInstanceId, float pWeightA, int pBInstanceId, float pWeightB, float pOutGain)
    {
        auto par = new FxParamsSum();

        par->aInstanceId = pAInstanceId;
        par->weightA     = pWeightA;

        par->bInstanceId = pBInstanceId;
        par->weightB     = pWeightB;

        par->outGain = pOutGain;

        params = par;

        processor = [](float *pIn, float *pOut, int pBufSize, int pSampleRate, void *pParams, void **pUsrData, int pCh, std::map<int, std::array<float *, 2> > *pOutputMap)
        {
            auto params = (FxParamsSum *) pParams;

            auto weightA = params->weightA;
            auto weightB = params->weightB;
            auto outGain = params->outGain;

            auto aInstanceId = params->aInstanceId;
            auto bInstanceId = params->bInstanceId;

            auto aData = pOutputMap->at(aInstanceId)[pCh];
            auto bData = pOutputMap->at(bInstanceId)[pCh];

            for (int i = 0; i < pBufSize; ++i)
            {
                pOut[i] = std::clamp((weightA * aData[i] + weightB * bData[i]) * outGain, -1.0F, 1.0F);
            }
        };
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

#endif //FX_MIX_HPP
