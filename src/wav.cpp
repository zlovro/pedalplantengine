//
// Created by lovro on 02/02/2026.
// Copyright (c) 2026 lovro. All rights reserved.
//

#include "wav.hpp"

#include <cstdint>
#include <fstream>

typedef struct
{
    uint8_t  riffMagic[4] = {'R', 'I', 'F', 'F'};
    uint32_t riffChSz;

    uint8_t waveMagic[4] = {'W', 'A', 'V', 'E'};
    uint8_t fmtMagic[4]  = {'f', 'm', 't', ' '};

    uint32_t fmtChSz        = 16;
    uint16_t audioFmt       = 1;
    uint16_t chCount        = 1;
    uint32_t sampleRate     = 48000;
    uint32_t bytesPerSecond = 48000 * 2;
    uint16_t blockAlign     = 2;
    uint16_t bitsPerSample  = 16;

    uint8_t  dataMagic[4] = {'d', 'a', 't', 'a'};
    uint32_t dataSize;
} wavHeader;

void wavDump(std::vector<float> &pDataIn, const std::string &pFileName, int pSampleRate)
{
    static_assert(sizeof(wavHeader) == 44);

    auto fsize = pDataIn.size() * 2;

    wavHeader wav;
    wav.sampleRate     = pSampleRate;
    wav.bytesPerSecond = pSampleRate * 2;
    wav.riffChSz       = fsize + sizeof(wav) - 8;
    wav.dataSize       = fsize + sizeof(wav) - 44;

    std::ofstream out(pFileName, std::ios::out | std::ios::binary);
    out.write((char *) &wav, sizeof(wav));

    for (float i: pDataIn)
    {
        auto x = (short) (i * 32767.0F);
        out.write((char *) &x, 2);
    }
}
