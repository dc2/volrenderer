#include "LutWidget.h"

#include <random>

#include <QMouseEvent>
#include <QPainter>

using namespace std;

LutWidget::LutWidget(QWidget *parent) :
    QWidget(parent), brushColor(255, 255, 255), buffer(256, 256, QImage::Format_ARGB32), lutBuffer(256, 256, QImage::Format_ARGB32), histogramBuffer(256, 256, QImage::Format_ARGB32)
{
    for(int x=0; x<4096; ++x) {
        lut[x] = 0;
        histogram[x] = 0;
    }
    
    histogram[4096] = 1;
    
    setBaseSize(256, 256);
    setMouseTracking(true);
    setFocus();
    
    connect(this, &LutWidget::lutChanged, this, &LutWidget::redraw);
    connect(this, &LutWidget::brushWidthChanged, this, &LutWidget::redraw);
}

LutWidget::~LutWidget()
{
    if(histogram != nullptr) {
        delete[] histogram;
    }
}

void LutWidget::setBrushColor(QColor c)
{
    brushColor = c;
}

void LutWidget::setHistogramVisible(bool show)
{
    showHistogram = show;
    drawLut();
    redraw();
}

void LutWidget::setBrushWidth(int w)
{
    brushWidth = w;
}

void LutWidget::mouseMoveEvent(QMouseEvent *e)
{
    cursor = e->pos();
    int x = e->x()-border, y = e->y()-border;
    x = (x/double(lutWidth()))*4096;
    
    y = qBound(0, y, lutHeight());
    
    if(e->buttons() == Qt::MouseButton::LeftButton || e->buttons() == Qt::MouseButton::RightButton) {
        if(e->buttons() == Qt::MouseButton::RightButton) {
            x = lockedX;
        }
        
        float bw = brushWidth * 4096 / double(lutWidth());
        
        updateLutRange(x-bw, x+bw, y, brushColor);
        
        drawLut(x-bw, x+bw);
        
        emit lutChanged(4096, lut);
    }
    
    redraw();
}

void LutWidget::mousePressEvent(QMouseEvent *e)
{
    if(e->buttons() == Qt::MouseButton::MiddleButton) {
        int x = (qBound(0, e->x()-border, lutWidth())/double(lutWidth()))*4096;
        QColor col = QColor(qBlue(lut[x]), qGreen(lut[x]), qRed(lut[x]));
        brushColor = col;
        
        emit brushColorChanged(col.rgb());
    }
    
    lockedX = e->x()-border;
    mouseMoveEvent(e);
}

void LutWidget::wheelEvent(QWheelEvent *e)
{
    if(e->delta() > 0) {
        brushWidth++;
    } else if(brushWidth > 0) {
        brushWidth--;
    }
    
    emit brushWidthChanged(brushWidth);
}

void LutWidget::resizeEvent(QResizeEvent *e)
{
    e->accept();
    int w = lutWidth()+1, h = lutHeight()+1;
    
    buffer = QImage(w, h, QImage::Format_RGB32);
    lutBuffer = QImage(w, h, QImage::Format_RGB32);
    histogramBuffer = QImage(w, h, QImage::Format_RGB32);
    
    drawHistogram();
    drawLut();
    redraw();
}

void LutWidget::paintEvent(QPaintEvent *e)
{
    e->accept();
    QPainter p(this);
    drawAxes(p);
    p.drawImage(border, border, buffer);
}

void LutWidget::calculateHistogram()
{
    const unsigned count = vol->voxelCount();

    if(vol->getData() == nullptr) {
        return;
    }

    for(unsigned i=0; i<4096; ++i) {
        histogram[i] = 0;
    }
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 10);
    
    const unsigned bitDepth = vol->getBitDepth();
    const unsigned div = qPow(2, bitDepth);

    for(unsigned i=0; i<count; i+=dis(gen)) {
        unsigned val = (vol->densityAt(i)/double(div)) * 4096;
        
        histogram[val]++;
    }

    unsigned &max = histogram[4096];
    max = 0;

    for(unsigned i=0; i<4096; ++i) {
        max = qMax(max, histogram[i]);
    }
    
    drawHistogram();
    drawLut();
}

void LutWidget::redraw()
{
    QPainter(&buffer).drawImage(0, 0, lutBuffer);
    
    drawCursor();
    
    update();
}

void LutWidget::drawCursor()
{
    int x = cursor.x()-border;
    int y = qBound(0, cursor.y()-border, lutHeight());
    
    QPainter p(&buffer);
    p.setPen(brushColor);
    
    p.drawLine(x-brushWidth, y, x+brushWidth, y);
}

void LutWidget::drawLut(int x1, int x2)
{
    QPainter p(&lutBuffer);
    
    if(showHistogram) {
        p.setBackground(QBrush(histogramBuffer));
    } else {
        p.setBackground(QBrush(QColor(0, 0, 0)));
    }
    
    unsigned h = lutBuffer.height(), w = lutBuffer.width();
    
    int _x1 = qMax(0, int((x1/4096.)*w));
    
    p.eraseRect(QRect(QPoint(_x1+1, 0), QPoint((x2/4096.)*w-1, h)));
    
    for(int x=x1; x<x2; ++x) {
        int density = lut[x];
        int y = h - qAlpha(density)/255. * h;
        int rx = (x/4096.)*w;

        p.setOpacity(.85);
        
        p.setPen(qRgb(qBlue(density), qGreen(density), qRed(density)));
        p.drawLine(rx, h, rx, y);
    }
}

void LutWidget::drawAxes(QPainter &p)
{
    p.drawLine(5, 5, 5, lutHeight()+15);
    p.drawLine(5, lutHeight()+15, lutWidth()+15, lutHeight()+15);
    
    QPen pen;
    pen.setWidth(5);
    
    p.setPen(pen);
    p.drawPoint(5, 5);
    p.drawPoint(lutWidth()+15, lutHeight()+15);
}

void LutWidget::drawHistogram()
{
    histogramBuffer.fill(QColor(0, 0, 0));
    QPainter p(&histogramBuffer);
    
    unsigned h = histogramBuffer.height();
    
    for(int x=0; x<4096; ++x) {
        int rx = (x/4096.)*histogramBuffer.width();
        
        p.setOpacity(.8);
        p.setPen(qRgb(255, 255, 255));
        
        double hist = (histogram[x]/double(histogram[4096]));
        p.drawLine(rx, h, rx, h-qLn(1+hist)/qLn(2)*h);
    }
}

void LutWidget::resetLut()
{
    for(int i=0; i<556; ++i) {
        lut[i] = 0 << 24 | 255 << 16 | 255 << 8 | 255;
    }
    
    for(int i=556; i<1687; ++i) {
        lut[i] = 9 << 24 | 26 << 16 | 130 << 8 | 192;
    }
    
    for(int i=1687; i<4078; ++i) {
        lut[i] = 255 << 24 | 255 << 16 | 255 << 8 | 255;
    }

    
    drawLut();
    emit lutChanged(4096, lut);
}

void LutWidget::updateLutRange(int x1, int x2, int y, const QColor &c)
{
    //double alpha = 255 - y/double(lutHeight());
    double alpha_255 = 255 - y/double(lutHeight())*255;
    
    for(int x=x1; x <= x2; x++) {
        int _x = qBound(0, x, 4095);
        lut[_x] = qRgba(c.blue(), c.green(), c.red(), alpha_255);
        //lut[_x] = qRgba(c.blue()*alpha, c.green()*alpha, c.red()*alpha, alpha_255);
    }
}

void LutWidget::updateVolume(const Volume *vol)
{
    this->vol = vol;
    calculateHistogram();
    redraw();
}

bool LutWidget::saveLut(const QString &filename)
{
    QFile f(filename);
    
    if(f.open(QFile::WriteOnly)) {
        f.write((char*)lut, 4096*4);
        
        f.close();
        return true;
    }

    return false;
}

bool LutWidget::loadLut(const QString &filename)
{
    QFile f(filename);
    
    if(f.exists() && f.open(QFile::ReadOnly)) {
        f.read((char*)lut, 4096*4);
        
        drawLut();
        emit lutChanged(4096, lut);
        
        f.close();
        return true;
    }

    return false;
}
