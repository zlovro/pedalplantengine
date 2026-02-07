//
// Created by lovro on 04/02/2026.
// Copyright (c) 2026 lovro. All rights reserved.
//

#ifndef FX_CURVE_HPP
#define FX_CURVE_HPP

#include <algorithm>
#include <cmath>
#include <queue>
#include <vector>

#include <fx/fx.hpp>

namespace Fx
{
    typedef struct
    {
        float x, y;
    } FxParamsCurvePoint;

    // bezier curve
    typedef struct
    {
        // how many discrete points per 1 unit, DO NOT SET DIRECTLY (use setCurveResolution)
        int resolution;

        // [resolution] points, key = t, t of point = i / resolution
        FxParamsCurvePoint *evalTable;

        std::vector<FxParamsCurvePoint> *points;
    } FxParamsBezierCurve;

    class FxDescriptorBezierCurve : public FxDescriptor
    {
        public:
        explicit FxDescriptorBezierCurve(FxInstanceId pInput, std::vector<FxParamsCurvePoint> *pPoints, int pResolution = 20) : FxDescriptor(pInput)
        {
            auto par = new FxParamsBezierCurve();

            par->evalTable = nullptr;
            par->points    = pPoints;

            // recalculates bezier and sets resolution
            setCurveResolution(pResolution);

            params = par;

            processor = [](std::vector<FxInstanceId> &pInputs, float *pOut, int pBufSize, int pSampleRate, void *pParams, void **pUsrData, int pCh, std::map<int, std::array<float *, 2> > *pOutputMap)
            {
                auto pIn = pOutputMap->at(pInputs[0])[pCh];

                auto params      = (FxParamsBezierCurve *) pParams;
                auto evalTable   = params->evalTable;
                auto resolution  = params->resolution;
                auto fResolution = (float) resolution;

                for (int i = 0; i < pBufSize; ++i)
                {
                    auto x   = pIn[i];
                    auto idx = (int) std::round(std::abs(x) * fResolution);
                    pOut[i]  = copysignf(evalTable[idx].y, x);
                };
            };
        }

        void setCurveResolution(int pResolution)
        {
            auto params = getParams();

            params->resolution = pResolution;

            delete[] params->evalTable;
            params->evalTable = new FxParamsCurvePoint[pResolution];

            recalculateBezier();
        }

        FxParamsCurvePoint evalBezier(float pT)
        {
            auto newPoints = std::vector(*getParams()->points);
            newPoints.insert(newPoints.begin(), {0, 0});
            newPoints.push_back({1, 1});

            auto n = newPoints.size();

            // ignore the bezier pyramid first and last row
            for (int i = 0; i < n - 2; i++)
            {
                // each rows length is N - 1 - i
                for (int j = 1; j < n - 1 - i; j++)
                {
                    auto a = newPoints[j - 1];
                    auto b = newPoints[j];

                    newPoints[j - 1] = FxParamsCurvePoint{std::lerp(a.x, b.x, pT), std::lerp(a.y, b.y, pT)};
                }
            }

            return newPoints[0];
        }

        float bezierFindTForX(float pX)
        {
            constexpr float EPSILON = 0.01F;

            float move = 0.25F;
            float t    = 0.5F;

            while (true)
            {
                auto point = evalBezier(t);
                auto error = point.x - pX;
                if (std::abs(error) < EPSILON)
                {
                    return t;
                }

                t = t + (error > 0 ? -move : move);
                move /= 2;
            }
        }

        void recalculateBezier()
        {
            auto params = getParams();

            auto resolution = params->resolution;
            auto step       = 1.0F / (float) (resolution - 1);

            float x = 0;
            for (int r = 0; r < resolution; r++, x += step)
            {
                float t;
                if (r == 0 || r == resolution - 1)
                {
                    t = x;
                }
                else
                {
                    t = bezierFindTForX(x);
                }

                params->evalTable[r] = evalBezier(t);
            }
        }

        FxParamsBezierCurve *getParams()
        {
            return (FxParamsBezierCurve *) params;
        }

        FxId getId() override
        {
            return FX_ID_BEZIER_CURVE;
        }

        const char *getName() override
        {
            return "Bezier curve";
        }

        ~FxDescriptorBezierCurve()
        override
        {
            auto params = getParams();
            delete[] params->evalTable;
            delete params->points;
            delete params;
        }
    };

    typedef struct
    {
        // how many discrete points per 1 unit, DO NOT SET DIRECTLY (use setCurveResolution)
        int resolution;

        // [resolution] points, key = t, t of point = i / resolution
        FxParamsCurvePoint *evalTable;

        // between 1 and 2, DO NOT SET DIRECTLY (use setCoefficient)
        float coefficient;
    } FxParamsGenericDiodeCurve;

    class FxDescriptorGenericDiodeCurve : public FxDescriptor
    {
        public:
        explicit FxDescriptorGenericDiodeCurve(FxInstanceId pInput, float pCoefficient = 1.0F, int pResolution = 20) : FxDescriptor(pInput)
        {
            auto par = new FxParamsGenericDiodeCurve();

            par->evalTable   = nullptr;
            par->coefficient = pCoefficient;

            setCurveResolution(pResolution);

            params = par;

            processor = [](std::vector<FxInstanceId> &pInputs, float *pOut, int pBufSize, int pSampleRate, void *pParams, void **pUsrData, int pCh, std::map<int, std::array<float *, 2> > *pOutputMap)
            {
                auto pIn = pOutputMap->at(pInputs[0])[pCh];

                auto params      = (FxParamsGenericDiodeCurve *) pParams;
                auto evalTable   = params->evalTable;
                auto resolution  = params->resolution;
                auto fResolution = (float) resolution;

                for (int i = 0; i < pBufSize; ++i)
                {
                    auto x   = pIn[i];
                    auto idx = (int) std::round(std::abs(x) * fResolution);
                    pOut[i]  = copysignf(evalTable[idx].y, x);
                };
            };
        }

        void setCoefficient(float pCoefficient)
        {
            getParams()->coefficient = pCoefficient;
            recalculateCurve();
        }

        void setCurveResolution(int pResolution)
        {
            auto params = getParams();

            params->resolution = pResolution;

            delete[] params->evalTable;
            params->evalTable = new FxParamsCurvePoint[pResolution];

            recalculateCurve();
        }

        void recalculateCurve()
        {
            auto params    = getParams();
            auto evalTable = params->evalTable;
            auto res       = params->resolution;

            auto step = 1.0F / ((float) res - 1.0F);

            auto x = 0.0F;
            for (int i = 0; i < res; ++i, x += step)
            {
                constexpr float Is = 1.0e-12;
                constexpr float q  = 1.602176634e-19;
                constexpr float n  = 1.0F;
                constexpr float T  = 300;
                constexpr float k  = 1.380649e-23;

                auto y       = std::clamp(Is * (powf(std::numbers::e, (q * x) / (n * k * T)) - 1), -1.0F, 1.0F);
                evalTable[i] = {x, y};
            }
        }

        FxParamsGenericDiodeCurve *getParams()
        {
            return (FxParamsGenericDiodeCurve *) params;
        }

        FxId getId() override
        {
            return FX_ID_GENERIC_DIODE_CURVE;
        }

        const char *getName() override
        {
            return "Generic Si diode";
        }

        ~FxDescriptorGenericDiodeCurve()
        override
        {
            auto params = getParams();
            delete[] params->evalTable;
            delete params;
        }
    };
}

#endif //FX_CURVE_HPP
