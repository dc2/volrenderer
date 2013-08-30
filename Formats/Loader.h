#ifndef LOADER_H
#define LOADER_H

#include <QString>
#include <qmath.h>
#include <cstdint>

class Loader
{
public:
    enum ByteOrder {
        BO_LITTLE_ENDIAN = 0,
        BO_BIG_ENDIAN = 1
    };
    
    virtual uint8_t *loadFile(const QString &filename) = 0;
    
    virtual void getDimensions(unsigned &width, unsigned &height, unsigned &depth, unsigned &bytesPerVal) const {
        width = this->width;
        height = this->height;
        depth = this->depth;
        
        bytesPerVal = qCeil(bitDepth/8.);
    }
    
    Loader &setLinearize(bool linearize) {
        this->linearize = linearize;
        return *this;
    }
    
protected:
    
    void findMinMax(int voxelCount, unsigned bytesPerValue, uint8_t *data, unsigned &min, unsigned &max);
    inline unsigned scaleHistogram(unsigned val, unsigned min, unsigned max, unsigned rangeMax);
    void normalizeData(int voxelCount, ByteOrder byteOrder, unsigned dstBytesPerVal, uint8_t *data, uint8_t *dst);
    
    unsigned width=0, height=0, depth=0;
    
    bool linearize = true;
    unsigned bitDepth;
};

#endif // LOADER_H
