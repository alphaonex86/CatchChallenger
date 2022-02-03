#include <QCoreApplication>
#include <QDomDocument>
#include <QFile>
#include <QByteArray>
#include <QStringList>
#include <iostream>
#include <zlib.h>
#include <cstdint>

#include <zstd.h>      // presumes zstd library is installed

/* The software recompress the zlib layer to zstd, that's greatly increase the decompression speed */

void logZlibError(int error)
{
    switch (error)
    {
    case Z_MEM_ERROR:
        std::cerr << "Out of memory while (de)compressing data!" << std::endl;
        break;
    case Z_VERSION_ERROR:
        std::cerr << "Incompatible zlib version!" << std::endl;
        break;
    case Z_NEED_DICT:
    case Z_DATA_ERROR:
        std::cerr << "Incorrect zlib compressed data!" << std::endl;
        break;
    default:
        std::cerr << "Unknown error while (de)compressing data!" << std::endl;
    }
}

uint32_t decompressZlib(const char * const input, const uint32_t &intputSize, char * const output, const uint32_t &outputSize)
{
    z_stream strm;

    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.next_in = (Bytef *) input;
    strm.avail_in = intputSize;
    strm.next_out = (Bytef *) output;
    strm.avail_out = outputSize;

    int ret = inflateInit2(&strm, 15 + 32);

    if (ret != Z_OK) {
    logZlibError(ret);
    return 0;
    }

    do {
    ret = inflate(&strm, Z_SYNC_FLUSH);

    switch (ret) {
        case Z_NEED_DICT:
        case Z_STREAM_ERROR:
        ret = Z_DATA_ERROR;
        case Z_DATA_ERROR:
        case Z_MEM_ERROR:
        inflateEnd(&strm);
        logZlibError(ret);
        return 0;
    }

    if (ret != Z_OK && ret != Z_STREAM_END) {
        if((strm.next_out-reinterpret_cast<unsigned char * const>(output))>outputSize)
        {
            logZlibError(Z_STREAM_ERROR);
            return 0;
        }
        logZlibError(Z_STREAM_ERROR);
        return 0;
    }
    }
    while (ret != Z_STREAM_END);

    if (strm.avail_in != 0) {
    logZlibError(Z_DATA_ERROR);
    return 0;
    }

    inflateEnd(&strm);

    return outputSize-strm.avail_out;
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    if(argc!=2)
    {
        std::cerr << "Only accept one arguement, the tmx to compress" << std::endl;
        return -1;
    }

    char uncompressedData[65536];
    char uncompressedDataTest[65536];

    QDomDocument domDocument;
    size_t oldCompressedSize=0,newCompressedSize=0;

    //open and quick check the file
    QFile mapFile(argv[1]);
    if(!mapFile.open(QIODevice::ReadOnly))
    {
        std::cerr << mapFile.fileName().toStdString() << ": " << mapFile.errorString().toStdString() << std::endl;
        return -1;
    }
    const QByteArray &xmlContent=mapFile.readAll();
    mapFile.close();
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
    {
        std::cerr << mapFile.fileName().toStdString() << ", Parse error at line " << std::to_string(errorLine) << " column " << std::to_string(errorColumn) << ": " << errorStr.toStdString() << std::endl;
        return -1;
    }
    const tinyxml2::XMLElement &root = domDocument.RootElement();
    if(root.tagName().toStdString()!="map")
    {
        std::cerr << "\"map\" root balise not found for the xml file" << std::endl;
        return -1;
    }

    bool ok;
    //get the width
    if(!root.hasAttribute("width"))
    {
        std::cerr << "the root node has not the attribute \"width\"" << std::endl;
        return -1;
    }
    unsigned int width=root.attribute("width").toUInt(&ok);
    if(!ok)
    {
        std::cerr << "the root node has wrong attribute \"width\"" << std::endl;
        return -1;
    }

    //get the height
    if(!root.hasAttribute("height"))
    {
        std::cerr << "the root node has not the attribute \"height\"" << std::endl;
        return -1;
    }
    unsigned int height=root.attribute("height").toUInt(&ok);
    if(!ok)
    {
        std::cerr << "the root node has wrong attribute \"height\"" << std::endl;
        return -1;
    }

    // layer
    tinyxml2::XMLElement child = root.FirstChildElement("layer");
    while(!child.isNull())
    {
        if(!child.isElement())
        {
            std::cerr << "Is Element: child.tagName(): " << child.tagName().toStdString() << ", file: " << argv[1] << std::endl;
            return -1;
        }
        else if(!child.hasAttribute("name"))
        {
            std::cerr << "Has not attribute \"name\": child.tagName(): " << child.tagName().toStdString() << ", file: " << argv[1] << std::endl;
            return -1;
        }
        else
        {
            std::cout << "Do the layer: " << child.attribute("name").toStdString() << " at line " << child.lineNumber() << std::endl;
            const tinyxml2::XMLElement &data=child.FirstChildElement("data");
            const std::string name=child.attribute("name").toStdString();
            if(data.isNull())
            {
                std::cerr << "Is Element for layer is null: " << data.tagName().toStdString() << " and name: " << name << ", file: " << argv[1] << std::endl;
                return -1;
            }
            else if(!data.isElement())
            {
                std::cerr << "Is Element for layer child.tagName(): " << data.tagName().toStdString() << ", file: " << argv[1] << std::endl;
                return -1;
            }
            else if(!data.hasAttribute("encoding"))
            {
                std::cerr << "Has not attribute \"base64\": child.tagName(): " << data.tagName().toStdString() << ", file: " << argv[1] << std::endl;
                return -1;
            }
            else if(!data.hasAttribute("compression"))
            {
                std::cerr << "Has not attribute \"zlib\": child.tagName(): " << data.tagName().toStdString() << ", file: " << argv[1] << std::endl;
                return -1;
            }
            else if(data.attribute("encoding")!="base64")
            {
                std::cerr << "only encoding base64 is supported, file: file: " << argv[1] << std::endl;
                return -1;
            }
            else if(data.attribute("compression")!="zlib")
            {
                std::cerr << "Only compression zlib is supported, file: file: " << argv[1] << std::endl;
                return -1;
            }
            else
            {
                const QByteArray compressedData=QByteArray::fromBase64(data.text().toLatin1());
                oldCompressedSize+=compressedData.size();
                memset(uncompressedData,0,65536);

                char * recompressedDataGzip=new char[65536*2];
                memset(recompressedDataGzip,0,65536*2);
                char * recompressedDataZlib=new char[65536*2];
                memset(recompressedDataZlib,0,65536*2);
                /// \warning deflate not supported by tmx format

                const size_t uncompressedSize=decompressZlib(compressedData.constData(),compressedData.size(),uncompressedData,sizeof(uncompressedData));
                if(uncompressedSize!=width*height*4)
                {
                    std::cerr << "uncompressedSize!=width*height*4: " << uncompressedSize << "!=" << width << "*" << height << "*4" << std::endl;
                    abort();
                }

                QByteArray newCompressedData;
                {
                    size_t const cBuffSize = ZSTD_compressBound(uncompressedSize);

                    void* const cBuff = malloc(cBuffSize);
                    if (!cBuff)
                    {
                        std::cerr << "error to alloc: " << cBuffSize << std::endl;
                        abort();
                    }

                    size_t const cSize = ZSTD_compress(cBuff, cBuffSize, uncompressedData, uncompressedSize, 22);
                    if (ZSTD_isError(cSize)) {
                        std::cerr << "error compressing: " << ZSTD_getErrorName(cSize) << std::endl;
                        abort();
                    }

                    newCompressedData=QByteArray(static_cast<char *>(cBuff),cSize);
                    free(cBuff);
                }

                tinyxml2::XMLElement newXmlElement=domDocument.createElement("data");
                newXmlElement.setAttribute("encoding","base64");
                newXmlElement.setAttribute("compression","zstd");
                QDomText newTextElement=domDocument.createTextNode(QString(newCompressedData.toBase64()));
                newXmlElement.appendChild(newTextElement);

                data.parentNode().removeChild(data);
                child.appendChild(newXmlElement);
            }
        }
        child = child.NextSiblingElement("layer");
    }

    if(oldCompressedSize>newCompressedSize)
    {
        QFile xmlFile(argv[1]);
        if(!xmlFile.open(QIODevice::WriteOnly))
        {
            std::cerr << "Unable to open the file in write: " << argv[1] << std::endl;
            return -1;
        }
        xmlFile.write(domDocument.toByteArray(1));
        xmlFile.close();
    }

    if(oldCompressedSize>newCompressedSize)
    {
        if(oldCompressedSize>0)
            std::cout << "Size decreased by: " << (100-newCompressedSize*100/oldCompressedSize) << "%: ";
        if(oldCompressedSize<5000)
            std::cout << oldCompressedSize << "B";
        else
            std::cout << oldCompressedSize/1024 << "KB";
        std::cout << " -> ";
        if(newCompressedSize<5000)
            std::cout << newCompressedSize << "B" << std::endl;
        else
            std::cout << newCompressedSize/1024 << "KB" << std::endl;
        std::cout << std::endl;
    }
    else
    {
        if(oldCompressedSize>0)
            std::cout << "Size increased by: " << (100-oldCompressedSize*100/newCompressedSize) << "% (not saved): ";
        if(oldCompressedSize<5000)
            std::cout << oldCompressedSize << "B";
        else
            std::cout << oldCompressedSize/1024 << "KB";
        std::cout << " -> ";
        if(newCompressedSize<5000)
            std::cout << newCompressedSize << "B" << std::endl;
        else
            std::cout << newCompressedSize/1024 << "KB" << std::endl;
        std::cout << std::endl;
    }

    return 0;
}
