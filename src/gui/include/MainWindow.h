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
    MainWindow(QWidget *parent = nullptr);

private:
    Canvas *canvas;
};

#endif // MAINWINDOW_H