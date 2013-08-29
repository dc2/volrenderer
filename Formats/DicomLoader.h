#ifndef DICOMLOADER_H
#define DICOMLOADER_H

#include "Loader.h"

class DicomLoader : public Loader
{
public:
    DicomLoader();
    virtual uint8_t *loadFile(const QString &filename) override;
};

#endif // DICOMLOADER_H
