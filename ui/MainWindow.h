#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGLFormat>
#include <QtWidgets>

#include "Widgets/SliceWidget.h"
#include "Widgets/VolRenderer.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    
public slots:
    bool eventFilter(QObject *object, QEvent *e) override;
    
    void updateBrushWidthSlider(int val);
    void updateBrushColor(QRgb colu);
    void updateStepsizeSpinBox(double val);
    void updateLightPos(const QVector3D &pos);
    void updateVolumeInfo(const Volume *vol);
    void updateFPS();
    
private slots:
    void on_saveLutButton_clicked();
    void on_openLutButton_clicked();
    void on_loadFileButton_clicked();
    void toggleFullscreen();
    
    //void on_pushButton_2_clicked();
    //void on_pushButton_3_clicked();
    
    void init();

private:
    Ui::MainWindow *ui;
    
    Volume *vol;
    
    VolRenderer *glw;
    SliceWidget *slw;
    
    QTimer *fpsTimer;
    //QList<double> fps;
    
    QColor showColorChooser(QLineEdit &e);
};

#endif // MAINWINDOW_H
