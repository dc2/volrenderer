#include "Loader.h"

#include <QDebug>
#include <qmath.h>

void Loader::findMinMax(int voxelCount, unsigned bytesPerValue, uint8_t *data, unsigned &min, unsigned &max)
{
    for(int i=0; i<voxelCount; i++) {
        unsigned int raw_val;
        
        if(bytesPerValue == 2) {
            raw_val = data[i*2] <<8 | data[i*2+1];
        } else {
            raw_val = data[i];
        }
        
        min = qMin(min, raw_val);
        max = qMax(max, raw_val);
    }
}

unsigned Loader::scaleHistogram(unsigned val, unsigned min, unsigned max, unsigned rangeMax)
{
    return (val - min) * (double(rangeMax) / (max-min));
}

void Loader::normalizeData(int voxelCount, Loader::ByteOrder byteOrder, unsigned dstBytesPerVal, uint8_t *data, uint8_t *dst)
{
    int bytesPerValue = qCeil(bitDepth/8.);
    
    unsigned int rangeMax = qPow(2, bitDepth);
    unsigned int min = rangeMax, max = 0;
    
    qDebug() << rangeMax;
    
    if(linearize) {
        findMinMax(voxelCount, bytesPerValue, data, min, max);
    }
    
    unsigned raw_val;
    unsigned val;
    
    for(int i=0; i<voxelCount; i++) {
        if(bytesPerValue == 1) {
            raw_val = raw_val = data[i];
        } else {
            uint16_t *d = (uint16_t*)data;
            raw_val = (byteOrder == BO_BIG_ENDIAN) ? (data[i*2] <<8 | data[i*2+1]) : d[i];
        }
    
        if(linearize) {
            // scale histogram to span the whole available range
            val = scaleHistogram(raw_val, min, max, rangeMax);
        } else if(bytesPerValue == 2 && dstBytesPerVal == 1) {
            val = uint8_t(raw_val/double(rangeMax)*255);
        }
        
        if(dstBytesPerVal == 1) {
            dst[i] = val;
        } else {
            ((uint16_t*)dst)[i] = val;
        }
    }
}
