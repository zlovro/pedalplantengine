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

namespace Fx::Gfx
{
    class FxGfxFxWidgetInput;
    extern const char *FX_FONT_FAMILY;
    extern const int   FX_FONT_SIZE;
    extern const int   FX_FONT_SIZE_SMALL;

    extern const QFont FX_DEFAULT_FONT;
    extern const QFont FX_BOLD_FONT;
    extern const QFont FX_BOLD_FONT_SMALL;

    class FxGfxMainWindow;

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

    class FxGfxFxWidget : public QWidget
    {
        public:
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
            FxGfxFxWidget *parent;

            QRect localRect;
            QRect globalRect;

            ConnectorType type;
            FxInstanceId  origin;

            explicit ConnectorPoint(FxGfxFxWidget *pParent = nullptr);
        };

        FxGfxMainWindow *wndParent;
        FxDescriptor *   fxDsc;

        bool justAdded;
        int  currentConnectorIdx;

        FxWidgetType                  widgetType;
        std::array<ConnectorPoint, 4> connectors;

        explicit FxGfxFxWidget(FxGfxMainWindow *pParent);

        virtual void render(QPainter &pPainter)
        {
        };

        void showCtxMenu(QPoint pPoint);
        void onCtxMenuItemChecked();

        void disconnectConnector(ConnectorPoint *pPoint) const;

        void resizeEvent(QResizeEvent *event) override;
        void moveEvent(QMoveEvent *event) override;
        void paintEvent(QPaintEvent *event) override;
        void mouseMoveEvent(QMouseEvent *event) override;
        void mousePressEvent(QMouseEvent *event) override;
        void leaveEvent(QEvent *event) override;

        QPoint getGlobalCentre() const;
        QPoint getLocalCentre() const;

        void updateConnectorPositions();
        void moveToCentre();

        private:
        QPoint mLastMousePos;
    };

    // singleton
    class FxGfxFxWidgetInput : public FxGfxFxWidget
    {
        public:
        static FxGfxFxWidgetInput *instance;

        explicit FxGfxFxWidgetInput(FxGfxMainWindow *pParent);

        void render(QPainter &pPainter) override;

        ~FxGfxFxWidgetInput() override;
    };

    class FxGfxFxWidgetOutput : public FxGfxFxWidget
    {
        public:
        static FxGfxFxWidgetOutput *instance;

        explicit FxGfxFxWidgetOutput(FxGfxMainWindow *pParent);

        void render(QPainter &pPainter) override;

        ~FxGfxFxWidgetOutput() override;
    };

    // singleton
    class FxGfxMainWindow : public QMainWindow
    {
        public:
        static FxGfxMainWindow *instance;

        class ConnectingLine
        {
            public:
            FxGfxFxWidget::ConnectorPoint *a, *b;

            ConnectingLine(FxGfxFxWidget::ConnectorPoint *pA, FxGfxFxWidget::ConnectorPoint *pB);
        };

        bool                           isDrawing;
        FxGfxFxWidget::ConnectorPoint *drawingOriginConnector;
        QPen                           drawingPen;

        // do not add or remove, only read
        std::vector<ConnectingLine *> connectingLines;

        FxGfxMainWindow();
        ~FxGfxMainWindow();

        void startDrawing(FxGfxFxWidget::ConnectorPoint *pSourceConnector, const QPen &pPen);
        void cancelDrawing();
        void stopDrawing();

        std::vector<ConnectingLine *> getLinesOnConnector(const FxGfxFxWidget::ConnectorPoint *pPoint) const;

        bool canConnectFx(FxGfxFxWidget::ConnectorPoint *pSrc, FxGfxFxWidget::ConnectorPoint *pDst);

        // you MUST call canConnectFx before calling this.
        void connectFx(FxGfxFxWidget::ConnectorPoint *pSrc, FxGfxFxWidget::ConnectorPoint *pDst);

        void showCtxMenu(QPoint);
        void addActionSimple(QMenu *pMenu, const QString &pLabel, void (FxGfxMainWindow::*pFunc)()) const;

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
