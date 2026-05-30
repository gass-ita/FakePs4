#include "MainWindow.h"
#include "Tool.h" // So we can instantiate the tools

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle("FakePs4 - Tool Selector");

    canvas = new Canvas(this);
    setCentralWidget(canvas);

    // ==========================================
    // CREATE THE TOOLBAR
    // ==========================================
    QToolBar *toolbar = addToolBar("Tools");

    QAction *brushAction = toolbar->addAction("Brush");
    QAction *eraserAction = toolbar->addAction("Eraser");
    QAction *rectAction = toolbar->addAction("Rectangle");

    // Connect the buttons to change the Canvas tool using Lambda functions
    connect(brushAction, &QAction::triggered, this, [this]()
            { canvas->setTool(std::make_shared<BrushTool>()); });

    connect(eraserAction, &QAction::triggered, this, [this]()
            { canvas->setTool(std::make_shared<EraserTool>()); });

    connect(rectAction, &QAction::triggered, this, [this]()
            { canvas->setTool(std::make_shared<RectangleTool>()); });

    adjustSize();
}