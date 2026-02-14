//
// Created by lovro on 13/02/2026.
// Copyright (c) 2026 lovro. All rights reserved.
//

#ifndef FX_WIDGETS_XPASS_HPP
#define FX_WIDGETS_XPASS_HPP

#include <fx_gfx.hpp>
#include <fx/fx_amplitude.hpp>

#include <QDial>

#include "fx/fx_xpass.hpp"

namespace Fx
{
    class FxWidgetHipass1 : public FxWidget
    {
        public:
        QDial* dial;

        explicit FxWidgetHipass1(FxMainWindow *pParent);

        void render(QPainter &pPainter) override;
        FxDescriptorHighPassFilterFirstOrder::FxParamsHighPassFirstOrder* getParams() const;

        ~FxWidgetHipass1();
    };
}


#endif //FX_WIDGETS_XPASS_HPP
