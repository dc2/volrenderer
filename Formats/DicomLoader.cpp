#include "DicomLoader.h"

#include <QDebug>

#include "lib/imebra/include/imebra.h"

DicomLoader::DicomLoader()
{
}

uint32_t *DicomLoader::loadFile(const QString &filename)
{
    using namespace puntoexe;
    
    ptr<stream> readStream(new stream);
    readStream->openFile(filename.toStdWString(), std::ios::in);
    ptr<streamReader> reader(new streamReader(readStream));
    ptr<imebra::dataSet> loadedDataSet = 
            imebra::codecs::codecFactory::getCodecFactory()->load(reader);
    
    // Get the first image. We use it in case there isn't any presentation VOI/LUT
    //  and we have to calculate the optimal one
    ptr<imebra::image> dataSetImage(loadedDataSet->getImage(0));
    dataSetImage->getSize(&width, &height);
    
    using namespace puntoexe::imebra;
    
    // Build the transforms chain
    ptr<transforms::transformsChain> chain(new transforms::transformsChain);

    ptr<transforms::modalityVOILUT> modalityVOILUT(new transforms::modalityVOILUT(loadedDataSet));
    chain->addTransform(modalityVOILUT);

    ptr<transforms::colorTransforms::colorTransformsFactory> colorFactory(transforms::colorTransforms::colorTransformsFactory::getColorTransformsFactory());
    if(colorFactory->isMonochrome(dataSetImage->getColorSpace()))
    {
        // Convert to MONOCHROME2 if a modality transform is not present
        ////////////////////////////////////////////////////////////////
        if(modalityVOILUT->isEmpty())
        {
            ptr<transforms::colorTransforms::colorTransform> monochromeColorTransform(colorFactory->getTransform(dataSetImage->getColorSpace(), L"MONOCHROME2"));
            if(monochromeColorTransform != 0)
            {
                chain->addTransform(monochromeColorTransform);
            }
        }

        ptr<transforms::VOILUT> presentationVOILUT(new transforms::VOILUT(loadedDataSet));
        imbxUint32 firstVOILUTID(presentationVOILUT->getVOILUTId(0));
        if(firstVOILUTID != 0)
        {
            presentationVOILUT->setVOILUT(firstVOILUTID);
        }
        else
        {
            // Run the transform on the first image
            ///////////////////////////////////////
            ptr<image> temporaryImage = chain->allocateOutputImage(dataSetImage, width, height);
            chain->runTransform(dataSetImage, 0, 0, width, height, temporaryImage, 0, 0);

            // Now find the optimal VOILUT
            //////////////////////////////
            presentationVOILUT->applyOptimalVOI(temporaryImage, 0, 0, width, height);
        }
        chain->addTransform(presentationVOILUT);
    }
        
    try {
        // Scan through the frames to count slices
        // ugly workaround hack because slice count can not be determined
        for(int frameNumber = 0; ; ++frameNumber)
        {
            if(frameNumber != 0)
            {
                dataSetImage = loadedDataSet->getImage(frameNumber);
            }

            ++depth;
        }
    }
    catch(imebra::dataSetImageDoesntExist&e)
    {
        // Ignore this exception. It is thrown when we reach the end of the images list
        exceptionsManager::getMessage();
    }
    
    uint32_t *result = new uint32_t[width*height*depth];
    int planeSize = width*height;
    
    ptr<image> finalImage(new image);
    finalImage->create(width, height, image::depthU8, L"MONOCHROME2", 8);
    
    for(int i=0; i<depth; ++i) {
        dataSetImage = loadedDataSet->getImage(i);
        
        if(!chain->isEmpty())
        {
            chain->runTransform(dataSetImage, 0, 0, width, height, finalImage, 0, 0);
        }
        
        imbxUint32 rowSize, channelPixelSize, channelsNumber;
        ptr<imebra::handlers::dataHandlerNumericBase> handler = finalImage->getDataHandler(false, &rowSize, &channelPixelSize, &channelsNumber);
        uint8_t *pBuffer = handler->getMemoryBuffer();
        
        int start = i*planeSize;
        
        for(int j=0; j<planeSize; ++j) {
            result[start + j] = pBuffer[j];
        }
    }
    
    return result;
}
