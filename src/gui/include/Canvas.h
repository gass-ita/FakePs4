#ifndef CANVAS_H
#define CANVAS_H

#include <QWidget>
#include <QMouseEvent>
#include <QPoint>
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
    void setTool(std::shared_ptr<Tool> newTool) { currentTool = newTool; }

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override; // Add release event

    // Mouse Events for drawing
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    LayerManager layerManager;
    std::shared_ptr<Tool> currentTool;
    QPoint lastPos; // Tracks the last mouse position to draw lines
};

#endif // CANVAS_H