#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <chrono>

#include <QGLWidget>
#include <QGLBuffer>
#include <QGLFramebufferObject>
#include <QGLShaderProgram>
#include <QTimer>

#include "Volume.h"
#include "LightSource.h"
#include "common.h"

class VolRenderer : public QGLWidget
{
    Q_OBJECT
public:
    explicit VolRenderer(const QGLFormat &f, Volume &volume, QWidget *parent = 0);
    ~VolRenderer();
    
    double getFPS() const;
    
signals:
    void clicked();
    void dblClicked();
    
    void lightMoved(const QVector3D &pos);
    
    void volumeChanged(const Volume *vol);
    
    void xRotationChanged(int angle);
    void yRotationChanged(int angle);
    void zRotationChanged(int angle);

public slots:
    void play();
    void pause();
    
    void toggleLight(bool forceOn);
    
    void updateVolume(unsigned width, unsigned height, unsigned depth, int bitDepth, uint8_t *data);
    void updateLut(unsigned len, uint32_t *data);
    void setStepsize(double stepsize);
    
    void setRayDithering(bool dither);
    void setFront2back(bool front2back);
    
    void setBackgroundColor(const QColor &c);
    
    void setLightAmbient(const QColor &c);
    void setLightDiffuse(const QColor &c);
    void setLightSpecular(const QColor &c);
    
    void setLightPos(const QVector3D &pos);
    
    void setLightX(double x);
    void setLightY(double y);
    void setLightZ(double z);
    
    void setXRotation(int angle);
    void setYRotation(int angle);
    void setZRotation(int angle);
    
protected:
    virtual void initializeGL() override;
    virtual void resizeGL(int width, int height) override;
    virtual void paintGL() override;

    virtual void mouseMoveEvent(QMouseEvent *e) override;
    virtual void mousePressEvent(QMouseEvent *e) override;
    virtual void mouseDoubleClickEvent(QMouseEvent *e) override;
    virtual void wheelEvent(QWheelEvent *e) override;
    
    virtual void resizeEvent(QResizeEvent *e) override;
    
private slots:
    void uploadVolumeTexture();
    void uploadLutTexture(int len = 256);
    
    void updateLight();
    
    void idle();

private:
    void calculateFPS();
    
    bool loadShader(QGLShaderProgram &raycastShader, const QString &vertexShaderPath, const QString &fragmentShaderPath);

    void initVolumeTexture();
    void initLutTexture();
    
    void initVertexArrayObjects();
    
    void renderCube(bool front);
    void raycast();
    
    QTimer *timer;
    int frameCount = 0;
    chrono::steady_clock::time_point currentTime, previousTime;
    float fps = 0;
    
    QPoint lastPos;
    int xRot = 0, yRot = 0, zRot = 0;
    
    QVector3D volumePosition = {0, 0, 0};
    
    double zoom = 1;
    
    QVector3D scale = {1, 1, 1};
    float stepsize = 0.003;
    
    bool rayDithering = false;
    bool front2back = true;
    
    QColor backgroundColor = QColor(255, 255, 255);
    
    Volume &vol;
    LightSource light;
    
    uint32_t *lut = nullptr;

    unsigned textureId;
    unsigned lutTextureId;

    QGLShaderProgram raycastShader;
    QGLShaderProgram directionShader;
    
    QGLFramebufferObject *frameBufferFront = nullptr;
    QGLFramebufferObject *frameBufferBack = nullptr;
    
    QGLBuffer rectVertexBuffer;
    QGLBuffer rectTexCoordBuffer;
    
    QGLBuffer cubeVertexBuffer;
    
    QMatrix4x4 projection;
    QMatrix4x4 view;
    QMatrix4x4 model;
    QMatrix4x4 mvp;
};

#endif // GLWIDGET_H
