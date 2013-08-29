#ifndef LIGHTSOURCE_H
#define LIGHTSOURCE_H

#include <QObject>
#include <QVector3D>
#include <QColor>

class LightSource : public QObject
{
Q_OBJECT
    
public:
    LightSource &setAmbient(const QColor &c);
    LightSource &setDiffuse(const QColor &c);
    LightSource &setSpecular(const QColor &c);
    
    LightSource &setPos(const QVector3D &pos);
    
    bool enabled = true;
    
    QVector3D pos;
    
    QColor ambient;
    QColor diffuse;
    QColor specular;
};

#endif // LIGHTSOURCE_H
