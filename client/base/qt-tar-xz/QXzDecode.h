/** \file QXzDecode.h
\brief To decompress a xz stream
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef QXZDECODE_H
#define QXZDECODE_H

#include <vector>
#include <string>

//comment this to check integrity of compressed file, compressed via: xz --check=crc32 YourFile
//#define XZ_DEC_ANY_CHECK

/// \brief The decode class for the xz stream
class QXzDecode
{
    public:
        /** \brief create the object to decode it
         * \param data the compressed data
         * \param maxSize the max size
         * **/
        QXzDecode(std::vector<char> data,uint64_t maxSize=0);
        /// \brief lunch the decode
        bool decode();
        /// \brief the error string
        std::string errorString();
        /// \brief the un-compressed data
        std::vector<char> decodedData();
    private:
        std::vector<char> data;
        std::string error;
        bool isDecoded;
        uint64_t maxSize;
};

#endif // QXZDECODE_H
