//
// Created by lovro on 08/02/2026.
// Copyright (c) 2026 lovro. All rights reserved.
//

#include "fx_widgets_amplitude.hpp"

namespace Fx::Gfx
{
    FxGfxFxWidgetGain::FxGfxFxWidgetGain(FxGfxMainWindow *pParent): FxGfxFxWidget(pParent)
    {
        fxDsc = new FxDescriptorGain(FX_INVALID_INSTANCE_ID);
    }

    void FxGfxFxWidgetGain::render(QPainter &pPainter)
    {
        pPainter.drawText(rect(), Qt::AlignCenter, "GAIN");
    }

    FxGfxFxWidgetGain::~FxGfxFxWidgetGain()
    {
        delete fxDsc;
    }
}
