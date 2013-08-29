#ifndef OPENWIZARD_H
#define OPENWIZARD_H

#include <QWizard>

namespace Ui {
class OpenWizard;
}

class OpenWizard : public QWizard
{
    Q_OBJECT
    
public:
    enum Loader {
        Loader_RAW = 0,
        Loader_DDS = 1,
        Loader_Dicom = 2
    };
    
    explicit OpenWizard(QWidget *parent = 0);
    ~OpenWizard();
    
    int getRawWidth() const;
    int getRawHeight() const;
    int getRawDepth() const;
    int getRawBitdepth() const;
    int getByteOrder() const;
    bool getRawNormalize() const;
    
    
    Loader getFormat() const {return format;}
    QString getFilename() const {return filename;}
    
private slots:
    void on_OpenWizard_currentIdChanged(int id);
    void on_OpenWizard_finished(int result);
    
    void on_format_doubleClicked();
    
private:
    QString filename;
    Loader format=Loader::Loader_RAW;
    
    Ui::OpenWizard *ui;
};

#endif // OPENWIZARD_H
