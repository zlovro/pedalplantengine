//
// Created by lovro on 02/02/2026.
// Copyright (c) 2026 lovro. All rights reserved.
//

#include <cstdlib>

#include <fx/fx.hpp>

FxDescriptor::FxDescriptor()
{
    instanceId = rand();

    processor = nullptr;
    params    = nullptr;
}

FxId FxDescriptor::getId()
{
    return FX_ID_NULL;
}

const char *FxDescriptor::getName()
{
    return nullptr;
}
