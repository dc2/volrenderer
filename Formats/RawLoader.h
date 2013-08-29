#ifndef RAWLOADER_H
#define RAWLOADER_H

#include "Loader.h"

#include <QString>
#include <cstdint>

class RawLoader : public Loader
{
public:
    uint8_t *loadFile(const QString &filename) override;
    uint8_t *loadFile(const QString &filename, int width, int height, int depth, ByteOrder byteOrder=BO_LITTLE_ENDIAN, short bitDepth=8);
    
    RawLoader &setWidth(int width) {
        this->width = width;
        return *this;
    }
    
    RawLoader &setHeight(int height) {
        this->height = height;
        return *this;
    }
    
    RawLoader &setDepth(int depth) {
        this->depth = depth;
        return *this;
    }
    
    RawLoader &setBitDepth(int bitDepth) {
        this->bitDepth = bitDepth;
        return *this;
    }
    
    RawLoader &setByteOrder(ByteOrder byteOrder) {
        this->byteOrder = byteOrder;
        return *this;
    }
    
    /*RawLoader &setAspectratio(double x, double y, double z) {
        aX = x;
        aY = y;
        aZ = z;
        
        return *this;
    }*/
    
private:
    ByteOrder byteOrder = ByteOrder::BO_LITTLE_ENDIAN;
    //double aX, aY, aZ;

};

#endif // RAWLOADER_H
