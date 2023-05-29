#ifndef ZstdDecodeThread_h
#define ZstdDecodeThread_h

#include <vector>
#include <string>
#include "../../general/base/lib.h"

/// \brief to decode the xz via a thread
class DLL_PUBLIC ZstdDecode
{
        public:
                ZstdDecode();
                ~ZstdDecode();
                /// \brief to return if the error have been found
                bool errorFound();
                /// \brief to return the error string
                std::string errorString();
                /// \brief to get the decoded data
                std::vector<char> decodedData();
                /// \brief to send the data
                void setData(std::vector<char> data);
        public:
                void run();
        private:
                std::vector<char> mDataToDecode;
                std::string mErrorString;
};

#endif // QZstdDecodeThread_h
