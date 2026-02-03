#include <iostream>
#include <complex>
#include <queue>
#include <map>

#include <asiodrivers.h>
#include <asio.h>

#include <fx/fx.hpp>
#include <fx/fx_amplitude.hpp>
#include <fx/fx_mix.hpp>
#include <fx/fx_xpass.hpp>

#include <wav.hpp>

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

std::vector<FxDescriptor *> gFxChain;

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

std::map<int, std::array<void *, 2> >  gFxUsrDataMap;
std::map<int, std::array<float *, 2> > gFxOutputDataMap;

int gFxInputInstanceId = 0;

void fxUpdateChainMemory()
{
    for (auto &fx: gFxChain)
    {
        if (fx->instanceId != gFxInputInstanceId)
        {
            delete[] gFxOutputDataMap[fx->instanceId][0];
            delete[] gFxOutputDataMap[fx->instanceId][1];
        }

        delete gFxUsrDataMap[fx->instanceId][0];
        delete gFxUsrDataMap[fx->instanceId][1];
    }

    gFxUsrDataMap.clear();
    gFxOutputDataMap.clear();

    gFxOutputDataMap[gFxInputInstanceId][0] = gInBuf[0];
    gFxOutputDataMap[gFxInputInstanceId][1] = gInBuf[1];

    for (auto &fx: gFxChain)
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

    auto workIn  = gWorkBuf[pCh];
    auto workOut = gWorkBuf[!pCh];

    auto bufSz      = gAsioDrvInfEx.actualBufSz;
    auto sampleRate = gAsioDrvInfEx.sampleRate;

    memcpy(workIn, in, bufSz * sizeof(float));

    for (auto &fx: gFxChain)
    {
        void *usrData = nullptr;
        if (gFxUsrDataMap.contains(fx->instanceId))
        {
            usrData = gFxUsrDataMap[fx->instanceId][pCh];
        }

        fx->processor(workIn, workOut, bufSz, sampleRate, fx->params, &usrData, pCh, &gFxOutputDataMap);
        memcpy(workIn, workOut, bufSz * sizeof(float));

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

void asioCbBufSw(long pDoubleBufIdx, ASIOBool pDirectProcess)
{
    long bufSz = gAsioDrvInfEx.actualBufSz;

    for (int i = 0; i < ASIO_CH_NUM; ++i)
    {
        auto bufInfo = gAsioBufInfos[i];
        auto chInfo  = gAsioDrvInfEx.chInfos[i];

        auto inBuf  = gInBuf[bufInfo.channelNum];
        auto outBuf = gOutBuf[bufInfo.channelNum];

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
                        inBuf[j]  = (float) y / (float) -INT32_MIN;
                    }
                    break;
                }

                case ASIOSTInt16MSB:
                {
                    for (int j = 0; j < bufSz; ++j)
                    {
                        int16_t y = ((int16_t *) srcBuf)[j];
                        inBuf[j]  = (float) y / (float) -INT16_MIN;
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

    process();
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
    auto lo1                     = new FxDescriptorLowPassFilterFirstOrder();
    lo1->getParams()->cutoffFreq = 2000;

    auto lo2                     = new FxDescriptorLowPassFilterFirstOrder();
    lo2->getParams()->cutoffFreq = 2000;

    auto lo3                     = new FxDescriptorLowPassFilterFirstOrder();
    lo3->getParams()->cutoffFreq = 2000;

    gFxChain.push_back((FxDescriptor *) lo1);
    gFxChain.push_back((FxDescriptor *) lo2);
    gFxChain.push_back((FxDescriptor *) lo3);
}

void testHipassChain()
{
    auto hi1                     = new FxDescriptorHighPassFilterFirstOrder();
    hi1->getParams()->cutoffFreq = 6000;

    gFxChain.push_back((FxDescriptor *) hi1);
}

void testHiLoChain()
{
    auto lo1                     = new FxDescriptorLowPassFilterFirstOrder();
    lo1->getParams()->cutoffFreq = 8000;

    auto gain1               = new FxDescriptorGain();
    gain1->getParams()->gain = 2;

    auto gain2               = new FxDescriptorGain();
    gain2->getParams()->gain = 0.5F;

    auto hi1                     = new FxDescriptorHighPassFilterFirstOrder();
    hi1->getParams()->cutoffFreq = 1000;

    gFxChain.push_back((FxDescriptor *) lo1);
    gFxChain.push_back((FxDescriptor *) gain1);
    gFxChain.push_back((FxDescriptor *) gain2);
    gFxChain.push_back((FxDescriptor *) hi1);
}

void testDistorsion()
{
    auto hi1   = new FxDescriptorHighPassFilterFirstOrder(3000);
    auto gain1 = new FxDescriptorGain(10000);
    auto gain2 = new FxDescriptorGain(0.1F);
    auto lo1   = new FxDescriptorLowPassFilterFirstOrder(17000);

    const float DIST = 0.74F;
    auto        sum1 = new FxDescriptorSum(gFxInputInstanceId, 1 - DIST, lo1->instanceId, DIST, 2);

    gFxChain.push_back(hi1);
    gFxChain.push_back(gain1);
    gFxChain.push_back(gain2);
    gFxChain.push_back(lo1);
    gFxChain.push_back(sum1);
}

errCode main2()
{
    errCode err = asioInitDrivers();
    if (err != ERR_OK)
    {
        return err;
    }

    ASIOError asioErr = ASIOInit(&gAsioDrvInfEx.drvInf);
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

    gFxInputInstanceId = rand();

    gInBuf[0]   = new float[bufSz];
    gInBuf[1]   = new float[bufSz];
    gWorkBuf[0] = new float[bufSz];
    gWorkBuf[1] = new float[bufSz];
    gOutBuf[0]  = new float[bufSz];
    gOutBuf[1]  = new float[bufSz];

    // testLopassChain();
    // testHipassChain();
    // testHiLoChain();
    testDistorsion();

    fxUpdateChainMemory();

    asioErr = ASIOStart();
    if (asioErr != ASE_OK)
    {
        return ERR_ASIO_START;
    }

    while (gRun)
    {
        sleepMs(100);

        #if WINDOWS
        if (GetKeyState(VK_CONTROL) >> 15 && GetKeyState(VK_MENU) >> 15)
        {
            gRun = false;
        }
        #endif

        fflush(stdout);

        gTimeMs += 100;

        // auto sumParams     = ((FxParamsSum *) gFxChain.at(gFxChain.size() - 1)->params);
        // auto y             = (sinf(((float) gTimeMs / 1000.0F) * 2 * (float) M_PI / 5.0F) + 1) / 2;
        // sumParams->weightA = y;
        // sumParams->weightB = 1 - y;

        // if (gTimeMs >= 5000)<
        // {
        //     break;
        // }>
    }

    // wavDump(gDbgOut, "lopass.wav", gAsioDrvInfEx.sampleRate);

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

    return ERR_OK;
}

int main()
{
    errCode err = main2();

    asioDeinitDrivers();

    return err;
}
