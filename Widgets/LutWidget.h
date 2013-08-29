#ifndef TESTWIDGET_H
#define TESTWIDGET_H

#include <QImage>
#include <QWidget>

#include "Volume.h"

class LutWidget : public QWidget
{
    Q_OBJECT
public:
    explicit LutWidget(QWidget *parent = 0);
    QRgb *getLut() const {return lut;}
    ~LutWidget();
    
signals:
    void brushWidthChanged(int val);
    void brushColorChanged(QRgb col);
    void lutChanged(unsigned len, QRgb *data);
    
public slots:
    void setBrushWidth(int w);
    void setBrushColor(QColor c);
    void setHistogramVisible(bool show);
    void resetLut();
    void updateLutRange(int x1, int x2, int y, const QColor &c);
    void updateVolume(const Volume *vol);
    
    bool saveLut(const QString &filename);
    bool loadLut(const QString &filename);
    
private:
    void mouseMoveEvent(QMouseEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void wheelEvent(QWheelEvent *e) override;
    
    void resizeEvent(QResizeEvent *e) override;
    
    void paintEvent(QPaintEvent * e) override;

    void calculateHistogram();
    
    void redraw();
    void drawCursor();
    void drawLut(int x1=0, int x2=4096);
    void drawAxes(QPainter &p);
    void drawHistogram();
    
    int lutWidth() const {return width()-2*border;}
    int lutHeight() const {return height()-2*border;}

    const Volume *vol = nullptr;
    unsigned *histogram = new unsigned[4097];
    
    bool showHistogram = true;
    
    QRgb *lut = new QRgb[4096];
    int lockedX;
    int brushWidth = 1;
    QColor brushColor;
    
    QPoint cursor;
    QImage buffer;
    QImage lutBuffer;
    QImage histogramBuffer;
    
    int border = 10;
};

#endif // TESTWIDGET_H
