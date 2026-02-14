//
// Created by lovro on 08/02/2026.
// Copyright (c) 2026 lovro. All rights reserved.
//

#include "fx_widgets_amplitude.hpp"

#include "audiomath.hpp"

namespace Fx
{
    FxWidgetGain::FxWidgetGain(FxMainWindow *pParent): FxWidget(pParent, new FxDescriptorGain())
    {
        dial = new QDial(this);
        dial->resize(FX_WIDGET_DIAL_SIZE, FX_WIDGET_DIAL_SIZE);
        dial->move((width() - FX_WIDGET_DIAL_SIZE) / 2, FX_WIDGET_DIAL_SIZE * 1.60);
        dial->setValue(getParams()->gain);
        dial->setMaximum(100);

        connect(dial, QDial::valueChanged, this, [this](int pValue)
        {
            if (getParams()->displayUnit == LEVEL_UNIT_PERCENTAGE)
            {
                getParams()->gain = AudioMath::linearToEasedInOutCubic(0, dial->maximum(), pValue) / dial->maximum();
            }
            else
            {
                getParams()->gain = AudioMath::linearToEasedInOutCubic(0, dial->maximum(), pValue);
            }
            update();
        });

        dial->show();

        resize(width(), std::max(height(), FX_WIDGET_DIAL_SIZE * 2 + FX_WIDGET_DIAL_MARGIN * 2));
    }

    void FxWidgetGain::render(QPainter &pPainter)
    {
        auto unit = getParams()->displayUnit;

        auto dialY = dial->y();
        pPainter.drawText(QRect{0, 0, width(), dialY - FX_WIDGET_DIAL_MARGIN}, Qt::AlignCenter, unit == LEVEL_UNIT_PERCENTAGE ? "LEVEL" : "GAIN");

        auto        dialValue = getParams()->gain;
        std::string dialText;

        switch (unit)
        {
            case LEVEL_UNIT_LINEAR:
                dialText = std::format("{:.2f}", dialValue);
                break;
            case LEVEL_UNIT_PERCENTAGE:
                dialText = std::format("{:.2f}%", 100 * dialValue);
                break;
            case LEVEL_UNIT_DB:
                dialText = std::format("{:.2f}", dialValue / 100.0F);
                break;
        }

        pPainter.drawText(QRect{0, dialY - FX_WIDGET_DIAL_MARGIN, width(), FX_WIDGET_DIAL_MARGIN}, Qt::AlignCenter, QString::fromStdString(dialText));
    }

    FxDescriptorGain::FxParamsGain *FxWidgetGain::getParams() const
    {
        return ((FxDescriptorGain *) fxDsc)->getParams();
    }

    FxWidgetGain::~FxWidgetGain()
    {
        delete fxDsc;
    }
}
