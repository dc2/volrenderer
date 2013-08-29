#ifndef VOLUME_H
#define VOLUME_H

#include "common.h"

#include <QObject>
#include <QVector3D>

class Volume : public QObject
{
Q_OBJECT
public:
    Volume() {}
    
    ~Volume();
    
    uint8_t *voxelAt(int x, int y, int z);
    uint8_t *voxelAt(unsigned i);

    uint8_t *voxelAt(int x, int y, int z) const;
    uint8_t *voxelAt(unsigned i) const;

    int densityAt(int x, int y, int z) const;
    int densityAt(unsigned i) const;
    
    const uint8_t *getData() const;
    const uint8_t *getSlice(int z) const;
    
    unsigned getWidth() const {return width;}
    unsigned getHeight() const{return height;}
    unsigned getDepth() const {return depth;}
    unsigned voxelCount() const {return width*height*depth;}
    
    unsigned getBitDepth() const {return bitDepth;}
    unsigned getBytesPerCell() const {return qCeil(bitDepth/8);}
    
signals:
    void volDataChanged();
    
public slots:
    void setVolData(unsigned width, unsigned height, unsigned depth, int bitDepth, uint8_t *data);
    
private:
    unsigned width;
    unsigned height;
    unsigned depth;
    
    unsigned bitDepth = 8;
    unsigned bytesPerCell = 1;
    
    uint8_t *volData = nullptr;
    
    friend class VolRenderer;
    friend class SliceWidget;
};

#endif // VOLUME_H
