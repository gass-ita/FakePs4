#ifndef CANVAS_H
#define CANVAS_H

#include <QWidget>
#include <QMouseEvent>
#include <QPoint>
#include <QWheelEvent>
#include "LayerManager.h"
#include "LMObserver.h"
#include <memory>
#include "Tool.h"

class Canvas : public QWidget, public LMObserver
{
    Q_OBJECT

public:
    explicit Canvas(QWidget *parent = nullptr);
    void onRegionChanged(int x, int y, int width, int height) override;
    void onLayerListChanged() override { emit coreLayerListChanged(); };
    void OnColorChange(int r, int g, int b, int a) override { emit coreColorChanged(r, g, b, a); };
    void setTool(std::unique_ptr<Tool> newTool)
    {
        layerManager.setActiveTool(std::move(newTool));
    }
    void setToolSize(int newSize)
    {
        layerManager.setToolSize(newSize);
    }
    void setToolColor(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255)
    {
        layerManager.setToolColor(red, green, blue, alpha);
    }
    void setLayerOpacity(int index, int opacityPercentage)
    {
        float opacity = std::clamp(opacityPercentage / 100.0f, 0.0f, 1.0f);
        layerManager.setLayerOpacity(index, opacity);
    }
    void addNewLayer(const QString &name);
    void setActiveLayer(int index);

    void setLayerVisibility(int index, bool visible);
    void setLayerName(int index, const QString &name);

    LayerManager &getLayerManager() { return layerManager; }

protected:
    void paintEvent(QPaintEvent *event) override;

    // Mouse Events for drawing
    void tabletEvent(QTabletEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    LayerManager layerManager;

    std::vector<uint8_t> frameBuffer;

    QPoint lastPos;

    float zoom = 1.0f;
    QPointF panOffset = QPointF(0, 0);

    bool isPanning = false;
    QPoint lastPanPos;

    // Helper math to convert screen clicks to actual image pixels
    QPoint screenToImage(const QPoint &screenPos) const;

signals:
    // 2. Define a Qt signal to shout to the rest of the app
    void coreLayerListChanged();
    void coreColorChanged(int r, int g, int b, int a);
};

#endif // CANVAS_H