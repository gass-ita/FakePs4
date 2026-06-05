#include "MainWindow.h"
#include "Tool.h"
#include "ConvolutionFilter.h"
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

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle("FakePs4");

    canvas = new Canvas(this);
    setCentralWidget(canvas);

    QToolBar *toolbar = addToolBar("Tools");

    // Add Tools
    QAction *brushAction = toolbar->addAction("Brush");
    QAction *eraserAction = toolbar->addAction("Eraser");
    QAction *sprayAction = toolbar->addAction("Spray");
    QAction *cPickerAction = toolbar->addAction("Color Picker");
    QAction *rectAction = toolbar->addAction("Rectangle");
    QAction *ellipseAction = toolbar->addAction("Ellipse");
    QAction *fillAction = toolbar->addAction("Fill");

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
            { canvas->setTool(std::make_unique<BrushTool>()); });

    connect(eraserAction, &QAction::triggered, this, [this]()
            { canvas->setTool(std::make_unique<EraserTool>()); });

    connect(sprayAction, &QAction::triggered, this, [this]()
            { canvas->setTool(std::make_unique<SprayTool>()); });
    connect(cPickerAction, &QAction::triggered, this, [this]()
            { canvas->setTool(std::make_unique<ColorPickerTool>()); });

    connect(rectAction, &QAction::triggered, this, [this]()
            { canvas->setTool(std::make_unique<RectangleTool>()); });

    // When the slider moves, tell the Canvas to update the tool size
    connect(sizeSlider, &QSlider::valueChanged, this, [this](int value)
            { canvas->setToolSize(value); });
    connect(ellipseAction, &QAction::triggered, this, [this]()
            { canvas->setTool(std::make_unique<EllipseTool>()); });
    connect(fillAction, &QAction::triggered, this, [this]()
            { canvas->setTool(std::make_unique<FillTool>()); });
    // Set the default tool to Brush
    canvas->setTool(std::make_unique<BrushTool>());

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
        QAction *propertiesAction = contextMenu.addAction("Properties...");
        QAction *filterAction = contextMenu.addAction("Filter..."); // <--- NEW ACTION

        // Spawn the menu exactly where the mouse cursor is
        QAction *selectedAction = contextMenu.exec(layerList->mapToGlobal(pos));

        // ACTION 1: RENAME
        if (selectedAction == renameAction) {
            layerList->editItem(item);
        } 
        // ACTION 2: PROPERTIES (OPACITY SLIDER)
        else if (selectedAction == propertiesAction) {
            
            int row = layerList->row(item);

            QDialog dialog(this);
            dialog.setWindowTitle(item->text() + " Properties");
            dialog.setMinimumWidth(250);
            
            QVBoxLayout layout(&dialog);

            float currentOpacity = canvas->getLayerManager().getLayers()[row]->opacity;
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

            connect(buttonBox.button(QDialogButtonBox::Apply), &QPushButton::clicked, this, [this, &slider, row]() {
                canvas->setLayerOpacity(row, slider.value());
            });

            connect(&buttonBox, &QDialogButtonBox::accepted, this, [this, &dialog, &slider, row]() {
                canvas->setLayerOpacity(row, slider.value());
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
            int row = layerList->row(item);

            // Force the clicked layer to become the active one so applyFilter targets it!
            layerList->setCurrentRow(row);
            canvas->setActiveLayer(row);

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

            // 2. The Stacked UI (Changes based on dropdown)
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

            // --- PAGE 1: Convolution Intensity (Sharpen, Edge, Emboss) ---
            QWidget *pageIntensity = new QWidget();
            QFormLayout *layoutIntensity = new QFormLayout(pageIntensity);
            QDoubleSpinBox *intensitySpin = new QDoubleSpinBox();
            intensitySpin->setRange(0.0, 5.0);
            intensitySpin->setSingleStep(0.1);
            intensitySpin->setValue(1.0);
            layoutIntensity->addRow("Intensity:", intensitySpin);
            stackedParams->addWidget(pageIntensity);

            // --- PAGE 2: Blend Amount (Invert, Grayscale) ---
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
                        if (index == 0)
                            stackedParams->setCurrentIndex(0); // Gaussian
                        else if (index >= 1 && index <= 3)
                            stackedParams->setCurrentIndex(1); // Convolution (Sharpen, etc)
                        else if (index >= 4 && index <= 5)
                            stackedParams->setCurrentIndex(2); // Point (Invert, Gray)
                        else if (index == 6)
                            stackedParams->setCurrentIndex(3); // Brightness/Contrast
                    });

            // 4. Buttons
            QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
            layout.addWidget(&buttonBox);

            // 5. Apply the math asynchronously when they hit OK
            connect(&buttonBox, &QDialogButtonBox::accepted, this, [this, &dialog, filterCombo, sizeSpin, sigmaSpin, intensitySpin, blendSpin, brightSpin, contrastSpin, row]()
                    {
                
                // CRITICAL: Grab ALL values before the dialog closes and destroys the UI elements!
                int filterType = filterCombo->currentIndex();
                int size = sizeSpin->value();
                float sigma = sigmaSpin->value();
                float intensity = intensitySpin->value();
                float blend = blendSpin->value();
                float bright = brightSpin->value();
                float contrast = contrastSpin->value();
                
                // Setup Progress Bar
                QProgressDialog *progress = new QProgressDialog("Calculating Filter...", nullptr, 0, 0, this);
                progress->setWindowModality(Qt::WindowModal);
                progress->setAttribute(Qt::WA_DeleteOnClose);
                progress->show();

                QFutureWatcher<void> *watcher = new QFutureWatcher<void>(this);
                
                connect(watcher, &QFutureWatcher<void>::finished, this, [this, progress, watcher]() {
                    progress->close();
                    watcher->deleteLater();
                    
                    LayerManager& manager = canvas->getLayerManager();
                    manager.addDirtyRect(0, 0, manager.getWidth(), manager.getHeight());
                    canvas->update(); 
                });

                // Fire off to Background Thread
                QFuture<void> future = QtConcurrent::run([this, row, filterType, size, sigma, intensity, blend, bright, contrast]() {
                    
                    // Construct the correct filter based on the dropdown choice using a smart pointer
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

                    // Apply the math to the layer memory safely
                    if (activeFilter) {
                        activeFilter->apply(this->canvas->getLayerManager().getLayers()[row].get());
                    }
                });

                watcher->setFuture(future);
                dialog.accept(); });

            connect(&buttonBox, &QDialogButtonBox::rejected, this, [&dialog]()
                    { dialog.reject(); });

            dialog.exec();
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

    connect(canvas, &Canvas::coreColorChanged, this, [this, colorAction](int r, int g, int b, int a)
            {
                QPixmap pixmap(24, 24);
                
                // 2. Fill with white first so semi-transparent colors are actually visible
                pixmap.fill(Qt::white); 
                
                // 3. Paint the new color over the top
                QPainter painter(&pixmap);
                painter.fillRect(pixmap.rect(), QColor(r, g, b, a));
                
                // 4. THIS is the missing line! Update the actual toolbar icon.
                colorAction->setIcon(QIcon(pixmap)); });

    adjustSize();
}