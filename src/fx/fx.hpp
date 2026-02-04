//
// Created by lovro on 02/02/2026.
// Copyright (c) 2026 lovro. All rights reserved.
//

#ifndef FX_HPP
#define FX_HPP

#include <cstdint>
#include <string>
#include <map>

typedef enum
{
    FX_ID_NULL,

    FX_ID_LOPASS_FIRST_ORDER,
    FX_ID_HIPASS_FIRST_ORDER,

    FX_ID_GAIN,
    FX_ID_COMPRESSOR,

    FX_ID_SUM,
    FX_ID_DRYWET,

    FX_ID_BEZIER_CURVE,
    FX_ID_GENERIC_DIODE_CURVE,
} FxId;

typedef void (*FxProcessor)(float *pIn, float *pOut, int pBufSz, int pSampleRate, void *pParams, void **pUsrData, int pCh, std::map<int, std::array<float *, 2> > *pOutputMap);

class FxDescriptor
{
    public:
    // usrData, if unused, must be nullptr, if used, must be allocated with new (NOT new[]!) and the underlying type should not have any destructors
    FxProcessor processor;
    void *      params;
    // void (*     paramsDestructor)(FxDescriptor *pFx){};
    int         instanceId;

    FxDescriptor();

    virtual FxId        getId();
    virtual const char *getName();

    virtual ~FxDescriptor()
    = default;
};

#endif //FX_HPP
