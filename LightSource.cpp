#include "LightSource.h"

LightSource &LightSource::setAmbient(const QColor &c) {
    ambient = c;
    return *this;
}

LightSource &LightSource::setDiffuse(const QColor &c) {
    diffuse = c;
    return *this;
}

LightSource &LightSource::setSpecular(const QColor &c) {
    specular = c;
    return *this;
}

LightSource &LightSource::setPos(const QVector3D &pos)
{
    this->pos = pos;
    return *this;
}
