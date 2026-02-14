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

namespace Fx
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

    FxMainWindow::ConnectingLine::ConnectingLine(FxWidget::ConnectorPoint *pA, FxWidget::ConnectorPoint *pB)
    {
        a = pA;
        b = pB;
    }

    FxMainWindow *FxMainWindow::instance = nullptr;

    FxMainWindow::FxMainWindow(const QPen &pDefaultPen) : QMainWindow()
    {
        instance = this;

        defaultPen = pDefaultPen;

        isDrawing              = false;
        drawingOriginConnector = nullptr;

        resize(1280, 720);

        this->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(this, customContextMenuRequested, this, showCtxMenu);

        auto inWidget = new FxWidgetIn(this);
        inWidget->show();

        auto outWidget = new FxWidgetOut(this);
        outWidget->show();

        setMouseTracking(true);

        gEventMainWindowAfterInit.trigger();
    }

    FxMainWindow::~FxMainWindow()
    {
        for (auto &x: connectingLines)
        {
            delete x;
        }

        connectingLines.clear();
    }

    void FxMainWindow::startDrawing(FxWidget::ConnectorPoint *pSourceConnector, const QPen &pPen)
    {
        isDrawing              = true;
        drawingOriginConnector = pSourceConnector;
        drawingPen             = pPen;

        drawingPen.setCapStyle(Qt::RoundCap);
    }

    void FxMainWindow::cancelDrawing()
    {
        isDrawing              = false;
        drawingOriginConnector = nullptr;
        update();
    }

    std::vector<FxMainWindow::ConnectingLine *> FxMainWindow::getLinesOnConnector(const FxWidget::ConnectorPoint *pPoint) const
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

    bool FxMainWindow::canConnectFx(FxWidget::ConnectorPoint *pSrc, FxWidget::ConnectorPoint *pDst) const
    {
        bool srcAsIn = pSrc->type == FxWidget::CONN_INPUT && pDst->type == FxWidget::CONN_OUTPUT;
        bool dstAsIn = pSrc->type == FxWidget::CONN_OUTPUT && pDst->type == FxWidget::CONN_INPUT;

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

    void FxMainWindow::connectFx(FxWidget::ConnectorPoint *pSrc, FxWidget::ConnectorPoint *pDst)
    {
        FxWidget::ConnectorPoint *in, *out;
        if (pSrc->type == FxWidget::CONN_INPUT)
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

        if (in->parent->widgetType != FxWidget::WIDGET_OUTPUT)
        {
            auto inParent = in->parent->fxDsc->instanceId;
            gFxChain.fxIdToFxMap[inParent]->inputs.push_back(out->origin);
        }
        in->origin = out->origin;

        connectingLines.push_back(line);

        gFxChain.optimize();

        isDrawing = false;
    }

    QPoint FxMainWindow::getCenter() const
    {
        return {size().width() / 2, size().height() / 2};
    }

    // rendering, gfx, gui code
    std::map<FxInstanceId, FxWidget *> FxWidget::fxIdToWidgetMap = std::map<FxInstanceId, FxWidget *>();

    FxWidget::FxWidget(FxMainWindow *pParent, FxDescriptor *pDsc = nullptr)
    {
        if ((fxDsc = pDsc))
        {
            fxIdToWidgetMap[pDsc->instanceId] = this;
        }
        setParent(pParent);

        this->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(this, customContextMenuRequested, this, showCtxMenu);

        justAdded           = true;
        moveAroundOnSpawn   = true;
        widgetType          = WIDGET_REGULAR;
        wndParent           = pParent;
        currentConnectorIdx = -1;
        connectors          = std::array<ConnectorPoint, 4>();

        for (auto &connector: connectors)
        {
            connector.parent = this;
            connector.type   = CONN_UNCONNECTED;
        }

        connectorRight  = connectors.data() + 0;
        connectorLeft   = connectors.data() + 1;
        connectorTop    = connectors.data() + 2;
        connectorBottom = connectors.data() + 3;

        setMouseTracking(true);

        resize(FX_WIDGET_DEFAULT_WIDTH, FX_WIDGET_DEFAULT_HEIGHT);

        mLastMousePos = QCursor::pos();
        auto cursor   = wndParent->mapFromGlobal(mLastMousePos);
        move(cursor.x() - width() / 2, cursor.y() - height() / 2);

        setCursor(Qt::SizeAllCursor);

        updateConnectorLocalRects();
        updateConnectorPositions();
    }

    void FxWidget::render(QPainter &pPainter)
    {
    }

    void FxMainWindow::paintEvent(QPaintEvent *event)
    {
        QMainWindow::paintEvent(event);

        auto painter = QPainter(this);

        for (auto x: connectingLines)
        {
            auto p1 = x->a->globalRect.center();
            auto p4 = x->b->globalRect.center();
            auto p3 = QPoint{(p1.x() + p4.x()) / 2, p4.y()};
            auto p2 = QPoint{p3.x(), p1.y()};

            drawBezier(painter, std::vector{p1, p2, p3, p4}, defaultPen);
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

    void FxMainWindow::mouseMoveEvent(QMouseEvent *event)
    {
        QMainWindow::mouseMoveEvent(event);

        if (isDrawing)
        {
            update();
        }
    }

    void FxMainWindow::mousePressEvent(QMouseEvent *event)
    {
        QMainWindow::mousePressEvent(event);

        auto isLeftClick  = event->button() == Qt::LeftButton;
        auto isRightClick = event->button() == Qt::RightButton;

        if (isDrawing && isRightClick)
        {
            cancelDrawing();
        }
    }

    void FxMainWindow::keyReleaseEvent(QKeyEvent *event)
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
        int           segmentLength = std::min(FxMainWindow::instance->width(), FxMainWindow::instance->height()) / LINE_DENSITY;

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

    void FxWidget::showCtxMenu(QPoint pPoint)
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

    void FxWidget::onCtxMenuItemChecked()
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

    void FxWidget::disconnectConnector(ConnectorPoint *pPoint) const
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

    void FxWidget::addToFxChainNoOptimize() const
    {
        gFxChain.addFxNoOptimize(fxDsc);
    }

    void FxWidget::resizeEvent(QResizeEvent *event)
    {
        QWidget::resizeEvent(event);

        updateConnectorLocalRects();
        updateConnectorPositions();
    }

    void FxWidget::moveEvent(QMoveEvent *event)
    {
        QWidget::moveEvent(event);

        updateConnectorPositions();
        wndParent->update();
    }

    void FxWidget::updateConnectorPositions()
    {
        for (auto &conn: connectors)
        {
            conn.globalRect = QRect{mapToParent(conn.localRect.topLeft()), conn.localRect.size()};
        }
    }

    void FxWidget::updateConnectorLocalRects() const
    {
        auto cx = width() / 2;
        auto cy = height() / 2;

        connectorTop->localRect    = {cx - FX_WIDGET_CONNECTOR_RADIUS_SIDES, 0, FX_WIDGET_CONNECTOR_RADIUS_SIDES * 2, FX_WIDGET_CONNECTOR_RADIUS_VERTICAL};
        connectorBottom->localRect = {cx - FX_WIDGET_CONNECTOR_RADIUS_SIDES, height() - FX_WIDGET_CONNECTOR_RADIUS_VERTICAL, FX_WIDGET_CONNECTOR_RADIUS_SIDES * 2, FX_WIDGET_CONNECTOR_RADIUS_VERTICAL};
        connectorRight->localRect  = {width() - FX_WIDGET_CONNECTOR_RADIUS_VERTICAL, cy - FX_WIDGET_CONNECTOR_RADIUS_SIDES, FX_WIDGET_CONNECTOR_RADIUS_VERTICAL, FX_WIDGET_CONNECTOR_RADIUS_SIDES * 2};
        connectorLeft->localRect   = {0, cy - FX_WIDGET_CONNECTOR_RADIUS_SIDES, FX_WIDGET_CONNECTOR_RADIUS_VERTICAL, FX_WIDGET_CONNECTOR_RADIUS_SIDES * 2};
    }

    void FxWidget::paintEvent(QPaintEvent *event)
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

    void FxWidget::mouseMoveEvent(QMouseEvent *event)
    {
        QWidget::mouseMoveEvent(event);

        auto leftClickDown = event->buttons() == Qt::LeftButton;
        if ((leftClickDown && currentConnectorIdx < 0) || (justAdded && moveAroundOnSpawn))
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

    void FxWidget::mousePressEvent(QMouseEvent *event)
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

    void FxWidget::leaveEvent(QEvent *event)
    {
        QWidget::leaveEvent(event);

        setCursor(Qt::ArrowCursor);
    }

    FxWidget::ConnectorPoint::ConnectorPoint(FxWidget *pParent)
    {
        parent = pParent;
        type   = CONN_UNCONNECTED;
        origin = FX_INVALID_INSTANCE_ID;
    }

    FxWidget::ConnectorPoint *FxWidget::ConnectorPoint::withType(ConnectorType pNewType)
    {
        type = pNewType;
        if (pNewType == CONN_OUTPUT && parent->widgetType == WIDGET_REGULAR)
        {
            origin = parent->fxDsc->instanceId;
        }
        else if (pNewType == CONN_INPUT)
        {
            origin = FX_INVALID_INSTANCE_ID;
        }

        return this;
    }

    QPoint FxWidget::getGlobalCentre() const
    {
        return QPoint{x() + width() / 2, y() + height() / 2};
    }

    QPoint FxWidget::getLocalCentre() const
    {
        return QPoint{width() / 2, height() / 2};
    }

    void FxWidget::moveToCentre()
    {
        move(wndParent->getCenter().x() - width() / 2, wndParent->getCenter().y() - height() / 2);
    }

    // input widget

    FxWidgetIn *FxWidgetIn::instance = nullptr;

    FxWidgetIn::FxWidgetIn(FxMainWindow *pParent): FxWidget(pParent)
    {
        fxIdToWidgetMap[gFxInputInstanceId] = this;

        instance = this;

        setCursor(Qt::ArrowCursor);
        widgetType        = WIDGET_INPUT;
        moveAroundOnSpawn = false;
        moveToCentre();

        for (auto &x: connectors)
        {
            x.origin = gFxInputInstanceId;
        }
    }

    void FxWidgetIn::render(QPainter &pPainter)
    {
        pPainter.drawText(rect(), Qt::AlignCenter, "IN");
    }

    FxWidgetIn::~FxWidgetIn()
    = default;

    // output widget

    FxWidgetOut *FxWidgetOut::instance = nullptr;

    FxWidgetOut::FxWidgetOut(FxMainWindow *pParent) : FxWidget(pParent)
    {
        instance = this;

        setCursor(Qt::ArrowCursor);
        widgetType        = WIDGET_OUTPUT;
        moveAroundOnSpawn = false;
        moveToCentre();
        move(x() + FX_WIDGET_DEFAULT_WIDTH * 3, y());

        for (auto &x: connectors)
        {
            x.origin = FX_INVALID_INSTANCE_ID;
        }
    }

    void FxWidgetOut::render(QPainter &pPainter)
    {
        pPainter.drawText(rect(), Qt::AlignCenter, "OUT");
    }

    FxWidgetOut::~FxWidgetOut()
    = default;

    // gui actions

    void FxMainWindow::showCtxMenu(QPoint pPoint)
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

    void FxMainWindow::addActionSimple(QMenu *pMenu, const QString &pLabel, void (FxMainWindow::*pFunc)()) const // NOLINT(*-convert-member-functions-to-static)
    {
        auto action = pMenu->addAction(pLabel);
        connect(action, QAction::triggered, this, pFunc);
    }

    void FxMainWindow::addGain()
    {
        auto widget = new FxWidgetGain(this);
        widget->show();

        gFxChain.addFxNoOptimize(widget->fxDsc);
    }

    void FxMainWindow::addBezier()
    {
    }

    void FxMainWindow::addDiode()
    {
    }

    void FxMainWindow::addSum()
    {
    }

    void FxMainWindow::addDryWet()
    {
    }

    void FxMainWindow::addLopass1()
    {
    }

    void FxMainWindow::addHipass1()
    {
        auto widget = new FxWidgetHipass1(this);
        widget->show();

        gFxChain.addFxNoOptimize(widget->fxDsc);
    }
}
