#include "SliceWidget.h"

#include <QPaintEvent>
#include <QPainter>
#include <qmath.h>
#include <qendian.h>

SliceWidget::SliceWidget(Volume &volume, QWidget *parent) :
    QWidget(parent), vol(volume)
{
    grayscale_lut.reserve(256);
    for(int i=0; i<256; ++i) {
        grayscale_lut << qRgb(i, i, i);
    }
}

bool SliceWidget::setSlice(int z)
{
    slice = qBound(0, z, (int)vol.depth-1);
    emit sliceChanged(z);
    
    update();
    
    if(z < 0 || z >= (int)vol.depth-1) {
        return false;
    } else {
        return true;
    }
}

void SliceWidget::setApplyLut(bool use)
{
    applyLut = use;
    update();
}

void SliceWidget::updateLut(unsigned len, uint32_t *data)
{
    lut = data;
    update();
}

void SliceWidget::setBackgroundColor(QColor color)
{
    backgroundColor = color;
}

void SliceWidget::paintEvent(QPaintEvent *e)
{
    e->accept();

    if(vol.getData() == nullptr) {
        return;
    }

    QPainter p(this);

    buffer = QImage(vol.width, vol.height, QImage::QImage::Format_ARGB32);
    
    
    for(unsigned x=0; x<vol.width; ++x) {
        for(unsigned y=0; y<vol.height; ++y) {
            int density = vol.densityAt(x, y, slice);
            QRgb col;
            
            if(applyLut) {
                int wColor;
                if(vol.getBytesPerCell() == 1) {
                    wColor = lut[int(density/255.*4096)];
                } else {
                    wColor = lut[int(density/65536.*4096)];
                }
                
                double alpha = qAlpha(wColor)/255.;
                
                col = qRgba(qBlue(wColor)*alpha, qGreen(wColor)*alpha, qRed(wColor)*alpha, alpha*255);
            } else {
                int val = density;
                
                if(vol.getBytesPerCell() == 2) {
                    val = double(density/65536.)*255;
                }
                
                col = qRgb(val, val, val);
            }
            
            buffer.setPixel(x, y, col);
        }
    }
    
    p.fillRect(0, 0, width(), height(), backgroundColor);
    
    p.drawImage(0, 0, buffer.scaled(width(), height(), Qt::AspectRatioMode::KeepAspectRatio));
}

void SliceWidget::wheelEvent(QWheelEvent *e)
{
    e->accept();
    if(e->delta() < 0) {
        setSlice(slice-1);
    } else {
        setSlice(slice+1);
    }
}
