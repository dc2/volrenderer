#ifndef DDSLOADER_H
#define DDSLOADER_H

#include "Loader.h"
#include <QString>

class DDSLoader : public Loader
{
public:
    uint8_t *loadFile(const QString &filename) override;

};

#endif // DDSLOADER_H
