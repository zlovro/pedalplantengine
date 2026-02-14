//
// Created by lovro on 13/02/2026.
// Copyright (c) 2026 lovro. All rights reserved.
//

#include "fx_widgets_xpass.hpp"

#include "audiomath.hpp"

Fx::FxWidgetHipass1::FxWidgetHipass1(FxMainWindow *pParent) : FxWidget(pParent, new FxDescriptorHighPassFilterFirstOrder())
{
    resize(width() * 1.2, std::max(height(), FX_WIDGET_DIAL_SIZE * 2 + FX_WIDGET_DIAL_MARGIN * 2));

    dial = new QDial(this);
    dial->resize(FX_WIDGET_DIAL_SIZE, FX_WIDGET_DIAL_SIZE);
    dial->move((width() - FX_WIDGET_DIAL_SIZE) / 2, FX_WIDGET_DIAL_SIZE * 1.60);
    dial->setValue(getParams()->cutoffFreq);
    dial->setMaximum(22000);

    connect(dial, QDial::valueChanged, this, [this](int pValue)
    {
        getParams()->cutoffFreq = AudioMath::linearToEasedInOutCubic(0, dial->maximum(), pValue);
        update();
    });

    dial->show();
}

void Fx::FxWidgetHipass1::render(QPainter &pPainter)
{
    auto dialY = dial->y();
    pPainter.drawText(QRect{0, FX_WIDGET_DIAL_MARGIN, width(), dialY - FX_WIDGET_DIAL_MARGIN}, Qt::AlignCenter, "HIPASS1");

    auto        dialValue = getParams()->cutoffFreq;
    std::string dialText  = std::format("{:.2f}", dialValue);

    pPainter.drawText(QRect{0, dialY - FX_WIDGET_DIAL_MARGIN, width(), FX_WIDGET_DIAL_MARGIN}, Qt::AlignCenter, QString::fromStdString(dialText));
}

Fx::FxDescriptorHighPassFilterFirstOrder::FxParamsHighPassFirstOrder *Fx::FxWidgetHipass1::getParams() const
{
    return (FxDescriptorHighPassFilterFirstOrder::FxParamsHighPassFirstOrder *) fxDsc->params;
}

Fx::FxWidgetHipass1::~FxWidgetHipass1()
{
}
