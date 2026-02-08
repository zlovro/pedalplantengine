#include <iostream>
#include <complex>
#include <queue>
#include <map>

#include <QApplication>
#include <QPushButton>
#include <QMainWindow>
#include <QStyleFactory>

#include <asiodrivers.h>
#include <asio.h>

#include <fx/fx.hpp>
#include <fx/fx_amplitude.hpp>
#include <fx/fx_mix.hpp>
#include <fx/fx_xpass.hpp>
#include <fx/fx_curve.hpp>

#include <wav.hpp>

#include "fx_gfx.hpp"
#include "fx_gfx.hpp"

#if WINDOWS
#define sleepMs(x) Sleep(x)
#endif

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

constexpr int  ASIO_CH_NUM                = 4;
ASIOBufferInfo gAsioBufInfos[ASIO_CH_NUM] = {
    {true, 0},
    {true, 1},
    {false, 0},
    {false, 1}
};

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

AsioDrivers *    gAsioDrivers;
ASIODriverInfoEx gAsioDrvInfEx = {};
bool             gRun          = true;

float *gInBuf[2];
float *gWorkBuf[2];
float *gOutBuf[2];

Fx::FxChain gFxChain;

errCode asioInitDrivers()
{
    gAsioDrivers = new AsioDrivers();

    constexpr int MAX_ASIO_DRIVERS = 32;
    char *        drvNames[MAX_ASIO_DRIVERS];
    for (auto &drvName: drvNames)
    {
        drvName = (char *) malloc(32);
    }

    int outDrvNameCount = gAsioDrivers->getDriverNames(drvNames, MAX_ASIO_DRIVERS);
    for (int i = 0; i < outDrvNameCount; ++i)
    {
        if (gAsioDrivers->loadDriver(drvNames[i]))
        {
            goto asioLoadedDrivers;
        }
    }

    return ERR_ASIO_DRV_INIT_COULD_NOT_LOAD_DRIVER;

asioLoadedDrivers:
    for (auto &drvName: drvNames)
    {
        free(drvName);
    }

    return ERR_OK;
}

void asioDeinitDrivers()
{
    gAsioDrivers->removeCurrentDriver();

    delete gAsioDrivers;
}

std::map<Fx::FxInstanceId, std::array<void *, 2> >  gFxUsrDataMap;
std::map<Fx::FxInstanceId, std::array<float *, 2> > gFxOutputDataMap;

extern Fx::FxInstanceId gFxInputInstanceId  = 0;
extern Fx::FxInstanceId gFxOutputInstanceId = 0;

void fxDestroyChain()
{
    for (auto &fx: gFxChain.chain)
    {
        delete[] gFxOutputDataMap[fx->instanceId][0];
        delete[] gFxOutputDataMap[fx->instanceId][1];

        delete gFxUsrDataMap[fx->instanceId][0];
        delete gFxUsrDataMap[fx->instanceId][1];

        delete fx;
    }

    gFxUsrDataMap.clear();
    gFxOutputDataMap.clear();
}

void fxUpdateChainMemory()
{
    fxDestroyChain();

    gFxOutputDataMap[gFxInputInstanceId][0] = gInBuf[0];
    gFxOutputDataMap[gFxInputInstanceId][1] = gInBuf[1];

    for (auto &fx: gFxChain.chain)
    {
        gFxOutputDataMap[fx->instanceId][0] = new float[gAsioDrvInfEx.actualBufSz];
        gFxOutputDataMap[fx->instanceId][1] = new float[gAsioDrvInfEx.actualBufSz];
    }
}

uint64_t gTimeMs;

void processChannel(int pCh)
{
    auto out = gOutBuf[pCh];
    auto in  = gInBuf[pCh];

    // auto workIn  = gWorkBuf[pCh];
    auto workOut = gWorkBuf[pCh];

    auto bufSz      = gAsioDrvInfEx.actualBufSz;
    auto sampleRate = gAsioDrvInfEx.sampleRate;

    // memcpy(workIn, in, bufSz * sizeof(float));

    for (auto &fx: gFxChain.chain)
    {
        void *usrData = nullptr;
        if (gFxUsrDataMap.contains(fx->instanceId))
        {
            usrData = gFxUsrDataMap[fx->instanceId][pCh];
        }

        fx->processor(fx->inputs, workOut, bufSz, sampleRate, fx->params, &usrData, pCh, &gFxOutputDataMap);
        // memcpy(workIn, workOut, bufSz * sizeof(float));

        memcpy(gFxOutputDataMap[fx->instanceId][pCh], workOut, sizeof(float) * bufSz);

        gFxUsrDataMap[fx->instanceId][pCh] = usrData;
    }

    memcpy(out, workOut, bufSz * sizeof(float));
}

void process()
{
    processChannel(0);
    processChannel(1);
}

typedef enum
{
    APP_STATE_REGULAR,
    APP_STATE_RECORDING_INPUT,
    APP_STATE_RECORDING_OUTPUT,
    APP_STATE_PLAYING_LOOP,
} AppState;

AppState                          gAppState = APP_STATE_REGULAR;
std::array<std::vector<float>, 2> gLoopBuffer;
std::array<std::vector<float>, 2> gRecordOutBuffer;

void startRecordingOutput()
{
    gAppState = APP_STATE_RECORDING_OUTPUT;

    gRecordOutBuffer[0].clear();
    gRecordOutBuffer[1].clear();
}

void stopRecordingOutput()
{
    wavDump(gRecordOutBuffer[0], "recording-out-ch0.wav", gAsioDrvInfEx.sampleRate);
    wavDump(gRecordOutBuffer[1], "recording-out-ch1.wav", gAsioDrvInfEx.sampleRate);

    gAppState = APP_STATE_REGULAR;

    gRecordOutBuffer[0].clear();
    gRecordOutBuffer[1].clear();
}

void startRecordingInput()
{
    gAppState = APP_STATE_RECORDING_INPUT;

    gLoopBuffer[0].clear();
    gLoopBuffer[1].clear();
}

void stopRecordingInput()
{
    gAppState = APP_STATE_PLAYING_LOOP;
}

void asioCbBufSw(long pDoubleBufIdx, ASIOBool pDirectProcess)
{
    long bufSz = gAsioDrvInfEx.actualBufSz;

    for (int i = 0; i < ASIO_CH_NUM; ++i)
    {
        auto bufInfo = gAsioBufInfos[i];
        auto chInfo  = gAsioDrvInfEx.chInfos[i];

        auto inBuf = gInBuf[bufInfo.channelNum];
        if (gAppState == APP_STATE_PLAYING_LOOP)
        {
            inBuf = gLoopBuffer[bufInfo.channelNum].data();
        }

        auto outBuf = gOutBuf[bufInfo.channelNum];
        if (gAppState == APP_STATE_RECORDING_INPUT)
        {
            // dont apply effects, recording only input
            outBuf = inBuf;
        }

        if (bufInfo.isInput)
        {
            void *srcBuf = bufInfo.buffers[pDoubleBufIdx];

            switch (chInfo.type)
            {
                case ASIOSTInt32LSB:
                {
                    for (int j = 0; j < bufSz; ++j)
                    {
                        int32_t y = ((int32_t *) srcBuf)[j];
                        inBuf[j]  = (float) y / (float) (1L << 31);
                    }
                    break;
                }

                case ASIOSTInt16MSB:
                {
                    for (int j = 0; j < bufSz; ++j)
                    {
                        int16_t y = ((int16_t *) srcBuf)[j];
                        inBuf[j]  = (float) y / (float) (1L << 15);
                    }
                    break;
                }

                default:
                {
                    printf("Unimplemented sample type %d\n", chInfo.type);
                }
            }
            continue;
        }

        switch (chInfo.type)
        {
            case ASIOSTInt32LSB:
            {
                for (int j = 0; j < bufSz; ++j)
                {
                    ((int32_t *) bufInfo.buffers[pDoubleBufIdx])[j] = (int32_t) (outBuf[j] * INT32_MAX);
                }

                break;
            }

            default:
            {
                printf("Unimplemented sample type %d\n", chInfo.type);
            }
        }
    }

    if (gAppState == APP_STATE_RECORDING_INPUT)
    {
        gLoopBuffer[0].insert(gLoopBuffer[0].end(), gInBuf[0], gInBuf[0] + bufSz);
        gLoopBuffer[1].insert(gLoopBuffer[1].end(), gInBuf[1], gInBuf[1] + bufSz);
    }
    else
    {
        process();
    }
}

void asioCbSampleRateChange(ASIOSampleRate pSr)
{
}

long asioCbMsg(long pSelector, long pValue, void *pMsg, double *pOpt)
{
    switch (pSelector)
    {
        case kAsioResetRequest:
        {
            return 1;
        }


        default:
        {
            return -1;
        }
    }
}

ASIOTime *asioCbBufSwTimeInf(ASIOTime *pParams, long pDoubleBufIdx, ASIOBool pDirectProcess)
{
}

void testLopassChain()
{
    auto lo1                     = new Fx::FxDescriptorLowPassFilterFirstOrder(gFxInputInstanceId);
    lo1->getParams()->cutoffFreq = 2000;

    auto lo2                     = new Fx::FxDescriptorLowPassFilterFirstOrder(lo1->instanceId);
    lo2->getParams()->cutoffFreq = 2000;

    auto lo3                     = new Fx::FxDescriptorLowPassFilterFirstOrder(lo2->instanceId);
    lo3->getParams()->cutoffFreq = 2000;

    gFxChain.chain.push_back((Fx::FxDescriptor *) lo1);
    gFxChain.chain.push_back((Fx::FxDescriptor *) lo2);
    gFxChain.chain.push_back((Fx::FxDescriptor *) lo3);
}

void testHipassChain()
{
    auto hi1                     = new Fx::FxDescriptorHighPassFilterFirstOrder(gFxInputInstanceId);
    hi1->getParams()->cutoffFreq = 6000;

    gFxChain.chain.push_back((Fx::FxDescriptor *) hi1);
}

void testHiLoChain()
{
    auto lo1                     = new Fx::FxDescriptorLowPassFilterFirstOrder(gFxInputInstanceId);
    lo1->getParams()->cutoffFreq = 8000;

    auto gain1               = new Fx::FxDescriptorGain(lo1->instanceId);
    gain1->getParams()->gain = 2;

    auto gain2               = new Fx::FxDescriptorGain(gain1->instanceId);
    gain2->getParams()->gain = 0.5F;

    auto hi1                     = new Fx::FxDescriptorHighPassFilterFirstOrder(gain2->instanceId);
    hi1->getParams()->cutoffFreq = 1000;

    gFxChain.chain.push_back((Fx::FxDescriptor *) lo1);
    gFxChain.chain.push_back((Fx::FxDescriptor *) gain1);
    gFxChain.chain.push_back((Fx::FxDescriptor *) gain2);
    gFxChain.chain.push_back((Fx::FxDescriptor *) hi1);
}

void testDistorsion()
{
    auto hi1    = new Fx::FxDescriptorHighPassFilterFirstOrder(4000);
    auto gain1  = new Fx::FxDescriptorGain(800);
    auto diode1 = new Fx::FxDescriptorGenericDiodeCurve(gain1->instanceId, 1.05F, 10);

    const float DIST    = 0.74F;
    auto        dryWet1 = new Fx::FxDescriptorDryWet(gFxInputInstanceId, diode1->instanceId, DIST, 1.0F);

    gFxChain.chain.push_back(hi1);
    gFxChain.chain.push_back(gain1);
    gFxChain.chain.push_back(diode1);
    gFxChain.chain.push_back(dryWet1);
}

void testChainOptimizer()
{
    auto b = new Fx::FxDescriptorGain(gFxInputInstanceId);
    auto d = new Fx::FxDescriptorGain(b->instanceId);

    auto a = new Fx::FxDescriptorSum(gFxInputInstanceId, d->instanceId, 0.5F, 0.5F, 1.0F);

    auto e = new Fx::FxDescriptorGain(a->instanceId);
    auto f = new Fx::FxDescriptorDryWet(d->instanceId, e->instanceId, 0.78F, 1.0F);
    auto c = new Fx::FxDescriptorGain(gFxInputInstanceId);
    auto g = new Fx::FxDescriptorSum(c->instanceId, f->instanceId, 0.5F, 0.5F, 1.0F);
    auto w = new Fx::FxDescriptorDryWet(e->instanceId, d->instanceId, 0.4F, 1.0F);
    auto k = new Fx::FxDescriptorDryWet(w->instanceId, g->instanceId, 0.7F, 1.0F);

    gFxChain.chain = {
        d, b, e, f, c, g, w, k, a
    };

    gFxChain.optimize();

    gRun = false;
}

void testChainDeserializer()
{
    gFxChain.deserialize("examplepedalscheme.json");
    gFxChain.serialize("examplepedalscheme-serialized.json");
    gFxChain.deserialize("examplepedalscheme-serialized.json");
}

errCode main2(int argc, char *argv[])
{
    gFxInputInstanceId  = rand();
    gFxOutputInstanceId = rand();

    // testChainOptimizer();
    // testChainDeserializer();
    //
    // return ERR_ASIO_BYPASS;

    const bool TEST_GUI = true;

    errCode   err;
    ASIOError asioErr;

    if (!TEST_GUI)
    {
        err = asioInitDrivers();
        if (err != ERR_OK)
        {
            return err;
        }

        asioErr = ASIOInit(&gAsioDrvInfEx.drvInf);
        if (asioErr != ASE_OK)
        {
            return ERR_ASIO_INIT;
        }

        ASIOGetBufferSize(&gAsioDrvInfEx.mnBufSz, &gAsioDrvInfEx.mxBufSz, &gAsioDrvInfEx.prefferedBufSz, &gAsioDrvInfEx.granularity);
        ASIOGetChannels(&gAsioDrvInfEx.numInCh, &gAsioDrvInfEx.numOutCh);

        double sr;
        ASIOGetSampleRate((ASIOSampleRate *) &sr);

        gAsioDrvInfEx.sampleRate = (long) sr;

        ASIOCallbacks asioCbs = {asioCbBufSw, asioCbSampleRateChange, asioCbMsg, asioCbBufSwTimeInf};

        long bufSz                = true ? 64 : gAsioDrvInfEx.prefferedBufSz;
        gAsioDrvInfEx.actualBufSz = bufSz;

        asioErr = ASIOCreateBuffers(gAsioBufInfos, ASIO_CH_NUM, bufSz, &asioCbs);
        if (asioErr != ASE_OK)
        {
            return ERR_ASIO_CREATE_BUFFERS;
        }

        for (int i = 0; i < ASIO_CH_NUM; ++i)
        {
            auto bufInf = gAsioBufInfos[i];

            gAsioDrvInfEx.chInfos[i] = {
                .channel = bufInf.channelNum,
                .isInput = bufInf.isInput,
            };

            asioErr = ASIOGetChannelInfo(gAsioDrvInfEx.chInfos + i);
            if (asioErr != ASE_OK)
            {
                return ERR_ASIO_CH_INFO;
            }
        }

        gInBuf[0]   = new float[bufSz];
        gInBuf[1]   = new float[bufSz];
        gWorkBuf[0] = new float[bufSz];
        gWorkBuf[1] = new float[bufSz];
        gOutBuf[0]  = new float[bufSz];
        gOutBuf[1]  = new float[bufSz];
    }

    // testLopassChain();
    // testHipassChain();
    // testHiLoChain();
    // testDistorsion();
    // testChainOptimizer();

    fxUpdateChainMemory();

    if (!TEST_GUI)
    {
        asioErr = ASIOStart();
        if (asioErr != ASE_OK)
        {
            return ERR_ASIO_START;
        }
    }

    QApplication qtApp(argc, argv);

    QApplication::setStyle(QStyleFactory::create("Fusion"));

    Fx::Gfx::FxGfxMainWindow mainWindow;
    mainWindow.show();

    long desiredFrameDurationUs = 1'000'000 / 144;
    while (gRun)
    {
        auto frameStart = std::chrono::high_resolution_clock::now();

        #if WINDOWS
        if (GetKeyState(VK_CONTROL) >> 15 && GetKeyState(VK_MENU) >> 15)
        {
            gRun = false;
        }
        #endif

        QApplication::processEvents();

        fflush(stdout);

        while (true)
        {
            auto frameDurationUs = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - frameStart).count();
            if (frameDurationUs >= desiredFrameDurationUs)
            {
                break;
            }
        }
    }

    asioErr = ASIOStop();
    if (asioErr != ASE_OK)
    {
        return ERR_ASIO_STOP;
    }

    delete[] gInBuf[0];
    delete[] gInBuf[1];
    delete[] gWorkBuf[0];
    delete[] gWorkBuf[1];
    delete[] gOutBuf[0];
    delete[] gOutBuf[1];

    for (auto v: gFxUsrDataMap | std::views::values)
    {
        // its ok to delete a void pointer since we know it doesnt need to call any destructors, and is not of type []
        // ReSharper disable once CppDeletingVoidPointer
        delete v[0];

        // ReSharper disable once CppDeletingVoidPointer
        delete v[1];
    }

    if (!TEST_GUI)
    {
        asioDeinitDrivers();
    }

    return ERR_OK;
}

int main(int argc, char *argv[])
{
    errCode err = main2(argc, argv);

    return err;
}
