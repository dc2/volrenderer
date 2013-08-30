#include <QKeyEvent>
#include <QCoreApplication>

#include "VolRenderer.h"

static inline void qNormalizeAngle(int &angle)
{
    while (angle < 0)
        angle += 360 * 16;
    while (angle > 360 * 16)
        angle -= 360 * 16;
}

VolRenderer::VolRenderer(const QGLFormat &f, Volume &volume, QWidget *parent)
    : QGLWidget(f, parent), vol(volume),
      rectVertexBuffer(QGLBuffer::VertexBuffer),
      rectTexCoordBuffer(QGLBuffer::VertexBuffer)
{
    makeCurrent();
    frameBufferFront = new QGLFramebufferObject(width(), height(), QGLFramebufferObject::Depth);
    frameBufferBack  = new QGLFramebufferObject(width(), height(), QGLFramebufferObject::Depth);
    
    initLutTexture();
    
    light.pos = {.2, 1.0, .9};
    light.ambient = {0, 0, 0};
    light.diffuse = {255, 255, 255};
    light.specular = {255, 255, 255};

    connect(this, &VolRenderer::lightMoved, this, &VolRenderer::setLightPos);
    connect(this, &VolRenderer::lightMoved, this, &VolRenderer::updateGL);
    
    connect(&vol, &Volume::volDataChanged, this, &VolRenderer::uploadVolumeTexture);
    
    timer = new QTimer(this);
    timer->setInterval(1);
    connect(timer, &QTimer::timeout, this, &VolRenderer::idle);
}

VolRenderer::~VolRenderer()
{
    if(frameBufferFront != nullptr) {
        delete frameBufferFront;
    }
    
    if(frameBufferBack != nullptr) {
        delete frameBufferBack;
    }
    
    if(lut != nullptr) {
        delete[] lut;
    }
}

double VolRenderer::getFPS() const
{
    return fps;
}

void VolRenderer::play()
{
    timer->start();
}

void VolRenderer::pause()
{
    timer->stop();
}

void VolRenderer::toggleLight(bool forceOn)
{
    if(forceOn) {
        light.enabled = true;
    } else {
        light.enabled = !light.enabled;
    }
    
    updateLight();
    updateGL();
}

void VolRenderer::updateVolume(unsigned width, unsigned height, unsigned depth, int bitDepth, uint8_t *data)
{
    vol.setVolData(width, height, depth, bitDepth, data);
    emit volumeChanged(&vol);
}

void VolRenderer::initVolumeTexture()
{
    glEnable(GL_TEXTURE_3D);

    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_3D, textureId);

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void VolRenderer::uploadVolumeTexture()
{
    glBindTexture(GL_TEXTURE_3D, textureId);
    
    qDebug() << vol.volData;
    
    if(vol.bytesPerCell == 1) {
        glTexImage3D(GL_TEXTURE_3D, 0, GL_R8, vol.width, vol.height, vol.depth, 0, GL_RED,
                     GL_UNSIGNED_BYTE, vol.volData);
    } else if(vol.bytesPerCell == 2) {
        glTexImage3D(GL_TEXTURE_3D, 0, GL_R16, vol.width, vol.height, vol.depth, 0, GL_RED,
                     GL_UNSIGNED_SHORT, vol.volData);
    }
}

void VolRenderer::uploadLutTexture(int len)
{
    glBindTexture(GL_TEXTURE_1D, lutTextureId);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAX_LEVEL, 0);

    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, len, 0, GL_RGBA, GL_UNSIGNED_BYTE, lut);
}

void VolRenderer::updateLight()
{
    raycastShader.bind();
    raycastShader.setUniformValue("light.enabled", light.enabled);
    raycastShader.setUniformValue("light.pos", light.pos);
    raycastShader.setUniformValue("light.ambient", light.ambient);
    raycastShader.setUniformValue("light.diffuse", light.diffuse);
    raycastShader.setUniformValue("light.specular", light.specular);
}

void VolRenderer::idle()
{
    xRot += 1;
    updateGL();
}

void VolRenderer::initLutTexture()
{
    glEnable(GL_TEXTURE_1D);

    glGenTextures(1, &lutTextureId);
    glBindTexture(GL_TEXTURE_1D, lutTextureId);
}

void VolRenderer::initVertexArrayObjects()
{
    uint vao;
    
    typedef void (APIENTRY *_glGenVertexArrays) (GLsizei, unsigned*);
    typedef void (APIENTRY *_glBindVertexArray) (unsigned);
    
    _glGenVertexArrays glGenVertexArrays;
    _glBindVertexArray glBindVertexArray;
    
    glGenVertexArrays = (_glGenVertexArrays) QGLWidget::context()->getProcAddress("glGenVertexArrays");
    glBindVertexArray = (_glBindVertexArray) QGLWidget::context()->getProcAddress("glBindVertexArray");
    
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
}

bool VolRenderer::loadShader(QGLShaderProgram &shader,
                          const QString &vertexShaderPath,
                          const QString &fragmentShaderPath)
{
    bool result = shader.addShaderFromSourceFile(QGLShader::Vertex, vertexShaderPath);
    if (!result) {
        qWarning() << shader.log();
    }
    
    result = shader.addShaderFromSourceFile(QGLShader::Fragment, fragmentShaderPath);
    if (!result) {
        qWarning() << shader.log();
    }
    
    result = shader.link();
    if (!result) {
        qWarning() << "Could not link shader program:" << shader.log();
    }
    
    return result;
}

void VolRenderer::initializeGL()
{
    #ifdef __WIN32
        initGLExt();
    #endif
    
    qDebug("OpenGL %d.%d", format().minorVersion(), format().majorVersion());
    
    if (!loadShader(raycastShader, "./data/shader/raycast.vert", "./data/shader/raycast.frag")) {
        QCoreApplication::exit();
        return;
    }
    
    if (!loadShader(directionShader, "./data/shader/directions.vert", "./data/shader/directions.frag")) {
        QCoreApplication::exit();
        return;
    }

    const vector<float> rectVertices = {
        1, -1, 0,
        1,  1, 0,
       -1,  1, 0,

       -1,  1, 0,
       -1, -1, 0,
        1, -1, 0
    };
    
    const vector<float> rectTexCoords = {
        1, 0, 0,
        1, 1, 0,
        0, 1, 0,

        0, 1, 0,
        0, 0, 0,
        1, 0, 0
    };
    
    const vector<float> cubeVertices = {
        -1,-1,-1,
        -1,-1, 1,
        -1, 1, 1,
        
         1, 1,-1,
        -1,-1,-1,
        -1, 1,-1,
        
        
         1,-1, 1,
        -1,-1,-1,
         1,-1,-1,
         
         1, 1,-1,
         1,-1,-1,
        -1,-1,-1,
        
        
        -1,-1,-1,
        -1, 1, 1,
        -1, 1,-1,
        
         1,-1, 1,
        -1,-1, 1,
        -1,-1,-1,
        
        
        -1, 1, 1,
        -1,-1, 1,
         1,-1, 1,
         
         1, 1, 1,
         1,-1,-1,
         1, 1,-1,
         
         
         1,-1,-1,
         1, 1, 1,
         1,-1, 1,
         
         1, 1, 1,
         1, 1,-1,
        -1, 1,-1,
        
        
         1, 1, 1,
        -1, 1,-1,
        -1, 1, 1,
        
         1, 1, 1,
        -1, 1, 1,
         1,-1, 1
    };

    initVertexArrayObjects();

    rectVertexBuffer.create();
    rectVertexBuffer.bind();
    rectVertexBuffer.setUsagePattern(QGLBuffer::StaticDraw);
    rectVertexBuffer.allocate(rectVertices.data(), rectVertices.size() * sizeof(rectVertices[0]));
    rectVertexBuffer.release();
    
    rectTexCoordBuffer.create();
    rectTexCoordBuffer.bind();
    rectTexCoordBuffer.setUsagePattern(QGLBuffer::StaticDraw);
    rectTexCoordBuffer.allocate(rectTexCoords.data(), rectTexCoords.size() * sizeof(rectTexCoords[0]));
    rectTexCoordBuffer.release();
    
    cubeVertexBuffer.create();
    cubeVertexBuffer.bind();
    cubeVertexBuffer.setUsagePattern(QGLBuffer::StaticDraw);
    cubeVertexBuffer.allocate(cubeVertices.data(), cubeVertices.size() * sizeof(cubeVertices[0]));
    cubeVertexBuffer.release();
    
    initVolumeTexture();
    
    uploadLutTexture();
    
    raycastShader.bind();
    updateLight();
    
    raycastShader.setUniformValue("volData", 0);
    raycastShader.setUniformValue("ray", 1);
    raycastShader.setUniformValue("lut", 2);
    
    raycastShader.setUniformValue("width", vol.width);
    raycastShader.setUniformValue("depth", vol.depth);
    raycastShader.setUniformValue("height", vol.height);
    raycastShader.release();
    
    setBackgroundColor(backgroundColor);
}


void VolRenderer::renderCube(bool front)
{
    if(front) {
        frameBufferFront->bind();
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
    } else {
        frameBufferBack->bind();
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
    }
    
    directionShader.bind();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    double aspect = width()/double(height());
    
    projection.setToIdentity();
    view.setToIdentity();
    model.setToIdentity();
    
    projection.ortho(-1*aspect, 1*aspect, -1, 1, 1000, -1000);
    //projection.perspective(5, aspect, .01, 100);
    //view.lookAt({0, 0, 4}, {0, 0, 0}, {0, 1, 0});
    
    model.scale(scale);
    model.scale(zoom);
    model.translate(volumePosition);

    model.rotate(xRot/16., 1, 0, 0);
    model.rotate(yRot/16., 0, 1, 0);
    model.rotate(zRot/16., 0, 0, 1);

    mvp = projection * view * model;

    directionShader.setUniformValue("mvp", mvp);
    
    cubeVertexBuffer.bind();
    
    directionShader.setAttributeBuffer("vertex", GL_FLOAT, 0, 3);
    directionShader.enableAttributeArray("vertex");

    glDrawArrays(GL_TRIANGLES, 0, 36);

    cubeVertexBuffer.release();
    directionShader.release();
    
    if(front) {
        frameBufferFront->release();
    } else {
        frameBufferBack->release();
    }
}

void VolRenderer::raycast()
{
    raycastShader.bind();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, textureId);
    
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_1D, lutTextureId);
    
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, frameBufferFront->texture());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, frameBufferBack->texture());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    
    
    raycastShader.setUniformValue("stepsize", stepsize);
    raycastShader.setUniformValue("rayDithering", rayDithering);
    raycastShader.setUniformValue("front2back", front2back);
    
    raycastShader.setUniformValue("volumePosition", volumePosition);
    
    raycastShader.setUniformValue("width", vol.width);
    raycastShader.setUniformValue("height", vol.height);
    raycastShader.setUniformValue("depth", vol.depth);
    
    raycastShader.setUniformValue("backgroundColor", backgroundColor);
    
    raycastShader.setUniformValue("front", 3);
    raycastShader.setUniformValue("back", 4);
    
    rectVertexBuffer.bind();
    
    raycastShader.setAttributeBuffer("vertex", GL_FLOAT, 0, 3);
    raycastShader.enableAttributeArray("vertex");
    
    rectTexCoordBuffer.bind();
    
    raycastShader.setAttributeBuffer("vertexTexCoord", GL_FLOAT, 0, 3);
    raycastShader.enableAttributeArray("vertexTexCoord");
    
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    rectVertexBuffer.release();
    rectTexCoordBuffer.release();

    raycastShader.release();
}

void VolRenderer::paintGL()
{
    if(vol.getData() == nullptr || !isEnabled() || !isVisible())
    {
        glClear(GL_COLOR_BUFFER_BIT);
        return;
    }

    renderCube(true);
    renderCube(false);
    //renderTexture();
    raycast();
    
    calculateFPS();
}

void VolRenderer::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
    
    delete frameBufferFront;
    frameBufferFront = new QGLFramebufferObject(w, h, QGLFramebufferObject::Depth);
    frameBufferFront->bind();
    

    glViewport(0, 0, w, h);
    frameBufferFront->release();
    
    delete frameBufferBack;
    frameBufferBack = new QGLFramebufferObject(w, h, QGLFramebufferObject::Depth);
    frameBufferBack->bind();
    
    glViewport(0, 0, w, h);
    frameBufferBack->release();
}

void VolRenderer::mouseMoveEvent(QMouseEvent *e)
{
    int dx = e->x() - lastPos.x();
    int dy = e->y() - lastPos.y();

    if(e->buttons() & Qt::LeftButton) {
        setXRotation(xRot - 8 * dy);
        setZRotation(zRot + 8 * dx);
    } else if(e->buttons() & Qt::RightButton) {
        setXRotation(xRot + 8 * dy);
        setYRotation(yRot + 8 * dx);
    } else if(e->buttons() & Qt::MiddleButton) {
        volumePosition += 4./zoom*QVector3D(dx/float(width()), -dy/float(height()), 0);
    }
    updateGL();
    lastPos = e->pos();
}

void VolRenderer::mousePressEvent(QMouseEvent *e)
{
    e->accept();
    lastPos = e->pos();
    
    emit clicked();
}

void VolRenderer::mouseDoubleClickEvent(QMouseEvent *e)
{
    e->accept();
    emit dblClicked();
}

void VolRenderer::wheelEvent(QWheelEvent *e)
{
    if(e->delta() > 0) {
        zoom*=1.10;
    } else {
        zoom/=1.10;
    }
    
    updateGL();
}

void VolRenderer::resizeEvent(QResizeEvent *e)
{
    resizeGL(width(), height());
}

void VolRenderer::calculateFPS()
{
    frameCount++;
    
    currentTime = chrono::steady_clock::now();

    //  Calculate time passed
    double timeInterval = (chrono::duration_cast<chrono::duration<double>>(currentTime - previousTime)).count();

    if(timeInterval > 1)
    {
        //  calculate the number of frames per second
        fps = frameCount / timeInterval;

        previousTime = currentTime;
        frameCount = 0;
    }
}

void VolRenderer::updateLut(unsigned len, uint32_t *data)
{
    lut = data;
    uploadLutTexture(len);
    updateGL();
}

void VolRenderer::setStepsize(double stepsize)
{
    this->stepsize = stepsize;
    updateGL();
}

void VolRenderer::setRayDithering(bool dither)
{
    rayDithering = dither;
    updateGL();
}

void VolRenderer::setFront2back(bool front2back)
{
    this->front2back = front2back;
    updateGL();
}

void VolRenderer::setBackgroundColor(const QColor &c)
{
    backgroundColor = c;
    glClearColor(backgroundColor.red(), backgroundColor.green(), backgroundColor.blue(), 1);
}

void VolRenderer::setLightAmbient(const QColor &c)
{
    light.setAmbient(c);
    updateLight();
}

void VolRenderer::setLightDiffuse(const QColor &c)
{
    light.setDiffuse(c);
    updateLight();
}

void VolRenderer::setLightSpecular(const QColor &c)
{
    light.setSpecular(c);
    updateLight();
}

void VolRenderer::setLightPos(const QVector3D &pos)
{
    light.setPos(pos);
    updateLight();
}

void VolRenderer::setLightX(double x)
{
    light.pos.setX(x/10.);
    emit lightMoved(light.pos);
}

void VolRenderer::setLightY(double y)
{
    light.pos.setY(y/10.);
    emit lightMoved(light.pos);
}

void VolRenderer::setLightZ(double z)
{
    light.pos.setZ(z/10.);
    emit lightMoved(light.pos);
}

void VolRenderer::setXRotation(int angle)
{
    qNormalizeAngle(angle);
    if (angle != xRot) {
        xRot = angle;
        emit xRotationChanged(angle);
    }
}

void VolRenderer::setYRotation(int angle)
{
    qNormalizeAngle(angle);
    if (angle != yRot) {
        yRot = angle;
        emit yRotationChanged(angle);
    }
}

void VolRenderer::setZRotation(int angle)
{
    qNormalizeAngle(angle);
    if (angle != zRot) {
        zRot = angle;
        emit zRotationChanged(angle);
    }
}
