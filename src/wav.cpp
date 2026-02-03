//
// Created by lovro on 02/02/2026.
// Copyright (c) 2026 lovro. All rights reserved.
//

#include "wav.hpp"

#include <cstdint>
#include <fstream>

typedef struct WAV_HEADER
{
    /* RIFF Chunk Descriptor */
    uint8_t  RIFF[4] = {'R', 'I', 'F', 'F'}; // RIFF Header Magic header
    uint32_t ChunkSize;                      // RIFF Chunk Size
    uint8_t  WAVE[4] = {'W', 'A', 'V', 'E'}; // WAVE Header
    /* "fmt" sub-chunk */
    uint8_t  fmt[4]        = {'f', 'm', 't', ' '}; // FMT header
    uint32_t Subchunk1Size = 16;                   // Size of the fmt chunk
    uint16_t AudioFormat   = 1;                    // Audio format 1=PCM,6=mulaw,7=alaw,     257=IBM
    // Mu-Law, 258=IBM A-Law, 259=ADPCM
    uint16_t NumOfChan     = 1;         // Number of channels 1=Mono 2=Sterio
    uint32_t SamplesPerSec = 16000;     // Sampling Frequency in Hz
    uint32_t bytesPerSec   = 16000 * 2; // bytes per second
    uint16_t blockAlign    = 2;         // 2=16-bit mono, 4=16-bit stereo
    uint16_t bitsPerSample = 16;        // Number of bits per sample
    /* "data" sub-chunk */
    uint8_t  Subchunk2ID[4] = {'d', 'a', 't', 'a'}; // "data"  string
    uint32_t Subchunk2Size;                         // Sampled data length
} wav_hdr;

void wavDump(std::vector<float> &pDataIn, std::string pFileName, int pSampleRate)
{
    static_assert(sizeof(wav_hdr) == 44, "");

    auto fsize = pDataIn.size() * 2;

    wav_hdr wav;
    wav.SamplesPerSec = pSampleRate;
    wav.bytesPerSec   = pSampleRate * 2;
    wav.ChunkSize     = fsize + sizeof(wav_hdr) - 8;
    wav.Subchunk2Size = fsize + sizeof(wav_hdr) - 44;

    std::ofstream out(pFileName,  std::ios::out | std::ios::binary);
    out.write((char*)&wav, sizeof(wav));

    for (int i = 0; i < pDataIn.size(); i++)
    {
        short x = pDataIn[i] * 32767;
        out.write((char*)&x, 2);
    }
}
