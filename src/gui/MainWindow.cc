#include "MainWindow.h"
#include "Tool.h"
#include "ConvolutionFilter.h"
#include <QColorDialog>
#include <QPixmap>
#include <QSlider>
#include <QLineEdit>
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
#include <QDialog>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QProgressDialog>
#include <QtConcurrent>
#include <QComboBox>
#include <QStackedWidget>
#include <QFormLayout>
#include <QFutureWatcher>
#include <QToolButton>
#include <QMenuBar>
#include <QScrollBar>
#include <QGridLayout>

MainWindow::MainWindow(int canvasWidth, int canvasHeight, const QString &filePath, QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle("FakePs4");

    canvas = new Canvas(canvasWidth, canvasHeight, this);
    // ==========================================
    // THE SCROLLBAR LAYOUT
    // ==========================================
    QWidget *centralContainer = new QWidget(this);
    QGridLayout *gridLayout = new QGridLayout(centralContainer);

    // Remove the default margins so the canvas touches the edges
    gridLayout->setContentsMargins(0, 0, 0, 0);
    gridLayout->setSpacing(0);

    QScrollBar *vScrollBar = new QScrollBar(Qt::Vertical, this);
    QScrollBar *hScrollBar = new QScrollBar(Qt::Horizontal, this);

    // Add widgets to the grid: Canvas at (0,0), V-Bar at (0,1), H-Bar at (1,0)
    gridLayout->addWidget(canvas, 0, 0);
    gridLayout->addWidget(vScrollBar, 0, 1);
    gridLayout->addWidget(hScrollBar, 1, 0);

    setCentralWidget(centralContainer);

    // ==========================================
    // SCROLLBAR CONNECTIONS
    // ==========================================

    // 1. When Canvas zooms or pans, update the Scrollbars
    connect(canvas, &Canvas::scrollRangeChanged, this, [hScrollBar, vScrollBar](int maxX, int maxY, int pageX, int pageY)
            {
        hScrollBar->setRange(-pageX/2, maxX); // Allow negative scroll to see past the left edge
        vScrollBar->setRange(-pageY/2, maxY);
        hScrollBar->setPageStep(pageX);
        vScrollBar->setPageStep(pageY); });

    connect(canvas, &Canvas::scrollValueChanged, this, [hScrollBar, vScrollBar](int x, int y)
            {
        // Temporarily block signals so the scrollbars don't fire an event back to the canvas, causing an infinite loop!
        hScrollBar->blockSignals(true);
        vScrollBar->blockSignals(true);
        
        hScrollBar->setValue(x);
        vScrollBar->setValue(y);
        
        hScrollBar->blockSignals(false);
        vScrollBar->blockSignals(false); });

    // 2. When the user drags a Scrollbar, update the Canvas
    connect(hScrollBar, &QScrollBar::valueChanged, canvas, &Canvas::setScrollX);
    connect(vScrollBar, &QScrollBar::valueChanged, canvas, &Canvas::setScrollY);

    QToolBar *toolbar = addToolBar("Tools");

    // ==========================================
    // 1. BRUSH DROPDOWN
    // ==========================================
    QToolButton *brushButton = new QToolButton(this);
    brushButton->setText("Brushes");
    brushButton->setPopupMode(QToolButton::InstantPopup);
    QMenu *brushMenu = new QMenu(brushButton);

    QAction *brushAction = brushMenu->addAction("Standard Brush");
    QAction *pressureBrushAction = brushMenu->addAction("Pressure Brush");
    QAction *coneAction = brushMenu->addAction("Cone Brush");
    QAction *sprayAction = brushMenu->addAction("Spray");

    brushButton->setMenu(brushMenu);
    toolbar->addWidget(brushButton);

    // ==========================================
    // 2. STANDALONE TOOLS
    // ==========================================
    QAction *eraserAction = toolbar->addAction("Eraser");
    QAction *fillAction = toolbar->addAction("Fill");
    QAction *cPickerAction = toolbar->addAction("Color Picker");

    // ==========================================
    // 3. SHAPE DROPDOWN
    // ==========================================
    QToolButton *shapeButton = new QToolButton(this);
    shapeButton->setText("Shapes");
    shapeButton->setPopupMode(QToolButton::InstantPopup);
    QMenu *shapeMenu = new QMenu(shapeButton);

    QAction *rectAction = shapeMenu->addAction("Rectangle");
    QAction *ellipseAction = shapeMenu->addAction("Ellipse");

    shapeButton->setMenu(shapeMenu);
    toolbar->addWidget(shapeButton);

    toolbar->addSeparator(); // Add a nice visual line

    // ==========================================
    // 4. COLOR & SIZE
    // ==========================================
    QPixmap colorPixmap(24, 24);
    colorPixmap.fill(Qt::red);

    QAction *colorAction = toolbar->addAction(QIcon(colorPixmap), "Color");
    toolbar->addSeparator();

    // Add Size Slider Label
    toolbar->addWidget(new QLabel(" Size: "));

    // Create the Slider
    QLineEdit *sizeTextBox = new QLineEdit();
    sizeTextBox->setFixedWidth(50);
    sizeTextBox->setText("5");
    QSlider *sizeSlider = new QSlider(Qt::Horizontal);
    sizeSlider->setRange(1, 150); // Brush size from 1px to 50px
    sizeSlider->setValue(5);      // Default to 5
    sizeSlider->setFixedWidth(150);
    toolbar->addWidget(sizeTextBox);
    toolbar->addWidget(sizeSlider);

    toolbar->addSeparator();

    QAction *zoomOutAction = toolbar->addAction(" - "); // Or use a QIcon!

    QLineEdit *zoomTextBox = new QLineEdit();
    zoomTextBox->setText("100%");
    zoomTextBox->setFixedWidth(60);
    zoomTextBox->setAlignment(Qt::AlignCenter);
    toolbar->addWidget(zoomTextBox);

    QAction *zoomInAction = toolbar->addAction(" + ");

    // --- ZOOM CONNECTIONS ---

    // 1. Buttons -> Canvas
    connect(zoomInAction, &QAction::triggered, canvas, &Canvas::zoomIn);
    connect(zoomOutAction, &QAction::triggered, canvas, &Canvas::zoomOut);

    // 2. Canvas -> Text Box (Updates when you use the scroll wheel OR buttons)
    connect(canvas, &Canvas::zoomChanged, this, [zoomTextBox](int percentage)
            { zoomTextBox->setText(QString::number(percentage) + "%"); });

    // 3. Text Box -> Canvas (Allows the user to type "200" and hit Enter!)
    connect(zoomTextBox, &QLineEdit::editingFinished, this, [this, zoomTextBox]()
            {
        QString text = zoomTextBox->text();
        text.replace("%", ""); // Strip out the % sign if they typed it
        
        bool ok;
        int value = text.toInt(&ok);
        
        if (ok) {
            // Convert integer percentage back to float (e.g., 200 -> 2.0f)
            canvas->setZoom(value / 100.0f); 
        } else {
            // If they typed gibberish ("abc"), reset the text box back to the actual zoom
            int currentZoom = static_cast<int>(canvas->getZoom() * 100);
            zoomTextBox->setText(QString::number(currentZoom) + "%");
        } });

    // ==========================================
    // LAYERS DOCK
    // ==========================================
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
        item->setFlags(item->flags() | Qt::ItemIsEditable | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Checked);
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

    // Brushes
    connect(brushAction, &QAction::triggered, this, [this]()
            { canvas->setTool(std::make_unique<MaskBrushTool>(MaskBrushTool::createRoundBrushMask(200, 0.8f))); });
    connect(pressureBrushAction, &QAction::triggered, this, [this]()
            {
        std::unique_ptr<Tool> brush = std::make_unique<MaskBrushTool>(MaskBrushTool::createRoundBrushMask(200, 0.8f));
    brush = std::make_unique<PressureSizeDecorator>(std::move(brush));
    canvas->setTool(std::move(brush)); });
    connect(coneAction, &QAction::triggered, this, [this]()
            {
        std::unique_ptr<Tool> brush = std::make_unique<MaskBrushTool>(MaskBrushTool::createRoundBrushMask(200, 0.8f));
    brush = std::make_unique<ConeGrowthDecorator>(std::move(brush), 0.1f, 2.0f);
    canvas->setTool(std::move(brush)); });
    connect(sprayAction, &QAction::triggered, this, [this]()
            { canvas->setTool(std::make_unique<SprayTool>()); });

    // Shapes
    connect(rectAction, &QAction::triggered, this, [this]()
            { canvas->setTool(std::make_unique<RectangleTool>()); });
    connect(ellipseAction, &QAction::triggered, this, [this]()
            { canvas->setTool(std::make_unique<EllipseTool>()); });

    // Standalone Tools
    connect(eraserAction, &QAction::triggered, this, [this]()
            { canvas->setTool(std::make_unique<EraserTool>()); });
    connect(fillAction, &QAction::triggered, this, [this]()
            { canvas->setTool(std::make_unique<FillTool>()); });
    connect(cPickerAction, &QAction::triggered, this, [this]()
            { canvas->setTool(std::make_unique<ColorPickerTool>()); });

    // Sliders
    connect(sizeSlider, &QSlider::valueChanged, this, [this](int value)
            { canvas->setToolSize(value); });

    connect(sizeSlider, &QSlider::valueChanged, this, [sizeTextBox](int value)
            { sizeTextBox->setText(QString::number(value)); });

    connect(sizeTextBox, &QLineEdit::editingFinished, this, [sizeTextBox, sizeSlider]()
            {
                bool ok = false;
                int value = sizeTextBox->text().toInt(&ok);
                if (!ok) {
                    sizeTextBox->setText(QString::number(sizeSlider->value()));
                    return;
                }

                value = qBound(sizeSlider->minimum(), value, sizeSlider->maximum());
                sizeSlider->setValue(value); });

    // Wire up the Color Picker
    connect(colorAction, &QAction::triggered, this, [this, colorAction]()
            {
        
        // ADD THE FLAG HERE: QColorDialog::ShowAlphaChannel
        QColor newColor = QColorDialog::getColor(
            currentToolColor, 
            this, 
            "Choose Brush Color", 
            QColorDialog::ShowAlphaChannel
        );
        
        if (newColor.isValid()) {
            currentToolColor = newColor;
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
    // 1. Add Layer Button
    connect(addLayerBtn, &QPushButton::clicked, this, [this, layerList, createLayerItem]()
            {
        // The new engine index is just the current count (e.g., 3 layers means new index is 3)
        int engineIndex = layerList->count();
        QString layerName = QString("Layer %1").arg(engineIndex);
        
        canvas->addNewLayer(layerName);
        
        // INSERT AT THE TOP OF THE UI LIST! (Row 0)
        layerList->insertItem(0, createLayerItem(layerName));
        
        // Automatically select the new top layer
        layerList->setCurrentRow(0); });

    // 2. Change Active Layer (Clicking an item)
    // 2. Change Active Layer (Clicking an item)
    // 2. Change Active Layer (Clicking an item)
    connect(layerList, &QListWidget::currentRowChanged, this, [this, layerList](int row)
            {
        if (row >= 0) {
            // THE REVERSE MATH
            int engineIndex = (layerList->count() - 1) - row;
            canvas->setActiveLayer(engineIndex);
        } });

    // 3. Rename or Toggle Visibility
    connect(layerList, &QListWidget::itemChanged, this, [this, layerList](QListWidgetItem *item)
            {
        int row = layerList->row(item);
        if (row < 0) return;

        // THE REVERSE MATH
        int engineIndex = (layerList->count() - 1) - row;

        bool isVisible = (item->checkState() == Qt::Checked);
        canvas->setLayerVisibility(engineIndex, isVisible);
        canvas->setLayerName(engineIndex, item->text()); });

    // 4. Context Menu
    connect(layerList, &QListWidget::customContextMenuRequested, this, [this, layerList](const QPoint &pos)
            {
        // Find exactly which layer item they right-clicked on
        QListWidgetItem *item = layerList->itemAt(pos);
        
        // If they clicked on empty space (not a layer), do nothing
        if (!item) return; 

        // Create the popup menu
        QMenu contextMenu(this);
        QAction *renameAction = contextMenu.addAction("Rename");
        QAction *propertiesAction = contextMenu.addAction("Properties...");
        QAction *filterAction = contextMenu.addAction("Filter..."); 

        // Spawn the menu exactly where the mouse cursor is
        QAction *selectedAction = contextMenu.exec(layerList->mapToGlobal(pos));

        if (!selectedAction) return; // They clicked away without choosing

        // ==========================================
        // THE REVERSE MATH (Calculate once!)
        // ==========================================
        int uiRow = layerList->row(item);
        int engineIndex = (layerList->count() - 1) - uiRow;

        // ACTION 1: RENAME
        if (selectedAction == renameAction) {
            layerList->editItem(item);
        } 
        // ACTION 2: PROPERTIES (OPACITY SLIDER)
        else if (selectedAction == propertiesAction) {
            
            QDialog dialog(this);
            dialog.setWindowTitle(item->text() + " Properties");
            dialog.setMinimumWidth(250);
            
            QVBoxLayout layout(&dialog);

            // Fetch using engineIndex!
            float currentOpacity = canvas->getLayerManager().getLayers()[engineIndex]->opacity;
            int currentPercentage = static_cast<int>(currentOpacity * 100);

            QLabel label(QString("Opacity: %1%").arg(currentPercentage));
            QSlider slider(Qt::Horizontal);
            slider.setRange(0, 100);
            slider.setValue(currentPercentage);

            QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply);

            layout.addWidget(&label);
            layout.addWidget(&slider);
            layout.addWidget(&buttonBox);

            connect(&slider, &QSlider::valueChanged, this, [&label](int value){
                label.setText(QString("Opacity: %1%").arg(value));
            });

            // Capture engineIndex in these lambdas!
            connect(buttonBox.button(QDialogButtonBox::Apply), &QPushButton::clicked, this, [this, &slider, engineIndex]() {
                canvas->setLayerOpacity(engineIndex, slider.value());
            });

            connect(&buttonBox, &QDialogButtonBox::accepted, this, [this, &dialog, &slider, engineIndex]() {
                canvas->setLayerOpacity(engineIndex, slider.value());
                dialog.accept(); 
            });

            connect(&buttonBox, &QDialogButtonBox::rejected, this, [&dialog]() {
                dialog.reject(); 
            });

            dialog.exec();
        }
        // ==========================================
        // ACTION 3: FILTER (DYNAMIC DROPDOWN)
        // ==========================================
        else if (selectedAction == filterAction)
        {
            // Set the UI visually using uiRow, but tell the core engine to use engineIndex
            layerList->setCurrentRow(uiRow);
            canvas->setActiveLayer(engineIndex);

            QDialog dialog(this);
            dialog.setWindowTitle("Apply Filter");
            dialog.setMinimumWidth(300);

            QVBoxLayout layout(&dialog);

            // 1. The Dropdown Menu
            QComboBox *filterCombo = new QComboBox(&dialog);
            filterCombo->addItems({"Gaussian Blur",
                                   "Sharpen",
                                   "Edge Detection",
                                   "Emboss",
                                   "Invert Colors",
                                   "Grayscale",
                                   "Brightness / Contrast"});
            layout.addWidget(new QLabel("Select Filter:"));
            layout.addWidget(filterCombo);

            // 2. The Stacked UI
            QStackedWidget *stackedParams = new QStackedWidget(&dialog);

            // --- PAGE 0: Gaussian Blur ---
            QWidget *pageGaussian = new QWidget();
            QFormLayout *layoutGaussian = new QFormLayout(pageGaussian);
            QSpinBox *sizeSpin = new QSpinBox();
            sizeSpin->setRange(3, 51);
            sizeSpin->setSingleStep(2);
            sizeSpin->setValue(5);
            QDoubleSpinBox *sigmaSpin = new QDoubleSpinBox();
            sigmaSpin->setRange(0.1, 15.0);
            sigmaSpin->setSingleStep(0.5);
            sigmaSpin->setValue(1.0);
            layoutGaussian->addRow("Kernel Size:", sizeSpin);
            layoutGaussian->addRow("Sigma:", sigmaSpin);
            stackedParams->addWidget(pageGaussian);

            // --- PAGE 1: Convolution Intensity ---
            QWidget *pageIntensity = new QWidget();
            QFormLayout *layoutIntensity = new QFormLayout(pageIntensity);
            QDoubleSpinBox *intensitySpin = new QDoubleSpinBox();
            intensitySpin->setRange(0.0, 5.0);
            intensitySpin->setSingleStep(0.1);
            intensitySpin->setValue(1.0);
            layoutIntensity->addRow("Intensity:", intensitySpin);
            stackedParams->addWidget(pageIntensity);

            // --- PAGE 2: Blend Amount ---
            QWidget *pageBlend = new QWidget();
            QFormLayout *layoutBlend = new QFormLayout(pageBlend);
            QDoubleSpinBox *blendSpin = new QDoubleSpinBox();
            blendSpin->setRange(0.0, 1.0);
            blendSpin->setSingleStep(0.1);
            blendSpin->setValue(1.0);
            layoutBlend->addRow("Blend Amount:", blendSpin);
            stackedParams->addWidget(pageBlend);

            // --- PAGE 3: Brightness & Contrast ---
            QWidget *pageBC = new QWidget();
            QFormLayout *layoutBC = new QFormLayout(pageBC);
            QDoubleSpinBox *brightSpin = new QDoubleSpinBox();
            brightSpin->setRange(-255.0, 255.0);
            brightSpin->setValue(0.0);
            QDoubleSpinBox *contrastSpin = new QDoubleSpinBox();
            contrastSpin->setRange(-255.0, 255.0);
            contrastSpin->setValue(0.0);
            layoutBC->addRow("Brightness:", brightSpin);
            layoutBC->addRow("Contrast:", contrastSpin);
            stackedParams->addWidget(pageBC);

            layout.addWidget(stackedParams);

            // 3. Connect Dropdown to Stacked UI
            connect(filterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [stackedParams](int index)
                    {
                        if (index == 0) stackedParams->setCurrentIndex(0); 
                        else if (index >= 1 && index <= 3) stackedParams->setCurrentIndex(1); 
                        else if (index >= 4 && index <= 5) stackedParams->setCurrentIndex(2); 
                        else if (index == 6) stackedParams->setCurrentIndex(3); 
                    });

            // 4. Buttons
            QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
            layout.addWidget(&buttonBox);

            // 5. Apply the math asynchronously (Capture engineIndex here!)
            connect(&buttonBox, &QDialogButtonBox::accepted, this, [this, &dialog, filterCombo, sizeSpin, sigmaSpin, intensitySpin, blendSpin, brightSpin, contrastSpin, engineIndex]()
                    {
                
                int filterType = filterCombo->currentIndex();
                int size = sizeSpin->value();
                float sigma = sigmaSpin->value();
                float intensity = intensitySpin->value();
                float blend = blendSpin->value();
                float bright = brightSpin->value();
                float contrast = contrastSpin->value();
                
                QProgressDialog *progress = new QProgressDialog("Calculating Filter...", nullptr, 0, 0, this);
                progress->setWindowModality(Qt::WindowModal);
                progress->setAttribute(Qt::WA_DeleteOnClose);
                progress->show();

                QFutureWatcher<void> *watcher = new QFutureWatcher<void>(this);
                
                connect(watcher, &QFutureWatcher<void>::finished, this, [this, progress, watcher]() {
                    progress->close();
                    watcher->deleteLater();
                });

                QFuture<void> future = QtConcurrent::run([this, engineIndex, filterType, size, sigma, intensity, blend, bright, contrast]()
                                                         {

    std::unique_ptr<Filter> activeFilter;

    switch(filterType) {
        case 0: activeFilter = std::make_unique<GaussianBlurFilter>(size, sigma); break;
        case 1: activeFilter = std::make_unique<SharpenFilter>(intensity); break;
        case 2: activeFilter = std::make_unique<EdgeDetectionFilter>(intensity); break;
        case 3: activeFilter = std::make_unique<EmbossFilter>(intensity); break;
        case 4: activeFilter = std::make_unique<InvertFilter>(blend); break;
        case 5: activeFilter = std::make_unique<GrayscaleFilter>(blend); break;
        case 6: activeFilter = std::make_unique<BrightnessContrastFilter>(bright, contrast); break;
    }

    // Route through applyFilter() so beginBatch/endBatch and cache
    // invalidation run correctly. The active layer index must match
    // engineIndex, so we temporarily switch it, apply, then restore.
    if (activeFilter) {
        LayerManager &manager = this->canvas->getLayerManager();
        size_t previousIndex = manager.getActiveLayerIndex();
        manager.setActiveLayer(engineIndex);
        manager.applyFilter(*activeFilter);      // ← goes through the public API
        manager.setActiveLayer(previousIndex);   // restore what the user had selected
    } });

                watcher->setFuture(future);
                dialog.accept(); });

            connect(&buttonBox, &QDialogButtonBox::rejected, this, [&dialog]()
                    { dialog.reject(); });

            dialog.exec();
        } });
    // Saving and Loading
    // ==========================================
    // EXPORT TO PNG (Ctrl+E)
    // ==========================================
    QAction *exportAction = toolbar->addAction("Export PNG");
    QShortcut *exportShortcut = new QShortcut(QKeySequence("Ctrl+E"), this);

    // A lambda to handle the export so both the button and shortcut can use it
    auto triggerExport = [this]()
    {
        // Default to the user's Pictures folder
        QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + "/artwork.png";

        QString filePath = QFileDialog::getSaveFileName(
            this,
            "Export Image",
            defaultPath,
            "PNG Image (*.png)");

        if (!filePath.isEmpty())
        {
            // Force the .png extension if the user forgot to type it
            if (!filePath.endsWith(".png", Qt::CaseInsensitive))
            {
                filePath += ".png";
            }

            if (canvas->exportToPNG(filePath))
            {
                QMessageBox::information(this, "Success", "Image exported successfully!");
            }
            else
            {
                QMessageBox::critical(this, "Error", "Failed to export the image.");
            }
        }
    };

    // Connect both the toolbar button and the keyboard shortcut to the lambda
    connect(exportAction, &QAction::triggered, this, triggerExport);
    connect(exportShortcut, &QShortcut::activated, this, triggerExport);

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
    // ==========================================
    // LOAD NATIVE PROJECT (Ctrl+O)
    // ==========================================
    QShortcut *openShortcut = new QShortcut(QKeySequence::Open, this);
    connect(openShortcut, &QShortcut::activated, this, [this]()
            {
        QString filePath = QFileDialog::getOpenFileName(this, "Open Project", "", "Paint Project (*.ptproj)");

        if (!filePath.isEmpty()) {
            try {
                if (!canvas->getLayerManager().loadProject(filePath.toStdString())) {
                    QMessageBox::critical(this, "Error", "File is corrupted or not a valid project.");
                } else {
                    canvas->syncScrollbars();
                    canvas->update();
                }
            } catch (const std::exception& e) {
                // If the C++ Core Engine throws a fatal error, catch it here!
                QMessageBox::critical(this, "Fatal Crash Prevented", "This save file is incompatible with the current engine version.");
            }
        } });
    // Note the captured variables in the brackets: [this, layerList, createLayerItem]
    // Note the captured variables in the brackets: [this, layerList, createLayerItem]
    connect(canvas, &Canvas::coreLayerListChanged, this, [this, layerList, createLayerItem]()
            {
        // 1. Temporarily block signals so clearing the list doesn't trigger active layer changes
        layerList->blockSignals(true);
        
        // 2. Clear the Qt List Widget completely
        layerList->clear(); 

        // 3. Ask the core engine for the current layers
        const auto& layers = canvas->getLayerManager().getLayers();

        // 4. REBUILD THE LIST IN REVERSE!
        // We use insertItem(0, ...) so that as we loop from engine index 0 upwards,
        // each subsequent layer pushes the previous one down, perfectly matching Photoshop's UI!
        for (size_t i = 0; i < layers.size(); ++i)
        {
            QListWidgetItem *item = createLayerItem(QString::fromStdString(layers[i]->name));

            // Restore visibility checkboxes
            item->setCheckState(layers[i]->visible ? Qt::Checked : Qt::Unchecked);

            // Force it to the top of the UI list
            layerList->insertItem(0, item);
        }

        // 5. Set the UI selection to match the active layer
        // Since we inverted the visual stack, we also invert the selection index!
        layerList->setCurrentRow(layers.size() - 1 - canvas->getLayerManager().getActiveLayerIndex());
        
        // 6. Unblock signals so the user can click on layers again
        layerList->blockSignals(false); });

    connect(canvas, &Canvas::coreColorChanged, this, [this, colorAction](int r, int g, int b, int a)
            {
                QPixmap pixmap(24, 24);
                
                // 2. Fill with white first so semi-transparent colors are actually visible
                pixmap.fill(Qt::white); 
                
                // 3. Paint the new color over the top
                QPainter painter(&pixmap);
                painter.fillRect(pixmap.rect(), QColor(r, g, b, a));
                
                // 4. THIS is the missing line! Update the actual toolbar icon.
                currentToolColor = QColor(r, g, b, a);
                colorAction->setIcon(QIcon(pixmap)); });

    if (!filePath.isEmpty())
    {
        try
        {
            // Ask the engine to load the file
            if (canvas->getLayerManager().loadProject(filePath.toStdString()))
            {
                canvas->syncScrollbars();
                canvas->update();
            }
            else
            {
                QMessageBox::critical(this, "Error", "Failed to load the startup project.");
            }
        }
        catch (...)
        {
            QMessageBox::critical(this, "Error", "Corrupted project file.");
        }
    }
    adjustSize();
}