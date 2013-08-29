#include "OpenWizard.h"
#include "ui_OpenWizard.h"

#include <QFileDialog>

OpenWizard::OpenWizard(QWidget *parent) :
    QWizard(parent),
    ui(new Ui::OpenWizard)
{
    ui->setupUi(this);
    #ifndef USE_DICOM
    ui->format->takeItem(2);
    #endif
}

OpenWizard::~OpenWizard()
{
    delete ui;
}

int OpenWizard::getRawWidth() const
{
    return ui->rawWidth->value();
}

int OpenWizard::getRawHeight() const
{
    return ui->rawHeight->value();
}

int OpenWizard::getRawDepth() const
{
    return ui->rawDepth->value();
}

int OpenWizard::getRawBitdepth() const
{
    return ui->rawBitdepth->value();
}

int OpenWizard::getByteOrder() const
{
    return ui->rawByteOrder->currentIndex();
}

bool OpenWizard::getRawNormalize() const
{
    return ui->rawNormalize->isChecked();
}

void OpenWizard::on_OpenWizard_currentIdChanged(int id)
{
    if(id == 1) {
        format = (Loader)ui->format->currentRow();
        
        if(format != Loader::Loader_RAW) // 0=RAW
        {
            this->accept();
        }
    }
}

void OpenWizard::on_OpenWizard_finished(int result)
{
    if(result) {
        filename = QFileDialog(this).getOpenFileName(this, QString("Select a %1 file").arg(ui->format->currentItem()->text()), "data/scans");
    }
}

void OpenWizard::on_format_doubleClicked()
{
    next();
}
