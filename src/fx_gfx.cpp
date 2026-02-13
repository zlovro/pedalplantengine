//
// Created by lovro on 07/02/2026.
// Copyright (c) 2026 lovro. All rights reserved.
//

#include "fx_gfx.hpp"

#include <QMenu>
#include <QPoint>
#include <QTextItem>
#include <QWidget>
#include <QMouseEvent>
#include <QApplication>

#include <cmath>

#include <fx/widgets/fx_widgets_amplitude.hpp>

#include "fx/widgets/fx_widgets_xpass.hpp"

namespace Fx::Gfx
{
    const char *  FX_FONT_FAMILY     = "Consolas";
    constexpr int FX_FONT_SIZE       = 12;
    constexpr int FX_FONT_SIZE_SMALL = 8;

    const QFont FX_DEFAULT_FONT(FX_FONT_FAMILY, FX_FONT_SIZE, QFont::Normal);
    const QFont FX_BOLD_FONT(FX_FONT_FAMILY, FX_FONT_SIZE, QFont::Bold);
    const QFont FX_BOLD_FONT_SMALL(FX_FONT_FAMILY, FX_FONT_SIZE_SMALL, QFont::Bold);

    extern const QColor FX_WIDGET_FILL_COLOR(140, 140, 140, 140);

    extern const QColor FX_WIDGET_OUTLINE_COLOR(0, 0, 0);
    extern const int    FX_WIDGET_OUTLINE_RADIUS = 7;
    extern const int    FX_WIDGET_OUTLINE_WIDTH  = 7;

    extern const QColor FX_WIDGET_UNCONNECTED_POINT_COLOR(255, 0, 0, 255);
    extern const QColor FX_WIDGET_CONNECTED_INPUT_COLOR(0, 240, 0, 120);
    extern const QColor FX_WIDGET_CONNECTED_OUTPUT_COLOR(0, 0, 240, 120);
    extern const QColor FX_WIDGET_IGNORE_POINT_COLOR(120, 120, 120, 120);
    extern const int    FX_WIDGET_CONNECTOR_RADIUS_SIDES    = 25;
    extern const int    FX_WIDGET_CONNECTOR_RADIUS_VERTICAL = 25;
    extern const int    FX_WIDGET_CONNECTOR_RADIUS_BORDER   = 5;

    extern const int FX_WIDGET_DEFAULT_WIDTH  = 120;
    extern const int FX_WIDGET_DEFAULT_HEIGHT = 80;

    FxGfxMainWindow::ConnectingLine::ConnectingLine(FxGfxFxWidget::ConnectorPoint *pA, FxGfxFxWidget::ConnectorPoint *pB)
    {
        a = pA;
        b = pB;
    }

    FxGfxMainWindow *FxGfxMainWindow::instance = nullptr;

    FxGfxMainWindow::FxGfxMainWindow() : QMainWindow()
    {
        instance = this;

        isDrawing              = false;
        drawingOriginConnector = nullptr;

        resize(1280, 720);

        this->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(this, customContextMenuRequested, this, showCtxMenu);

        auto inWidget = new FxGfxFxWidgetInput(this);
        inWidget->show();

        auto outWidget = new FxGfxFxWidgetOutput(this);
        outWidget->show();

        setMouseTracking(true);
    }

    FxGfxMainWindow::~FxGfxMainWindow()
    {
        for (auto &x: connectingLines)
        {
            delete x;
        }

        connectingLines.clear();
    }

    void FxGfxMainWindow::startDrawing(FxGfxFxWidget::ConnectorPoint *pSourceConnector, const QPen &pPen)
    {
        isDrawing              = true;
        drawingOriginConnector = pSourceConnector;
        drawingPen             = pPen;

        drawingPen.setCapStyle(Qt::RoundCap);
    }

    void FxGfxMainWindow::cancelDrawing()
    {
        isDrawing              = false;
        drawingOriginConnector = nullptr;
        update();
    }

    std::vector<FxGfxMainWindow::ConnectingLine *> FxGfxMainWindow::getLinesOnConnector(const FxGfxFxWidget::ConnectorPoint *pPoint) const
    {
        std::vector<ConnectingLine *> list;
        for (auto x: connectingLines)
        {
            if (x->a == pPoint || x->b == pPoint)
            {
                list.push_back(x);
            }
        }

        return list;
    }

    bool FxGfxMainWindow::canConnectFx(FxGfxFxWidget::ConnectorPoint *pSrc, FxGfxFxWidget::ConnectorPoint *pDst)
    {
        bool srcAsIn = pSrc->type == FxGfxFxWidget::CONN_INPUT && pDst->type == FxGfxFxWidget::CONN_OUTPUT;
        bool dstAsIn = pSrc->type == FxGfxFxWidget::CONN_OUTPUT && pDst->type == FxGfxFxWidget::CONN_INPUT;

        if (srcAsIn || dstAsIn)
        {
            auto in  = srcAsIn ? pSrc : pDst;
            auto out = srcAsIn ? pDst : pSrc;

            if (!getLinesOnConnector(in).empty())
            {
                return false;
            }

            return true;
        }

        return false;
    }

    void FxGfxMainWindow::connectFx(FxGfxFxWidget::ConnectorPoint *pSrc, FxGfxFxWidget::ConnectorPoint *pDst)
    {
        FxGfxFxWidget::ConnectorPoint *in, *out;
        if (pSrc->type == FxGfxFxWidget::CONN_INPUT)
        {
            in  = pSrc;
            out = pDst;
        }
        else
        {
            in  = pDst;
            out = pSrc;
        }

        auto line = new ConnectingLine(pSrc, pDst);

        if (in->parent->widgetType != FxGfxFxWidget::WIDGET_OUTPUT)
        {
            auto inParent = in->parent->fxDsc->instanceId;
            gFxChain.fxIdToFxMap[inParent]->inputs.push_back(out->origin);
        }
        in->origin = out->origin;

        connectingLines.push_back(line);

        gFxChain.optimize();

        isDrawing = false;
    }

    QPoint FxGfxMainWindow::getCenter() const
    {
        return {size().width() / 2, size().height() / 2};
    }

    // rendering, gfx, gui code
    FxGfxFxWidget::FxGfxFxWidget(FxGfxMainWindow *pParent)
    {
        setParent(pParent);

        this->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(this, customContextMenuRequested, this, showCtxMenu);

        justAdded           = true;
        widgetType          = WIDGET_REGULAR;
        wndParent           = pParent;
        currentConnectorIdx = -1;
        fxDsc               = nullptr;
        connectors          = std::array<ConnectorPoint, 4>();

        for (auto &connector: connectors)
        {
            connector.parent = this;
            connector.type   = CONN_UNCONNECTED;
        }

        setMouseTracking(true);

        resize(FX_WIDGET_DEFAULT_WIDTH, FX_WIDGET_DEFAULT_HEIGHT);

        mLastMousePos = QCursor::pos();
        auto cursor   = wndParent->mapFromGlobal(mLastMousePos);
        move(cursor.x() - width() / 2, cursor.y() - height() / 2);

        setCursor(Qt::SizeAllCursor);
    }

    void FxGfxMainWindow::paintEvent(QPaintEvent *event)
    {
        QMainWindow::paintEvent(event);

        auto painter = QPainter(this);

        for (auto x: connectingLines)
        {
            auto p1 = x->a->globalRect.center();
            auto p4 = x->b->globalRect.center();
            auto p3 = QPoint{(p1.x() + p4.x()) / 2, p4.y()};
            auto p2 = QPoint{p3.x(), p1.y()};

            drawBezier(painter, std::vector{p1, p2, p3, p4}, drawingPen);
        }

        if (isDrawing)
        {
            auto p1 = drawingOriginConnector->globalRect.center();
            auto p4 = mapFromGlobal(QCursor::pos());
            auto p3 = QPoint{(p1.x() + p4.x()) / 2, p4.y()};
            auto p2 = QPoint{p3.x(), p1.y()};

            drawBezier(painter, std::vector{p1, p2, p3, p4}, drawingPen);
        }
    }

    void FxGfxMainWindow::mouseMoveEvent(QMouseEvent *event)
    {
        QMainWindow::mouseMoveEvent(event);

        if (isDrawing)
        {
            update();
        }
    }

    void FxGfxMainWindow::mousePressEvent(QMouseEvent *event)
    {
        QMainWindow::mousePressEvent(event);

        auto isLeftClick  = event->button() == Qt::LeftButton;
        auto isRightClick = event->button() == Qt::RightButton;

        if (isDrawing && isRightClick)
        {
            cancelDrawing();
        }
    }

    void FxGfxMainWindow::keyReleaseEvent(QKeyEvent *event)
    {
        QMainWindow::keyReleaseEvent(event);

        if (isDrawing && event->key() == Qt::Key_Escape)
        {
            cancelDrawing();
        }
    }

    void drawBezier(QPainter &pPainter, const std::vector<QPoint> &pPoints, const QPen &pPen)
    {
        QPoint previousPoint = pPoints[0];
        QPoint finalPoint    = pPoints[pPoints.size() - 1];

        pPainter.setPen(pPen);

        // how many lines fit in one height or width of the window
        constexpr int LINE_DENSITY  = 50;
        int           segmentLength = std::min(FxGfxMainWindow::instance->width(), FxGfxMainWindow::instance->height()) / LINE_DENSITY;

        auto da           = finalPoint.x() - previousPoint.x();
        auto db           = finalPoint.y() - previousPoint.y();
        auto approxLength = std::sqrtf(da * da + db * db);

        int segments = std::ceil(approxLength / segmentLength);

        auto step = 1.0F / (segments - 1);
        auto t    = step;
        auto n    = pPoints.size();

        for (int i = 1; i < segments; i++, t += step)
        {
            QPoint point;

            if (i == segments - 1)
            {
                point = finalPoint;
            }
            else
            {
                std::vector points(pPoints);

                for (int j = 0; j < n - 1; j++)
                {
                    for (int k = 1; k < n - j; k++)
                    {
                        auto a = points[k - 1];
                        auto b = points[k];

                        points[k - 1] = QPoint{(int) std::lerp(a.x(), b.x(), t), (int) std::lerp(a.y(), b.y(), t)};
                    }
                }

                point = points[0];
            }

            pPainter.drawLine(previousPoint, point);
            previousPoint = point;
        }
    }

    void FxGfxFxWidget::showCtxMenu(QPoint pPoint)
    {
        if (currentConnectorIdx != -1)
        {
            QMenu contextMenu("Action", this);

            auto actionGroup = QActionGroup(parent());
            actionGroup.setExclusive(true);

            auto type = connectors[currentConnectorIdx].type;

            auto actionInput = contextMenu.addAction("Input");
            actionInput->setEnabled(widgetType != WIDGET_INPUT);
            actionInput->setCheckable(true);
            actionInput->setChecked(type == CONN_INPUT);
            connect(actionInput, QAction::triggered, this, onCtxMenuItemChecked);
            actionGroup.addAction(actionInput);

            auto actionOutput = contextMenu.addAction("Output");
            actionOutput->setEnabled(widgetType != WIDGET_OUTPUT);
            actionOutput->setCheckable(true);
            actionOutput->setChecked(type == CONN_OUTPUT);
            connect(actionOutput, QAction::triggered, this, onCtxMenuItemChecked);
            actionGroup.addAction(actionOutput);

            auto actionUnconnected = contextMenu.addAction("Unconnected");
            actionUnconnected->setCheckable(true);
            actionUnconnected->setChecked(type == CONN_UNCONNECTED);
            connect(actionUnconnected, QAction::triggered, this, onCtxMenuItemChecked);
            actionGroup.addAction(actionUnconnected);

            contextMenu.exec(mapToGlobal(pPoint));
        }
    }

    void FxGfxFxWidget::onCtxMenuItemChecked()
    {
        auto          oldType = connectors[currentConnectorIdx].type;
        ConnectorType newType = CONN_INVALID;

        for (auto &action: findChild<QMenu *>()->actions())
        {
            if (!action->isChecked())
            {
                continue;
            }

            auto txt = action->text();

            if (txt == "Input")
            {
                newType = CONN_INPUT;
            }
            else if (txt == "Output")
            {
                newType = CONN_OUTPUT;
            }
            else
            {
                newType = CONN_UNCONNECTED;
            }
            break;
        }

        // ensure there is enough room for inputs, otherwise quit
        if (newType == CONN_OUTPUT && fxDsc)
        {
            auto remaining = 3;
            for (int i = 0; i < 4; ++i)
            {
                if (i != currentConnectorIdx && connectors[i].type == CONN_OUTPUT)
                {
                    remaining--;
                }
            }

            if (remaining < 1)
            {
                newType = oldType;
                goto end;
            }
        }

        if (newType == CONN_INPUT)
        {
            connectors[currentConnectorIdx].origin = FX_INVALID_INSTANCE_ID;
        }
        else
        {
            if (widgetType == WIDGET_INPUT)
            {
                connectors[currentConnectorIdx].origin = gFxInputInstanceId;
            }
            else if (widgetType == WIDGET_OUTPUT)
            {
            }
            else
            {
                connectors[currentConnectorIdx].origin = fxDsc->instanceId;
            }
        }

        // fxDsc will be null when widget is IN or OUT
        if (newType == CONN_INPUT && fxDsc)
        {
            for (int i = 0, inputCount = 1; i < 4; ++i)
            {
                if (i == currentConnectorIdx)
                {
                    continue;
                }

                if (connectors[i].type == CONN_INPUT)
                {
                    inputCount++;
                }

                if (inputCount > fxDsc->getExpectedInputCount())
                {
                    disconnectConnector(&connectors[i]);
                    inputCount--;
                }
            }
        }

        if (newType == CONN_UNCONNECTED)
        {
            disconnectConnector(&connectors[currentConnectorIdx]);
        }

    end:
        connectors[currentConnectorIdx].type = newType;
        wndParent->update();
        update();
    }

    void FxGfxFxWidget::disconnectConnector(ConnectorPoint *pPoint) const
    {
        pPoint->type   = CONN_UNCONNECTED;
        pPoint->origin = FX_INVALID_INSTANCE_ID;

        auto lines = &wndParent->connectingLines;
        for (int i = 0; i < lines->size(); i++)
        {
            if (lines->at(i)->a == pPoint || lines->at(i)->b == pPoint)
            {
                lines->erase(lines->begin() + i);
                break;
            }
        }

        gFxChain.optimize();
    }

    void FxGfxFxWidget::resizeEvent(QResizeEvent *event)
    {
        QWidget::resizeEvent(event);

        auto cx = width() / 2;
        auto cy = height() / 2;

        // top centre
        connectors[0].localRect = {cx - FX_WIDGET_CONNECTOR_RADIUS_SIDES, 0, FX_WIDGET_CONNECTOR_RADIUS_SIDES * 2, FX_WIDGET_CONNECTOR_RADIUS_VERTICAL};

        // bottom centre
        connectors[1].localRect = {cx - FX_WIDGET_CONNECTOR_RADIUS_SIDES, height() - FX_WIDGET_CONNECTOR_RADIUS_VERTICAL, FX_WIDGET_CONNECTOR_RADIUS_SIDES * 2, FX_WIDGET_CONNECTOR_RADIUS_VERTICAL};

        // right centre
        connectors[2].localRect = {width() - FX_WIDGET_CONNECTOR_RADIUS_VERTICAL, cy - FX_WIDGET_CONNECTOR_RADIUS_SIDES, FX_WIDGET_CONNECTOR_RADIUS_VERTICAL, FX_WIDGET_CONNECTOR_RADIUS_SIDES * 2};

        // left centre
        connectors[3].localRect = {0, cy - FX_WIDGET_CONNECTOR_RADIUS_SIDES, FX_WIDGET_CONNECTOR_RADIUS_VERTICAL, FX_WIDGET_CONNECTOR_RADIUS_SIDES * 2};

        updateConnectorPositions();
    }

    void FxGfxFxWidget::moveEvent(QMoveEvent *event)
    {
        QWidget::moveEvent(event);

        updateConnectorPositions();
        wndParent->update();
    }

    void FxGfxFxWidget::updateConnectorPositions()
    {
        for (auto &conn: connectors)
        {
            conn.globalRect = QRect{mapToParent(conn.localRect.topLeft()), conn.localRect.size()};
        }
    }

    void FxGfxFxWidget::paintEvent(QPaintEvent *event)
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        QPainterPath path;
        path.addRoundedRect(0, 0, size().width(), size().height(), FX_WIDGET_OUTLINE_RADIUS, FX_WIDGET_OUTLINE_RADIUS);

        painter.setClipPath(path);
        painter.fillPath(path, FX_WIDGET_FILL_COLOR);
        painter.strokePath(path, QPen(Qt::black, FX_WIDGET_OUTLINE_WIDTH));

        painter.setPen(QPen(Qt::black, FX_WIDGET_OUTLINE_WIDTH * 0.5F));

        for (auto &conn: connectors)
        {
            QString text;
            QColor  color;

            switch (conn.type)
            {
                case CONN_INPUT:
                    text = "I";
                    color = FX_WIDGET_CONNECTED_INPUT_COLOR;
                    break;
                case CONN_OUTPUT:
                    text = "O";
                    color = FX_WIDGET_CONNECTED_OUTPUT_COLOR;
                    break;
                case CONN_UNCONNECTED:
                    text = "X";
                    color = FX_WIDGET_UNCONNECTED_POINT_COLOR;
                    break;
                case CONN_TYPE_COUNT:
                    break;
                default: ;
            }

            painter.setBrush(color);
            painter.drawRoundedRect(conn.localRect, FX_WIDGET_CONNECTOR_RADIUS_BORDER, FX_WIDGET_CONNECTOR_RADIUS_BORDER);

            painter.setBrush(Qt::black);
            painter.setFont(FX_BOLD_FONT);
            painter.drawText(conn.localRect, Qt::AlignCenter, text);
        }

        painter.setBrush(Qt::black);

        painter.setFont(FX_BOLD_FONT);
        render(painter);
    }

    void FxGfxFxWidget::mouseMoveEvent(QMouseEvent *event)
    {
        QWidget::mouseMoveEvent(event);

        auto leftClickDown = event->buttons() == Qt::LeftButton;
        if ((leftClickDown && currentConnectorIdx < 0) || justAdded)
        {
            auto evtPos = event->globalPos();
            auto delta  = evtPos - mLastMousePos;
            move(pos() + delta);
            mLastMousePos = evtPos;
            return;
        }

        if (!leftClickDown && !justAdded)
        {
            currentConnectorIdx = -1;
            setCursor(Qt::SizeAllCursor);

            auto cursorLocal = mapFromGlobal(QCursor::pos());

            int i = 0;
            for (auto &conn: connectors)
            {
                if (conn.localRect.contains(cursorLocal))
                {
                    currentConnectorIdx = i;

                    if (conn.type != CONN_UNCONNECTED)
                    {
                        setCursor(Qt::CrossCursor);
                    }
                    else
                    {
                        setCursor(Qt::ArrowCursor);
                    }
                }

                i++;
            }
        }
    }

    void FxGfxFxWidget::mousePressEvent(QMouseEvent *event)
    {
        QWidget::mousePressEvent(event);

        if (justAdded)
        {
            justAdded = false;
        }

        auto isLeftClick  = event->button() == Qt::LeftButton;
        auto isRightClick = event->button() == Qt::RightButton;

        auto hoveringOverConnector = currentConnectorIdx != -1;

        if (wndParent->isDrawing && isLeftClick && hoveringOverConnector && wndParent->canConnectFx(wndParent->drawingOriginConnector, &connectors[currentConnectorIdx]))
        {
            wndParent->connectFx(wndParent->drawingOriginConnector, &connectors[currentConnectorIdx]);
            update();
        }
        else
        {
            bool ctxMenuOpen = findChild<QMenu *>();
            if (!ctxMenuOpen && isLeftClick && currentConnectorIdx != -1 && connectors[currentConnectorIdx].type != CONN_UNCONNECTED)
            {
                wndParent->startDrawing(&connectors[currentConnectorIdx], QPen(Qt::black, 3));
                return;
            }
        }

        mLastMousePos = event->globalPos();
    }

    void FxGfxFxWidget::leaveEvent(QEvent *event)
    {
        QWidget::leaveEvent(event);

        setCursor(Qt::ArrowCursor);
    }

    FxGfxFxWidget::ConnectorPoint::ConnectorPoint(FxGfxFxWidget *pParent)
    {
        parent = pParent;
        type   = CONN_UNCONNECTED;
        origin = FX_INVALID_INSTANCE_ID;
    }

    QPoint FxGfxFxWidget::getGlobalCentre() const
    {
        return QPoint{x() + width() / 2, y() + height() / 2};
    }

    QPoint FxGfxFxWidget::getLocalCentre() const
    {
        return QPoint{width() / 2, height() / 2};
    }

    void FxGfxFxWidget::moveToCentre()
    {
        move(wndParent->getCenter().x() - width() / 2, wndParent->getCenter().y() - height() / 2);
    }

    // input widget

    FxGfxFxWidgetInput *FxGfxFxWidgetInput::instance = nullptr;

    FxGfxFxWidgetInput::FxGfxFxWidgetInput(FxGfxMainWindow *pParent): FxGfxFxWidget(pParent)
    {
        instance = this;

        setCursor(Qt::ArrowCursor);
        widgetType = WIDGET_INPUT;
        justAdded  = false;
        moveToCentre();

        for (auto &x: connectors)
        {
            x.origin = gFxInputInstanceId;
        }
    }

    void FxGfxFxWidgetInput::render(QPainter &pPainter)
    {
        pPainter.drawText(rect(), Qt::AlignCenter, "IN");
    }

    FxGfxFxWidgetInput::~FxGfxFxWidgetInput()
    = default;

    // output widget

    FxGfxFxWidgetOutput *FxGfxFxWidgetOutput::instance = nullptr;

    FxGfxFxWidgetOutput::FxGfxFxWidgetOutput(FxGfxMainWindow *pParent) : FxGfxFxWidget(pParent)
    {
        instance = this;

        setCursor(Qt::ArrowCursor);
        widgetType = WIDGET_OUTPUT;
        justAdded  = false;
        moveToCentre();
        move(x() + FX_WIDGET_DEFAULT_WIDTH * 3, y());

        for (auto &x: connectors)
        {
            x.origin = FX_INVALID_INSTANCE_ID;
        }
    }

    void FxGfxFxWidgetOutput::render(QPainter &pPainter)
    {
        pPainter.drawText(rect(), Qt::AlignCenter, "OUT");
    }

    FxGfxFxWidgetOutput::~FxGfxFxWidgetOutput()
    = default;

    // gui actions

    void FxGfxMainWindow::showCtxMenu(QPoint pPoint)
    {
        QMenu contextMenu("Action", this);

        auto subMenuAddFx = new QMenu("Add new effect");

        subMenuAddFx->addSection("Amplitude & Gain");
        subMenuAddFx->addSeparator();
        addActionSimple(subMenuAddFx, "Gain", addGain);

        subMenuAddFx->addSection("Curves");
        subMenuAddFx->addSeparator();
        addActionSimple(subMenuAddFx, "Bezier", addBezier);
        addActionSimple(subMenuAddFx, "Diode", addDiode);

        subMenuAddFx->addSection("Mixers");
        subMenuAddFx->addSeparator();
        addActionSimple(subMenuAddFx, "Sum", addSum);
        addActionSimple(subMenuAddFx, "Dry/Wet", addDryWet);

        subMenuAddFx->addSection("Filters & EQs");
        subMenuAddFx->addSeparator();
        addActionSimple(subMenuAddFx, "Low pass (1st order)", addLopass1);
        addActionSimple(subMenuAddFx, "High pass (1st order)", addHipass1);

        contextMenu.addMenu(subMenuAddFx);

        contextMenu.exec(mapToGlobal(pPoint));
    }

    void FxGfxMainWindow::addActionSimple(QMenu *pMenu, const QString &pLabel, void (FxGfxMainWindow::*pFunc)()) const // NOLINT(*-convert-member-functions-to-static)
    {
        auto action = pMenu->addAction(pLabel);
        connect(action, QAction::triggered, this, pFunc);
    }

    void FxGfxMainWindow::addGain()
    {
        auto widget = new FxGfxFxWidgetGain(this);
        widget->show();

        gFxChain.addFxNoOptimize(widget->fxDsc);
    }

    void FxGfxMainWindow::addBezier()
    {
    }

    void FxGfxMainWindow::addDiode()
    {
    }

    void FxGfxMainWindow::addSum()
    {
    }

    void FxGfxMainWindow::addDryWet()
    {
    }

    void FxGfxMainWindow::addLopass1()
    {
    }

    void FxGfxMainWindow::addHipass1()
    {
        auto widget = new FxGfxFxWidgetHighPass1(this);
        widget->show();

        gFxChain.addFxNoOptimize(widget->fxDsc);
    }
}
