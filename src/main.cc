#include <QApplication>
#include "MainWindow.h"
#include <QDialog>
#include <QFormLayout>
#include <QSpinBox>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QFileDialog>
#include <QString>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Variables to store the user's choice
    QString fileToOpen = "";
    int canvasWidth = 1920;
    int canvasHeight = 1080;

    // ==========================================
    // 1. THE STARTUP DIALOG
    // ==========================================
    QDialog startupDialog;
    startupDialog.setWindowTitle("FakePs4 - Startup");

    QFormLayout form(&startupDialog);

    QSpinBox *widthSpin = new QSpinBox(&startupDialog);
    widthSpin->setRange(1, 8000);
    widthSpin->setValue(1920);

    QSpinBox *heightSpin = new QSpinBox(&startupDialog);
    heightSpin->setRange(1, 8000);
    heightSpin->setValue(1080);

    form.addRow("Width (px):", widthSpin);
    form.addRow("Height (px):", heightSpin);

    // Add buttons
    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &startupDialog);

    // Create a custom "Open Project" button and add it to the box
    QPushButton *openBtn = new QPushButton("Open Project...");
    buttonBox.addButton(openBtn, QDialogButtonBox::ActionRole);

    form.addRow(&buttonBox);

    QObject::connect(&buttonBox, &QDialogButtonBox::accepted, &startupDialog, &QDialog::accept);
    QObject::connect(&buttonBox, &QDialogButtonBox::rejected, &startupDialog, &QDialog::reject);

    // ==========================================
    // HANDLE "OPEN PROJECT" CLICK
    // ==========================================
    QObject::connect(openBtn, &QPushButton::clicked, [&]()
                     {
        QString path = QFileDialog::getOpenFileName(&startupDialog, "Open Project", "", "Paint Project (*.ptproj)");
        if (!path.isEmpty()) {
            fileToOpen = path;        // Save the path
            startupDialog.accept();   // Close the dialog successfully!
        } });

    // Pause execution here until the user clicks a button!
    if (startupDialog.exec() != QDialog::Accepted)
    {
        return 0; // If they clicked Cancel or closed the window, exit the app cleanly.
    }

    // Grab the final width/height (will be ignored by MainWindow if fileToOpen has a valid path)
    canvasWidth = widthSpin->value();
    canvasHeight = heightSpin->value();

    // ==========================================
    // 2. START THE ACTUAL APP
    // ==========================================
    // Pass everything to MainWindow!
    MainWindow window(canvasWidth, canvasHeight, fileToOpen);
    window.show();

    return app.exec();
}