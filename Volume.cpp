#include "Volume.h"

#include <qmath.h>

Volume::~Volume()
{
    if(volData != nullptr) {
        delete [] volData;
    }
}

uint8_t *Volume::voxelAt(int x, int y, int z)
{
    x = qBound(0, x, (int)width-1);
    y = qBound(0, y, (int)height-1);
    z = qBound(0, z, (int)depth-1);
    int index = x + y*width + z*width*height;

    return &volData[index*bytesPerCell];
}

uint8_t *Volume::voxelAt(unsigned i)
{
    return &volData[i*bytesPerCell];
}

uint8_t *Volume::voxelAt(int x, int y, int z) const
{
    x = qBound(0, x, (int)width-1);
    y = qBound(0, y, (int)height-1);
    z = qBound(0, z, (int)depth-1);
    int index = x + y*width + z*width*height;

    return &volData[index*bytesPerCell];
}

uint8_t *Volume::voxelAt(unsigned i) const
{
    return &volData[i*bytesPerCell];
}


int Volume::densityAt(int x, int y, int z) const
{
    if(bytesPerCell == 1) {
        return int(*voxelAt(x, y, z));
    } else {
        return int(*(uint16_t*)(voxelAt(x, y, z)));
    }
}

int Volume::densityAt(unsigned i) const
{
    if(bytesPerCell == 1) {
        return int(*voxelAt(i));
    } else {
        return int(*(uint16_t*)(voxelAt(i)));
    }
}

const uint8_t *Volume::getData() const
{
    return volData;
}

const uint8_t *Volume::getSlice(int z) const
{
    return &volData[z*width*height*bytesPerCell];
}

void Volume::setVolData(unsigned width, unsigned height, unsigned depth, int bitDepth, uint8_t *data)
{
    this->bitDepth = bitDepth;
    this->bytesPerCell = qCeil(bitDepth/8.);
    
    
    this->width = width;
    this->height = height;
    this->depth = depth;
    
    if(data != nullptr) {
        if(volData != nullptr) {
            delete[] volData;
        }
        
        volData = data;
        
        qDebug("Dimensions: %d x %d x %d", width, height, depth);
        
        emit volDataChanged();
    }
}
