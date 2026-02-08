//
// Created by lovro on 08/02/2026.
// Copyright (c) 2026 lovro. All rights reserved.
//

#ifndef FX_WIDGETS_AMPLITUDE_HPP
#define FX_WIDGETS_AMPLITUDE_HPP

#include <fx_gfx.hpp>
#include <fx/fx_amplitude.hpp>

namespace Fx::Gfx
{
    class FxGfxFxWidgetGain : public FxGfxFxWidget
    {
        public:
        FxGfxFxWidgetGain(FxGfxMainWindow *pParent);

        void render(QPainter &pPainter) override;

        ~FxGfxFxWidgetGain();
    };
}

#endif //FX_WIDGETS_AMPLITUDE_HPP
