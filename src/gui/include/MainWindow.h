#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "Canvas.h"
#include <QToolBar>
#include <QAction>

// ... existing code ...

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(int canvasWidth, int canvasHeight, const QString &filePath = "", QWidget *parent = nullptr);

private:
    QColor currentToolColor = Qt::red;
    Canvas *canvas;
};

#endif // MAINWINDOW_H