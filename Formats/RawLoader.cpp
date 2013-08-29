#include "Formats/RawLoader.h"

#include <QFile>
#include <QVector3D>
#include <QDebug>

#include <qmath.h>

uint8_t *RawLoader::loadFile(const QString &filename)
{
    return loadFile(filename, width, height, depth, byteOrder, bitDepth);
}

uint8_t *RawLoader::loadFile(const QString &filename, int width, int height, int depth, ByteOrder byteOrder, short bitDepth)
{
    QFile f(filename);
    
    this->width = width;
    this->height = height;
    this->depth = depth;
    this->bitDepth = bitDepth;
    
    int bytesPerValue = qCeil(bitDepth/8.);
    
    int voxelCount = width*height*depth;
    int bytesToRead = voxelCount*bytesPerValue;
    
    uint8_t *raw = new uint8_t[bytesToRead];
    uint8_t *dst = new uint8_t[bytesToRead];
    
    if(f.exists() && f.open(QFile::ReadOnly)) {
        f.read((char*)raw, bytesToRead);
        
        f.close();
        
        normalizeData(voxelCount, byteOrder, bytesPerValue, raw, dst);
        delete[] raw;
        
        return dst;
    }
    
    return nullptr;
}
