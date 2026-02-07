//
// Created by lovro on 02/02/2026.
// Copyright (c) 2026 lovro. All rights reserved.
//

#ifndef FX_HPP
#define FX_HPP

#include <cstdint>
#include <filesystem>
#include <string>
#include <map>
#include <vector>

namespace Fx
{
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

    typedef int    FxInstanceId;
    typedef void (*FxProcessor)(std::vector<FxInstanceId> &pInputs, float *pOut, int pBufSz, int pSampleRate, void *pParams, void **pUsrData, int pCh, std::map<int, std::array<float *, 2> > *pOutputMap);

    class FxDescriptor
    {
        public:
        // usrData, if unused, must be nullptr, if used, must be allocated with new (NOT new[]!) and the underlying type should not have any destructors
        FxProcessor               processor{};
        void *                    params{};
        FxInstanceId              instanceId{};
        std::vector<FxInstanceId> inputs;

        FxDescriptor();
        explicit FxDescriptor(std::vector<FxInstanceId> &pInputs);
        explicit FxDescriptor(FxInstanceId pInput);

        virtual FxId        getId() = 0;
        virtual const char *getName() = 0;

        virtual ~FxDescriptor()
        = default;
    };

    class FxChain
    {
        public:
        std::vector<FxDescriptor *> chain;

        FxChain();

        // should (or must) be called before processing (preferably after adding/removing/inserting/changing chain). organizes the chain such that no element tries to capture output from an unprocessed fx
        void optimize();

        bool deserialize(const std::filesystem::path &pFile);
        bool serialize(const std::filesystem::path &pFile);
    };
}

constexpr Fx::FxInstanceId FX_INVALID_INSTANCE_ID = -1;

extern Fx::FxInstanceId gFxInputInstanceId;

#endif //FX_HPP
