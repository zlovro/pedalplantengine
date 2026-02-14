//
// Created by lovro on 10/02/2026.
// Copyright (c) 2026 lovro. All rights reserved.
//

#ifndef MAIN_HPP
#define MAIN_HPP

#include <asio.h>
#include <json.hpp>

#include "event.hpp"

typedef enum
{
    ERR_OK,

    ERR_ASIO_INIT = 1000,
    ERR_ASIO_CREATE_BUFFERS,
    ERR_ASIO_START,
    ERR_ASIO_STOP,
    ERR_ASIO_CH_INFO,

    ERR_ASIO_DRV_INIT_COULD_NOT_LOAD_DRIVER = 2000,

    ERR_ASIO_BYPASS = 99000,
} errCode;

typedef enum
{
    LEVEL_UNIT_LINEAR,
    LEVEL_UNIT_PERCENTAGE,
    LEVEL_UNIT_DB,
} levelUnit;

NLOHMANN_JSON_SERIALIZE_ENUM(levelUnit, {
                             {LEVEL_UNIT_LINEAR, "linear"},
                             {LEVEL_UNIT_PERCENTAGE, "percentage"},
                             {LEVEL_UNIT_DB, "db"}
                             })

inline constexpr int ASIO_CH_NUM = 4;

typedef struct
{
    ASIOChannelInfo chInfos[ASIO_CH_NUM];
    ASIODriverInfo  drvInf;

    long mnBufSz, mxBufSz;
    long prefferedBufSz;
    long granularity;
    long actualBufSz;

    long sampleRate;

    long numInCh, numOutCh;
} ASIODriverInfoEx;

extern ASIODriverInfoEx gAsioDrvInfEx;

void mainUpdateBuffers();
void mainDestroyFxChain();

extern Event gEventMainWindowAfterInit;

#endif //MAIN_HPP
