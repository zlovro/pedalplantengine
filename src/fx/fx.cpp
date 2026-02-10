//
// Created by lovro on 02/02/2026.
// Copyright (c) 2026 lovro. All rights reserved.
//

#include <cstdlib>
#include <set>
#include <queue>
#include <forward_list>
#include <filesystem>
#include <fstream>
#include <print>

#include <fx/fx.hpp>
#include <json.hpp>

#include "fx_amplitude.hpp"
#include "fx_curve.hpp"
#include "fx_mix.hpp"
#include "fx_xpass.hpp"

using json = nlohmann::json;

namespace Fx
{
    FxChain      gFxChain = FxChain();
    FxInstanceId gFxInputInstanceId;

    void init()
    {
        gFxInputInstanceId = rand();
    }

    FxDescriptor::FxDescriptor()
    {
        instanceId = rand();

        inputs = std::vector<FxInstanceId>();

        lastOutput    = std::array<float *, 2>();
        lastOutput[0] = nullptr;
        lastOutput[1] = nullptr;

        processor = nullptr;
        params    = nullptr;
    }

    FxDescriptor::FxDescriptor(const std::vector<FxInstanceId> &pInputs) : FxDescriptor()
    {
        inputs = pInputs;
    }

    FxDescriptor::FxDescriptor(FxInstanceId pInput) : FxDescriptor(std::vector{pInput})
    {
    }

    void FxDescriptor::refreshBuffers() const
    {
        if (instanceId == gFxInputInstanceId)
        {
            return;
        }

        delete[] lastOutput[0];
        delete[] lastOutput[1];
    }

    FxId FxDescriptor::getId()
    {
        return FX_ID_NULL;
    }

    const char *FxDescriptor::getName()
    {
        return nullptr;
    }

    FxChain::FxChain()
    {
        chain       = std::vector<FxDescriptor *>();
        fxIdToFxMap = std::map<FxInstanceId, FxDescriptor *>();
    }

    void FxChain::addFxNoOptimize(FxDescriptor *pFx)
    {
        chain.push_back(pFx);
        fxIdToFxMap[pFx->instanceId] = pFx;
    }

    void FxChain::optimize()
    {
        std::map<FxInstanceId, FxDescriptor *> idToInstanceMap;

        // topological sort setup

        std::vector<FxInstanceId> tasks;
        tasks.reserve(chain.size());

        std::set<std::pair<FxInstanceId, FxInstanceId> > dependencies;

        for (auto &fx: chain)
        {
            auto instanceId = fx->instanceId;

            tasks.push_back(instanceId);
            idToInstanceMap[instanceId] = fx;

            for (auto &input: fx->inputs)
            {
                dependencies.emplace(instanceId, input);
            }
        }

        // topological sort

        std::map<FxInstanceId, std::vector<FxInstanceId> > adj;
        std::map<FxInstanceId, int>                        inDegree;

        for (auto [dependent, root]: dependencies)
        {
            adj[dependent].push_back(root);
            inDegree[root]++;
        }

        std::deque<FxInstanceId> queue;
        for (auto &task: tasks)
        {
            if (inDegree[task] == 0)
            {
                queue.push_back(task);
            }
        }

        std::forward_list<FxInstanceId> result;

        while (!queue.empty())
        {
            auto task = queue.front();
            queue.pop_front();

            result.push_front(task);

            for (auto &root: adj[task])
            {
                inDegree[root]--;

                if (inDegree[root] == 0)
                {
                    queue.push_back(root);
                }
            }
        }

        // the first element will always be gFxInputInstanceId, it is unnecessary
        result.pop_front();

        auto i = 0;
        for (auto &instanceId: result)
        {
            chain[i++] = idToInstanceMap[instanceId];
        }
    }

    bool FxChain::deserialize(const std::filesystem::path &pFile)
    {
        std::ifstream file(pFile);
        if (!file.is_open())
        {
            return false;
        }

        auto data = json::parse(file);
        file.close();

        // name, (fxInstance, inputs[])
        std::map<std::string, std::pair<std::vector<std::string>, FxDescriptor *> > fxMap;

        for (auto &jsonFx: data)
        {
            auto name   = jsonFx["name"].get<std::string>();
            auto inputs = jsonFx["inputs"].get<std::vector<std::string> >();
            auto type   = jsonFx["type"].get<std::string>();
            auto params = jsonFx["params"];

            // amplitude
            if (type == "gain")
            {
                fxMap[name] = {inputs, new FxDescriptorGain(FX_INVALID_INSTANCE_ID, params["gain"].get<float>())};
            }

            // curves
            else if (type == "bezier")
            {
                auto points = new std::vector<FxParamsCurvePoint>();
                for (auto &point: params["points"])
                {
                    points->push_back({point[0].get<float>(), point[1].get<float>()});
                }
                auto bezier = new FxDescriptorBezierCurve(FX_INVALID_INSTANCE_ID, points, params["resolution"].get<int>());

                fxMap[name] = {inputs, bezier};
            }
            else if (type == "diode")
            {
                fxMap[name] = {inputs, new FxDescriptorGenericDiodeCurve(FX_INVALID_INSTANCE_ID, params["coefficient"].get<float>(), params["resoltuion"].get<int>())};
            }

            // mixers
            else if (type == "sum")
            {
                fxMap[name] = {inputs, new FxDescriptorSum(FX_INVALID_INSTANCE_ID, FX_INVALID_INSTANCE_ID, params["weightA"].get<float>(), params["weightB"].get<float>(), params["outGain"].get<float>())};
            }
            else if (type == "drywet")
            {
                fxMap[name] = {inputs, new FxDescriptorDryWet(FX_INVALID_INSTANCE_ID, FX_INVALID_INSTANCE_ID, params["balance"].get<float>(), params["outGain"].get<float>())};
            }

            // filters and eqs
            else if (type == "hipass1")
            {
                fxMap[name] = {inputs, new FxDescriptorHighPassFilterFirstOrder(FX_INVALID_INSTANCE_ID, params["cutoff"].get<float>())};
            }
            else if (type == "lopass1")
            {
                fxMap[name] = {inputs, new FxDescriptorLowPassFilterFirstOrder(FX_INVALID_INSTANCE_ID, params["cutoff"].get<float>())};
            }
            else
            {
                printf("Invalid effect type found during deserialization: %s\n", type.c_str());
                return false;
            }
        }

        for (auto &fx: chain)
        {
            delete fx;
        }

        chain = std::vector<FxDescriptor *>(fxMap.size());

        int fxIdx = 0;
        for (const auto &[name, p]: fxMap)
        {
            auto inputs   = p.first;
            auto instance = p.second;

            int i = 0;
            for (auto &input: inputs)
            {
                instance->inputs[i++] = input == "IN" ? gFxInputInstanceId : fxMap[input].second->instanceId;
            }

            chain[fxIdx++] = instance;
        }

        optimize();

        return true;
    }

    bool FxChain::serialize(const std::filesystem::path &pFile)
    {
        json pedalSchemeJson;

        optimize();

        std::map<FxInstanceId, std::string> fxInstanceToNameMap = {
            {gFxInputInstanceId, "IN"}
        };

        int i = 0;
        for (auto &fx: chain)
        {
            fxInstanceToNameMap[fx->instanceId] = std::to_string(i++);
        }

        i = 0;
        for (auto &fx: chain)
        {
            json fxInfoJson;

            fxInfoJson["name"] = std::to_string(i++);
            for (auto &fxInput: fx->inputs)
            {
                fxInfoJson["inputs"].push_back(fxInstanceToNameMap[fxInput]);
            }

            auto fxId = fx->getId();

            // amplitude
            if (fxId == FX_ID_GAIN)
            {
                auto params = ((FxDescriptorGain *) fx)->getParams();

                fxInfoJson["type"]   = "gain";
                fxInfoJson["params"] = {
                    {"gain", params->gain}
                };
            }

            // curves
            else if (fxId == FX_ID_BEZIER_CURVE)
            {
                auto params = ((FxDescriptorBezierCurve *) fx)->getParams();

                fxInfoJson["type"]   = "bezier";
                fxInfoJson["params"] = {
                    {"resolution", params->resolution},
                };

                for (auto &point: *params->points)
                {
                    fxInfoJson["params"]["points"].push_back(std::array{point.x, point.y});
                }
            }
            else if (fxId == FX_ID_GENERIC_DIODE_CURVE)
            {
                auto params = ((FxDescriptorGenericDiodeCurve *) fx)->getParams();

                fxInfoJson["type"]   = "diode";
                fxInfoJson["params"] = {
                    {"resolution", params->resolution},
                };
            }

            // mixers
            else if (fxId == FX_ID_DRYWET)
            {
                auto params = ((FxDescriptorDryWet *) fx)->getParams();

                fxInfoJson["type"]   = "drywet";
                fxInfoJson["params"] = {
                    {"balance", params->balance},
                    {"outGain", params->outGain},
                };
            }
            else if (fxId == FX_ID_SUM)
            {
                auto params = ((FxDescriptorSum *) fx)->getParams();

                fxInfoJson["type"]   = "sum";
                fxInfoJson["params"] = {
                    {"weightA", params->weightA},
                    {"weightB", params->weightB},
                    {"outGain", params->outGain},
                };
            }

            // filters and eqs
            else if (fxId == FxId::FX_ID_HIPASS_FIRST_ORDER)
            {
                auto params = ((FxDescriptorHighPassFilterFirstOrder *) fx)->getParams();

                fxInfoJson["type"]   = "hipass1";
                fxInfoJson["params"] = {
                    {"cutoff", params->cutoffFreq},
                };
            }
            else if (fxId == FxId::FX_ID_LOPASS_FIRST_ORDER)
            {
                auto params = ((FxDescriptorLowPassFilterFirstOrder *) fx)->getParams();

                fxInfoJson["type"]   = "lopass1";
                fxInfoJson["params"] = {
                    {"cutoff", params->cutoffFreq},
                };
            }

            else
            {
                printf("Invalid fx id found during serialization: %d\n", fxId);
                return false;
            }

            pedalSchemeJson.push_back(fxInfoJson);
        }

        std::ofstream file(pFile);
        if (!file.is_open())
        {
            return false;
        }

        file << pedalSchemeJson.dump(4);
        file.close();

        return true;
    }
}
