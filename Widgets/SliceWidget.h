#ifndef SLICEWIDGET_H
#define SLICEWIDGET_H

#include <QWidget>

#include "Volume.h"

class SliceWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SliceWidget(Volume &volume, QWidget *parent = 0);
    
signals:
    void sliceChanged(int slice);
    
public slots:
    bool setSlice(int z);
    void setApplyLut(bool use);
    void updateLut(unsigned len, uint32_t *data);
    void setBackgroundColor(QColor color);
    
protected:
    void paintEvent(QPaintEvent *e) override;
    void wheelEvent(QWheelEvent *e) override;
    
private:
    QVector<QRgb> grayscale_lut;
    uint32_t *lut = nullptr;
    
    Volume &vol;
    unsigned slice = 0;
    bool applyLut = false;
    
    QColor backgroundColor = QColor(255, 255, 255);
    
    QImage buffer;
};

#endif // SLICEWIDGET_H
