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
    void setTool(std::shared_ptr<Tool> newTool)
    {
        currentTool = newTool;
        if (currentTool)
        {
            currentTool->setSize(currentToolSize);
            currentTool->setColor(r, g, b, a);
        }
    }
    void setToolSize(int newSize)
    {
        currentToolSize = newSize;
        if (currentTool)
        {
            currentTool->setSize(newSize);
        }
    }
    void setToolColor(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255)
    {
        r = red;
        g = green;
        b = blue;
        a = alpha;
        if (currentTool)
        {
            currentTool->setColor(r, g, b, a);
        }
    }

    void addNewLayer(const QString &name);
    void setActiveLayer(int index);

    void setLayerVisibility(int index, bool visible);
    void setLayerName(int index, const QString &name);

protected:
    void paintEvent(QPaintEvent *event) override;

    // Mouse Events for drawing
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    int currentToolSize = 5;
    LayerManager layerManager;
    std::shared_ptr<Tool> currentTool;
    QPoint lastPos;
    uint8_t r = 0, g = 0, b = 0, a = 255; // active color (default black)

    float zoom = 1.0f;
    QPointF panOffset = QPointF(0, 0);

    bool isPanning = false;
    QPoint lastPanPos;

    // Helper math to convert screen clicks to actual image pixels
    QPoint screenToImage(const QPoint &screenPos) const;
};

#endif // CANVAS_H