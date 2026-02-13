//
// Created by lovro on 08/02/2026.
// Copyright (c) 2026 lovro. All rights reserved.
//

#include "fx_widgets_amplitude.hpp"

namespace Fx::Gfx
{
    FxGfxFxWidgetGain::FxGfxFxWidgetGain(FxGfxMainWindow *pParent): FxGfxFxWidget(pParent)
    {
        fxDsc = new FxDescriptorGain();

        dial = new QDial(this);
        dial->resize(FX_WIDGET_DIAL_SIZE, FX_WIDGET_DIAL_SIZE);
        dial->move((width() - FX_WIDGET_DIAL_SIZE) / 2, FX_WIDGET_DIAL_SIZE * 1.60);
        dial->setValue(getParams()->gain);
        dial->setMaximum(100);

        connect(dial, QDial::valueChanged, this, [this](int pValue)
        {
            getParams()->gain = (1.0F - std::logf(dial->maximum() - pValue + 1) / std::logf(dial->maximum() + 1)) * dial->maximum();
            update();
        });

        dial->show();

        resize(width(), std::max(height(), FX_WIDGET_DIAL_SIZE * 2 + FX_WIDGET_DIAL_MARGIN * 2));
    }

    void FxGfxFxWidgetGain::render(QPainter &pPainter)
    {
        auto dialY = dial->y();
        pPainter.drawText(QRect{0, 0, width(), dialY - FX_WIDGET_DIAL_MARGIN}, Qt::AlignCenter, "GAIN");

        auto        dialValue = getParams()->gain;
        std::string dialText;

        switch (getParams()->displayUnit)
        {
            case LEVEL_UNIT_LINEAR:
                dialText = std::format("{:.2f}", dialValue);
                break;
            case LEVEL_UNIT_PERCENTAGE:
                dialText = std::format("{:.2f}%", (float) dialValue);
                break;
            case LEVEL_UNIT_DB:
                dialText = std::format("{:.2f}", dialValue / 100.0F);
                break;
        }

        pPainter.drawText(QRect{0, dialY - FX_WIDGET_DIAL_MARGIN, width(), FX_WIDGET_DIAL_MARGIN}, Qt::AlignCenter, QString::fromStdString(dialText));
    }

    FxDescriptorGain::FxParamsGain *FxGfxFxWidgetGain::getParams() const
    {
        return ((FxDescriptorGain *) fxDsc)->getParams();
    }

    FxGfxFxWidgetGain::~FxGfxFxWidgetGain()
    {
        delete fxDsc;
    }
}
