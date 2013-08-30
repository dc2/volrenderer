#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "OpenWizard.h"

#include <QColorDialog>

#include "Formats/Loader.h"
#include "Formats/DDSLoader.h"
#include "Formats/RawLoader.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    
    // Specify an OpenGL 3.3 format using the Core profile.
    QGLFormat f;
    f.setVersion(3, 3);
    f.setProfile(QGLFormat::CoreProfile);
    //stdGlFormat.setDoubleBuffer(true);
    //stdGlFormat.setSampleBuffers(true);

    vol = new Volume();

    // Create a VolRenderer requesting our format
    glw = new VolRenderer(f, *vol, ui->volume);
    slw = new SliceWidget(*vol, ui->volume);
    
    glw->setMinimumSize(400, 400);
    glw->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    slw->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    
    delete ui->volumeWidget;
    delete ui->sliceWidget;
    
    ui->volumeLayout->insertWidget(0, glw);
    ui->sliceLayout->insertWidget(0, slw);
    
    ui->backgroundColor->installEventFilter(this);
    ui->brushColor->installEventFilter(this);
    ui->lightAmbientColor->installEventFilter(this);
    ui->lightDiffuseColor->installEventFilter(this);
    ui->lightSpecularColor->installEventFilter(this);
    
    glw->installEventFilter(this);
    
    connect(ui->stepsizeSpinBox, SIGNAL(valueChanged(double)), glw, SLOT(setStepsize(double)));
    
    connect(ui->lightXSpinBox, SIGNAL(valueChanged(double)), glw, SLOT(setLightX(double)));
    connect(ui->lightYSpinBox, SIGNAL(valueChanged(double)), glw, SLOT(setLightY(double)));
    connect(ui->lightZSpinBox, SIGNAL(valueChanged(double)), glw, SLOT(setLightZ(double)));
    
    connect(ui->brushSize, &QSlider::valueChanged, ui->lut, &LutWidget::setBrushWidth);
    connect(ui->resetLutButton, &QPushButton::clicked, ui->lut, &LutWidget::resetLut);
    connect(ui->histogramCheckbox, &QCheckBox::toggled, ui->lut, &LutWidget::setHistogramVisible);
    
    connect(ui->lut, &LutWidget::brushWidthChanged, this, &MainWindow::updateBrushWidthSlider);
    connect(ui->lut, &LutWidget::brushColorChanged, this, &MainWindow::updateBrushColor);
    connect(ui->lut, &LutWidget::lutChanged, glw, &VolRenderer::updateLut);
    connect(ui->lut, &LutWidget::lutChanged, slw, &SliceWidget::updateLut);
    
    
    connect(ui->applyLut, &QCheckBox::toggled, slw, &SliceWidget::setApplyLut);
    
    connect(glw, &VolRenderer::dblClicked, this, &MainWindow::toggleFullscreen);
    connect(glw, &VolRenderer::lightMoved, this, &MainWindow::updateLightPos);
    
    connect(glw, &VolRenderer::volumeChanged, this, &MainWindow::updateVolumeInfo);
    connect(glw, &VolRenderer::volumeChanged, ui->lut, &LutWidget::updateVolume);
    
    connect(slw, &SliceWidget::sliceChanged, ui->sliceSpinBox, &QSpinBox::setValue);
    connect(ui->sliceSpinBox, SIGNAL(valueChanged(int)), slw, SLOT(setSlice(int)));
    
    connect(ui->lightGroupBox, &QGroupBox::toggled, glw, &VolRenderer::toggleLight);
    connect(ui->ditheredRay, &QCheckBox::toggled, glw, &VolRenderer::setRayDithering);
    connect(ui->front2back, &QRadioButton::toggled, glw, &VolRenderer::setFront2back);
    
    fpsTimer = new QTimer(this);
    fpsTimer->setInterval(250);
    
    connect(fpsTimer, &QTimer::timeout,this, &MainWindow::updateFPS);
    
    QTimer::singleShot(150, this, SLOT(init()));
}

bool MainWindow::eventFilter(QObject *object, QEvent* e)
{
    if(e->type() == QEvent::MouseButtonDblClick && (object == ui->brushColor || object == ui->lightAmbientColor || object == ui->lightDiffuseColor || object == ui->lightSpecularColor || object == ui->backgroundColor)) {
        QWidget *w = (QWidget*)object;
        
        if(!w->isEnabled()) return false;
        
        QColor color = showColorChooser(dynamic_cast<QLineEdit&>(*object));
        
        if(color.isValid())
        {
            if(object == ui->brushColor)
            {
                ui->lut->setBrushColor(color);
            } else if(object == ui->lightAmbientColor)
            {
                glw->setLightAmbient(color);
            } else if(object == ui->lightDiffuseColor)
            {
                glw->setLightDiffuse(color);
            } else if(object == ui->lightSpecularColor)
            {
                glw->setLightSpecular(color);
            } else if(object == ui->backgroundColor)
            {
                glw->setBackgroundColor(color);
                slw->setBackgroundColor(color);
            }
        }
        
        return false;
    } else if(e->type() == QEvent::KeyPress && object == glw)
    {
        if(((QKeyEvent *)e)->key() == Qt::Key_Escape) {
            if(glw->isFullScreen()) {
                toggleFullscreen();
            }
        }
    }
    
    return false;
}

void MainWindow::updateBrushWidthSlider(int val)
{
    ui->brushSize->setValue(val);
}

void MainWindow::updateBrushColor(QRgb col)
{
    ui->brushColor->setText(QColor(col).name());
}

void MainWindow::updateStepsizeSpinBox(double val)
{
    ui->stepsizeSpinBox->setValue(val);
}

void MainWindow::updateLightPos(const QVector3D &pos)
{
    ui->lightXSpinBox->blockSignals(true);
    ui->lightYSpinBox->blockSignals(true);
    ui->lightZSpinBox->blockSignals(true);
    
    ui->lightXSpinBox->setValue(pos.x()*10.);
    ui->lightYSpinBox->setValue(pos.y()*10.);
    ui->lightZSpinBox->setValue(pos.z()*10.);
    
    ui->lightXSpinBox->blockSignals(false);
    ui->lightYSpinBox->blockSignals(false);
    ui->lightZSpinBox->blockSignals(false);
}

void MainWindow::updateVolumeInfo(const Volume *vol)
{
    unsigned width = vol->getWidth(), height = vol->getHeight(), depth = vol->getDepth();
    unsigned bytesDensities = width*height*depth;
    
    ui->volWidth ->setText(QString("%L1px").arg(width));
    ui->volHeight->setText(QString("%L1px").arg(height));
    ui->volDepth ->setText(QString("%L1px").arg(depth));
    
    ui->memoryDensities   ->setText(QString("%L1 Byte").arg(bytesDensities));
    ui->memoryDensitiesMiB->setText(QString("%L1 MiB").arg(bytesDensities/1024./1024., 16, 'f', 2));
    
    
    ui->sliceSpinBox->setMaximum(depth);
}

void MainWindow::updateFPS()
{
    double fps = glw->getFPS();
    
    //this->fps.append(fps);
    
    ui->fpsLabel->setText(QString().sprintf("%.2ffps", fps));
}

QColor MainWindow::showColorChooser(QLineEdit &e)
{
    QColorDialog d(QColor(e.text()), this);
    centralWidget()->setDisabled(true);
    d.setModal(true);
    
    d.exec();
    QColor color = d.selectedColor();
    
    if(color.isValid()) {
        e.setText(color.name());
    }
    
    d.accept();
    centralWidget()->setDisabled(false);
    return color;
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_saveLutButton_clicked()
{
    centralWidget()->setEnabled(false);
    ui->lut->saveLut(QFileDialog(this).getSaveFileName(this, "Select a place to save the LUT", "data/luts"));
    centralWidget()->setEnabled(true);
}

void MainWindow::on_openLutButton_clicked()
{
    centralWidget()->setEnabled(false);
    ui->lut->loadLut(QFileDialog(this).getOpenFileName(this, "Select a LUT", "data/luts"));
    centralWidget()->setEnabled(true);
    
    int current, last = 0, lastindex = 0;
    
    QRgb *lut = ui->lut->getLut();
    for(int i=0; i<4096; ++i) {
        current = lut[i];
        
        if(current != last && lastindex != i) {
            qDebug("for(int i=%d; i<%d; ++i) {", lastindex, i);
            qDebug("    lut[i] = %d << 24 | %d << 16 | %d << 8 | %d;", qAlpha(last), qRed(last), qGreen(last), qBlue(last));
            qDebug("}\n");
            lastindex = i;
        }
        
        last = current;
    }
}

void MainWindow::toggleFullscreen()
{
    if(glw->isFullScreen()) {
        glw->setParent(ui->volume, Qt::Widget);
        glw->showNormal();
        
        ui->volumeLayout->insertWidget(0, glw);
        ui->volumeLayout->update();
        
        glw->setFocus();
        
        showNormal();
        
    } else {
        showMinimized();
        ui->volumeLayout->removeWidget(glw);
        
        glw->setParent(nullptr);
        glw->setWindowState(Qt::WindowFullScreen|Qt::WindowActive);
        glw->showFullScreen();
        glw->setFocus();
    }
}

void MainWindow::init()
{
    fpsTimer->start();
    
    ui->lut->resetLut();
}

void MainWindow::on_loadFileButton_clicked()
{
    OpenWizard w;
    centralWidget()->setDisabled(true);
    
    w.exec();
    QString filename = w.getFilename();
    
    if(!filename.isEmpty()) {
        Loader *loader = nullptr;
        
        switch(w.getFormat()) {
        case OpenWizard::Loader::Loader_RAW: {
            loader = new RawLoader();
            RawLoader *rl = (RawLoader*)loader;
            
            rl->setWidth(w.getRawWidth());
            rl->setHeight(w.getRawHeight());
            rl->setDepth(w.getRawDepth());
            
            rl->setBitDepth(w.getRawBitdepth());
            rl->setByteOrder((Loader::ByteOrder)w.getByteOrder());
            rl->setLinearize(w.getRawNormalize());
            
            break;
        }
            
        case OpenWizard::Loader::Loader_DDS: {
            loader = new DDSLoader();
            
            break;
        }
            
        #ifdef USE_DICOM
        case OpenWizard::Loader::Loader_Dicom: {
            loader = new DicomLoader();

            break;
        }
        #endif
        }
        
        if(loader != nullptr) {
            uint8_t *data = loader->loadFile(filename);
            
            unsigned width, height, depth, bystesPerVal;
            loader->getDimensions(width, height, depth, bystesPerVal);
            
            glw->updateVolume(width, height, depth, bystesPerVal*8, data);
            
            delete loader;
        }
    }
    
    centralWidget()->setDisabled(false);
}
