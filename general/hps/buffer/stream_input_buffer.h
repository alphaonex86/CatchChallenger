#ifndef JL2922_HPS_STREAM_INPUT_BUFFER_H
#define JL2922_HPS_STREAM_INPUT_BUFFER_H

#include <cstring>
#include <fstream>
#include "../serializer.h"

namespace hps {

#ifdef CATCHCHALLENGER_CLIENT
constexpr size_t STREAM_INPUT_BUFFER_SIZE = 65536;
#else
constexpr size_t STREAM_INPUT_BUFFER_SIZE = 65536;//65536 -> to load map of 256*256
#endif

class StreamInputBuffer {
 public:
  StreamInputBuffer(std::istream& stream) : stream(&stream) {
    stream.seekg(0, stream.beg);
    contentSize=0;
    load();
  }

  void read(char* content, size_t length) {
      /*if(length<=contentSize)
      {
        read_core(content, length);
        return;
      }*/
    while(pos + length > STREAM_INPUT_BUFFER_SIZE)
    {
      const size_t n_avail = STREAM_INPUT_BUFFER_SIZE - pos;
      //std::cerr << "need " << length << "bytes, get n_avail from buffer" << std::endl;
      read_core(content, n_avail);
      length -= n_avail;
      content += n_avail;
      //std::cerr << "need " << length << "bytes" << std::endl;
      if(!load())
          break;
    }
    if(length==0)
        return;
    read_core(content, length);
  }

  char read_char() {
    if (pos == STREAM_INPUT_BUFFER_SIZE) {
      load();
    }
    const char ch = buffer[pos];
    pos++;
    contentSize--;
    return ch;
  }

  template <class T>
  StreamInputBuffer& operator>>(T& t) {
    Serializer<T, StreamInputBuffer>::parse(t, *this);
    return *this;
  }

  inline ssize_t tellg() const {
      if(stream->tellg()>0)
        return (ssize_t)stream->tellg() - (ssize_t)STREAM_INPUT_BUFFER_SIZE + (size_t)pos;
      else
        return (size_t)pos;
  }

 private:
  std::istream* const stream;

  char buffer[STREAM_INPUT_BUFFER_SIZE];

  size_t pos;
  size_t contentSize;

  void read_core(char* content, const size_t length) {
      if(length>contentSize)
      {
          std::cerr << "try read unset zone, try read after the end of file?, abort()" << std::endl;
          abort();
      }
    memcpy(content, buffer + pos, length);
    pos += length;
    contentSize -= length;
  }

  bool load() {
      pos = 0;
      contentSize = 0;
      if(stream->eof())
          return false;
      const ssize_t oldpos=stream->tellg();
      if(oldpos<0)
      {
          std::cerr << "stream->tellg(): " << stream->tellg() << " then buggy file, abort()" << std::endl;
          abort();
      }
      stream->read(buffer, STREAM_INPUT_BUFFER_SIZE);
      contentSize = stream->gcount();
      const ssize_t temppos=stream->tellg();
      if ( (stream->rdstate() & std::ifstream::failbit ) != 0 )
      {
          if(stream->eof())
              return false;
          std::cerr << "stream->rdstate(): " << stream->rdstate() << " then buggy file, only read: " << stream->gcount() << ", abort()" << std::endl;
          std::cerr << stream->good() << " " << stream->eof() << " " << stream->fail() << " " << stream->bad() << std::endl;
          abort();
      }
      if(temppos<0)
      {
          std::cerr << "stream->tellg(): " << stream->tellg() << " then buggy file, abort()" << std::endl;
          abort();
      }
      if(temppos<oldpos)
      {
          std::cerr << "temppos<oldpos: " << stream->tellg() << " then buggy file, abort()" << std::endl;
          abort();
      }
      const ssize_t size=temppos-oldpos;
      if((size_t)size>STREAM_INPUT_BUFFER_SIZE)
      {
          std::cerr << "stream->tellg(): " << stream->tellg() << "-" << oldpos << " greater than max then buggy file, abort()" << std::endl;
          abort();
      }
      if(size==0)
          std::cerr << "read 0 bytes from HPS source, maybe a problem" << std::endl;
        //std::cout << "read: " << size << " " << binarytoHexa(buffer,size) << std::endl;
    return true;
  }
};

}  // namespace hps
#endif
