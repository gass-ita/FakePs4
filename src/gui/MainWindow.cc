#include "MainWindow.h"
#include "Tool.h"
#include <QColorDialog>
#include <QPixmap>
#include <QSlider>
#include <QLabel>
#include <QDockWidget>
#include <QListWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QPainter>
#include <QMenu>
#include <QFileDialog>
#include <QStandardPaths>
#include <QMessageBox>
#include <QShortcut>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
        setWindowTitle("FakePs4");

        canvas = new Canvas(this);
        setCentralWidget(canvas);

        QToolBar *toolbar = addToolBar("Tools");

        // Add Tools
        QAction *brushAction = toolbar->addAction("Brush");
        QAction *eraserAction = toolbar->addAction("Eraser");
        QAction *rectAction = toolbar->addAction("Rectangle");
        QAction *ellipseAction = toolbar->addAction("Ellipse");

        toolbar->addSeparator(); // Add a nice visual line

        QPixmap colorPixmap(24, 24);
        colorPixmap.fill(Qt::red);

        QAction *colorAction = toolbar->addAction(QIcon(colorPixmap), "Color");
        toolbar->addSeparator();

        // Add Size Slider Label
        toolbar->addWidget(new QLabel(" Size: "));

        // Create the Slider
        QSlider *sizeSlider = new QSlider(Qt::Horizontal);
        sizeSlider->setRange(1, 50); // Brush size from 1px to 50px
        sizeSlider->setValue(5);     // Default to 5
        sizeSlider->setFixedWidth(150);
        toolbar->addWidget(sizeSlider);

        QDockWidget *layerDock = new QDockWidget("Layers", this);
        layerDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

        QWidget *dockContents = new QWidget();
        QVBoxLayout *dockLayout = new QVBoxLayout(dockContents);

        QListWidget *layerList = new QListWidget();
        layerList->setContextMenuPolicy(Qt::CustomContextMenu);

        // Helper lambda to create a fully interactive list item
        auto createLayerItem = [](const QString &name)
        {
                QListWidgetItem *item = new QListWidgetItem(name);
                // Enable selection, double-click editing, and checkboxes
                item->setFlags(item->flags() | Qt::ItemIsEditable | Qt::ItemIsUserCheckable);
                item->setCheckState(Qt::Checked); // Visible by default
                return item;
        };

        // Add the initial Background layer
        layerList->addItem(createLayerItem("Background"));
        layerList->setCurrentRow(0);

        QPushButton *addLayerBtn = new QPushButton("Add New Layer");

        dockLayout->addWidget(layerList);
        dockLayout->addWidget(addLayerBtn);
        layerDock->setWidget(dockContents);
        addDockWidget(Qt::RightDockWidgetArea, layerDock);

        // ==========================================
        // CONNECTIONS
        // ==========================================
        connect(brushAction, &QAction::triggered, this, [this]()
                { canvas->setTool(std::make_shared<BrushTool>()); });

        connect(eraserAction, &QAction::triggered, this, [this]()
                { canvas->setTool(std::make_shared<EraserTool>()); });

        connect(rectAction, &QAction::triggered, this, [this]()
                { canvas->setTool(std::make_shared<RectangleTool>()); });

        // When the slider moves, tell the Canvas to update the tool size
        connect(sizeSlider, &QSlider::valueChanged, this, [this](int value)
                { canvas->setToolSize(value); });
        connect(ellipseAction, &QAction::triggered, this, [this]()
                { canvas->setTool(std::make_shared<EllipseTool>()); });
        // Set the default tool to Brush
        canvas->setTool(std::make_shared<BrushTool>());

        // Wire up the Color Picker
        connect(colorAction, &QAction::triggered, this, [this, colorAction]()
                {
        
        // ADD THE FLAG HERE: QColorDialog::ShowAlphaChannel
        QColor newColor = QColorDialog::getColor(
            Qt::red, 
            this, 
            "Choose Brush Color", 
            QColorDialog::ShowAlphaChannel
        );
        
        if (newColor.isValid()) {
            canvas->setToolColor(newColor.red(), newColor.green(), newColor.blue(), newColor.alpha());
            
            QPixmap pixmap(24, 24);
            // Optional: Fill with a checkerboard or white first so highly transparent colors are visible in the UI
            pixmap.fill(Qt::white); 
            
            // Draw the actual picked color over it
            QPainter painter(&pixmap);
            painter.fillRect(pixmap.rect(), newColor);
            
            colorAction->setIcon(QIcon(pixmap));
        } });

        // 1. Add Layer Button
        connect(addLayerBtn, &QPushButton::clicked, this, [this, layerList, createLayerItem]()
                {
        int newIndex = layerList->count();
        QString layerName = QString("Layer %1").arg(newIndex);
        
        canvas->addNewLayer(layerName);
        
        // Add the interactive item
        layerList->addItem(createLayerItem(layerName));
        layerList->setCurrentRow(newIndex); });

        // 2. Change Active Layer (Clicking an item)
        connect(layerList, &QListWidget::currentRowChanged, this, [this](int row)
                {
        if (row >= 0) {
            canvas->setActiveLayer(row);
        } });

        // 3. Rename or Toggle Visibility
        // This fires whenever a checkbox is clicked OR text is edited
        connect(layerList, &QListWidget::itemChanged, this, [this, layerList](QListWidgetItem *item)
                {
        int row = layerList->row(item);
        if (row < 0) return;

        // Sync the visibility state
        bool isVisible = (item->checkState() == Qt::Checked);
        canvas->setLayerVisibility(row, isVisible);

        // Sync the name
        canvas->setLayerName(row, item->text()); });

        connect(layerList, &QListWidget::customContextMenuRequested, this, [this, layerList](const QPoint &pos)
                {
        // Find exactly which layer item they right-clicked on
        QListWidgetItem *item = layerList->itemAt(pos);
        
        // If they clicked on empty space (not a layer), do nothing
        if (!item) return; 

        // Create the popup menu
        QMenu contextMenu(this);
        QAction *renameAction = contextMenu.addAction("Rename");

        // Spawn the menu exactly where the mouse cursor is
        QAction *selectedAction = contextMenu.exec(layerList->mapToGlobal(pos));

        // If they clicked "Rename"
        if (selectedAction == renameAction) {
            // Trigger Qt's inline text editor for this item.
            // When they hit Enter, your existing itemChanged signal will catch it!
            layerList->editItem(item);
        } });

        // Saving and Loading
        // ==========================================
        // SAVE NATIVE PROJECT (Ctrl+S)
        // ==========================================
        QShortcut *saveShortcut = new QShortcut(QKeySequence::Save, this);
        connect(saveShortcut, &QShortcut::activated, this, [this]()
                {
        QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/untitled.ptproj";
        QString filePath = QFileDialog::getSaveFileName(this, "Save Project", defaultPath, "Paint Project (*.ptproj)");

        if (!filePath.isEmpty()) {
            if (!canvas->getLayerManager().saveProject(filePath.toStdString())) {
                QMessageBox::critical(this, "Error", "Failed to save project.");
            }
        } });

        // ==========================================
        // LOAD NATIVE PROJECT (Ctrl+O)
        // ==========================================
        QShortcut *openShortcut = new QShortcut(QKeySequence::Open, this);
        connect(openShortcut, &QShortcut::activated, this, [this]()
                {
        QString filePath = QFileDialog::getOpenFileName(this, "Open Project", "", "Paint Project (*.ptproj)");

        if (!filePath.isEmpty()) {
            if (!canvas->getLayerManager().loadProject(filePath.toStdString())) {
                QMessageBox::critical(this, "Error", "File is corrupted or not a valid project.");
            } else {
                // Resize the actual Qt Window widget if the loaded canvas is a different size!
                canvas->setFixedSize(canvas->getLayerManager().getWidth(), canvas->getLayerManager().getHeight());
            }
        } });

        // Note the captured variables in the brackets: [this, layerList, createLayerItem]
        connect(canvas, &Canvas::coreLayerListChanged, this, [this, layerList, createLayerItem]()
                {
            // 1. Temporarily block signals so clearing the list doesn't trigger active layer changes
            layerList->blockSignals(true);
            
            // 2. Clear the Qt List Widget completely
            layerList->clear(); 

            // 3. Ask the core engine for the current layers
            const auto& layers = canvas->getLayerManager().getLayers();

            for (size_t i = 0; i < layers.size(); ++i)
            {

                    QListWidgetItem *item = createLayerItem(QString::fromStdString(layers[i]->name));

                    // Restore visibility checkboxes
                    item->setCheckState(layers[i]->visible ? Qt::Checked : Qt::Unchecked);

                    layerList->addItem(item);
            }

            // 5. Set the UI selection to match the active layer
            layerList->setCurrentRow(layers.size() - 1 - canvas->getLayerManager().getActiveLayerIndex());
            
            // 6. Unblock signals so the user can click on layers again
            layerList->blockSignals(false); });

        adjustSize();
}