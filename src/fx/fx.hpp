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
        FX_ID_LEVEL,
        FX_ID_COMPRESSOR,

        FX_ID_SUM,
        FX_ID_DRYWET,

        FX_ID_BEZIER_CURVE,
        FX_ID_GENERIC_DIODE_CURVE,
    } FxId;

    inline constexpr float FX_MAX_AUDIO_VALUE = 0.95F;

    typedef int    FxInstanceId;
    typedef void (*FxProcessor)(const std::vector<float *> &pInputs, float *pOut, int pBufSz, int pSampleRate, const void *pParams, int pCh);

    class FxDescriptor
    {
        public:
        // pInputs - array of float pointers.
        FxProcessor               processor;
        void *                    params;
        FxInstanceId              instanceId;
        std::vector<FxInstanceId> inputs;
        std::array<float *, 2>    lastOutput;

        FxDescriptor();
        explicit FxDescriptor(const std::vector<FxInstanceId> &pInputs);
        explicit FxDescriptor(FxInstanceId pInput);

        void refreshBuffers();

        virtual int         getExpectedInputCount() = 0;
        virtual FxId        getId() = 0;
        virtual const char *getName() = 0;

        virtual ~FxDescriptor()
        = default;
    };

    class FxChain
    {
        public:
        // dont add or remove from this list - only use it for iteration
        std::vector<FxDescriptor *> chainFront, chainBack;

        // dont add or remove from this map - only use it for iteration
        std::map<FxInstanceId, FxDescriptor *> fxIdToFxMap;

        bool isFrontChainValid;

        FxChain();

        // adds an element to the back chain without calling optimize(). useful for multiple adds
        void addFxNoOptimize(FxDescriptor *pFx);

        /**
         * modifies back chain. should (or must) be called before processing (preferably after adding/removing/inserting/changing chain). organizes the chain such that no element tries to capture output from an unprocessed fx
         * @return FxChain::isFrontChainValid
         */
        bool optimize();

        void copyBackChainToFrontOptimize();

        bool deserialize(const std::filesystem::path &pFile);
        bool serialize(const std::filesystem::path &pFile);
    };

    extern FxChain gFxChain;

    inline constexpr FxInstanceId FX_INVALID_INSTANCE_ID = -1;

    extern FxInstanceId gFxInputInstanceId;

    void init();
}


#endif //FX_HPP
