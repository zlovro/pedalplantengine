//
// Created by lovro on 07/02/2026.
// Copyright (c) 2026 lovro. All rights reserved.
//

#ifndef FX_GFX_HPP
#define FX_GFX_HPP

#include <QWindow>
#include <QCursor>
#include <QMainWindow>
#include <QPainter>
#include <QPen>

#include <fx/fx.hpp>

namespace Fx
{
    class FxWidgetIn;
    extern const char *FX_FONT_FAMILY;
    extern const int   FX_FONT_SIZE;
    extern const int   FX_FONT_SIZE_SMALL;

    extern const QFont FX_DEFAULT_FONT;
    extern const QFont FX_BOLD_FONT;
    extern const QFont FX_BOLD_FONT_SMALL;

    class FxMainWindow;

    extern const QColor FX_WIDGET_FILL_COLOR;

    extern const QColor FX_WIDGET_OUTLINE_COLOR;
    extern const int    FX_WIDGET_OUTLINE_RADIUS;

    extern const QColor FX_WIDGET_UNCONNECTED_POINT_COLOR;
    extern const QColor FX_WIDGET_CONNECTED_INPUT_COLOR;
    extern const QColor FX_WIDGET_CONNECTED_OUTPUT_COLOR;
    extern const QColor FX_WIDGET_IGNORE_POINT_COLOR;
    extern const int    FX_WIDGET_CONNECTOR_RADIUS_SIDES;
    extern const int    FX_WIDGET_CONNECTOR_RADIUS_VERTICAL;
    extern const int    FX_WIDGET_CONNECTOR_RADIUS_BORDER;

    extern const int     FX_WIDGET_DEFAULT_WIDTH, FX_WIDGET_DEFAULT_HEIGHT;
    inline constexpr int FX_WIDGET_DIAL_SIZE   = 60;
    inline constexpr int FX_WIDGET_DIAL_MARGIN = 30;
    extern const int     FX_WIDGET_OUTLINE_WIDTH;

    class FxWidget : public QWidget
    {
        public:
        static std::map<FxInstanceId, FxWidget*> fxIdToWidgetMap;

        typedef enum FxWidgetType
        {
            WIDGET_REGULAR,
            WIDGET_INPUT,
            WIDGET_OUTPUT
        } FxWidgetType;

        typedef enum
        {
            CONN_INPUT,
            CONN_OUTPUT,
            CONN_UNCONNECTED,
            CONN_TYPE_COUNT,
            CONN_INVALID
        } ConnectorType;

        class ConnectorPoint
        {
            public:
            FxWidget *parent;

            QRect localRect;
            QRect globalRect;

            ConnectorType type;
            FxInstanceId  origin;

            explicit ConnectorPoint(FxWidget *pParent = nullptr);

            ConnectorPoint *withType(ConnectorType pNewType);
        };

        FxMainWindow *wndParent;
        FxDescriptor *fxDsc;

        bool justAdded, moveAroundOnSpawn;
        int  currentConnectorIdx;

        FxWidgetType                  widgetType;
        std::array<ConnectorPoint, 4> connectors;

        explicit FxWidget(FxMainWindow *pParent, FxDescriptor* pDsc);

        virtual void render(QPainter &pPainter);

        ConnectorPoint *connectorRight, *connectorLeft, *connectorTop, *connectorBottom;

        void showCtxMenu(QPoint pPoint);
        void onCtxMenuItemChecked();

        void disconnectConnector(ConnectorPoint *pPoint) const;
        void addToFxChainNoOptimize() const;

        void resizeEvent(QResizeEvent *event) override;
        void moveEvent(QMoveEvent *event) override;
        void paintEvent(QPaintEvent *event) override;
        void mouseMoveEvent(QMouseEvent *event) override;
        void mousePressEvent(QMouseEvent *event) override;
        void leaveEvent(QEvent *event) override;

        QPoint getGlobalCentre() const;
        QPoint getLocalCentre() const;

        void updateConnectorPositions();
        void updateConnectorLocalRects() const;
        void moveToCentre();

        private:
        QPoint mLastMousePos;
    };

    // singleton
    class FxWidgetIn : public FxWidget
    {
        public:
        static FxWidgetIn *instance;

        explicit FxWidgetIn(FxMainWindow *pParent);

        void render(QPainter &pPainter) override;

        ~FxWidgetIn() override;
    };

    class FxWidgetOut : public FxWidget
    {
        public:
        static FxWidgetOut *instance;

        explicit FxWidgetOut(FxMainWindow *pParent);

        void render(QPainter &pPainter) override;

        ~FxWidgetOut() override;
    };

    // singleton
    class FxMainWindow : public QMainWindow
    {
        public:
        static FxMainWindow *instance;

        class ConnectingLine
        {
            public:
            FxWidget::ConnectorPoint *a, *b;

            ConnectingLine(FxWidget::ConnectorPoint *pA, FxWidget::ConnectorPoint *pB);
        };

        bool                      isDrawing;
        FxWidget::ConnectorPoint *drawingOriginConnector;
        QPen                      drawingPen, defaultPen;

        // do not add or remove, only read
        std::vector<ConnectingLine *> connectingLines;

        FxMainWindow(const QPen& pDefaultPen);
        ~FxMainWindow();

        void startDrawing(FxWidget::ConnectorPoint *pSourceConnector, const QPen &pPen);
        void cancelDrawing();
        void stopDrawing();

        std::vector<ConnectingLine *> getLinesOnConnector(const FxWidget::ConnectorPoint *pPoint) const;

        bool canConnectFx(FxWidget::ConnectorPoint *pSrc, FxWidget::ConnectorPoint *pDst) const;

        // you MUST call canConnectFx before calling this.
        void connectFx(FxWidget::ConnectorPoint *pSrc, FxWidget::ConnectorPoint *pDst);

        void showCtxMenu(QPoint);
        void addActionSimple(QMenu *pMenu, const QString &pLabel, void (FxMainWindow::*pFunc)()) const;

        void addGain();
        void addBezier();
        void addDiode();
        void addSum();
        void addDryWet();
        void addLopass1();
        void addHipass1();

        void paintEvent(QPaintEvent *event) override;
        void mouseMoveEvent(QMouseEvent *event) override;
        void mousePressEvent(QMouseEvent *event) override;
        void keyReleaseEvent(QKeyEvent *event) override;

        QPoint getCenter() const;
    };

    static void drawBezier(QPainter &pPainter, const std::vector<QPoint> &pPoints, const QPen &pPen);

    void update();
}


#endif //FX_GFX_HPP
