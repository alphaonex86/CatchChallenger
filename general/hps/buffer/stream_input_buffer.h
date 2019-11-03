#ifndef JL2922_HPS_STREAM_INPUT_BUFFER_H
#define JL2922_HPS_STREAM_INPUT_BUFFER_H

#include <cstring>
#include "../serializer.h"

namespace hps {

#ifdef CATCHCHALLENGER_CLIENT
constexpr size_t STREAM_INPUT_BUFFER_SIZE = 65536;
#else
constexpr size_t STREAM_INPUT_BUFFER_SIZE = 4096;
#endif

class StreamInputBuffer {
 public:
  StreamInputBuffer(std::istream& stream) : stream(&stream) {
    stream.seekg(0, stream.beg);
    load();
  }

  void read(char* content, size_t length) {
    while(pos + length > STREAM_INPUT_BUFFER_SIZE)
    {
      const size_t n_avail = STREAM_INPUT_BUFFER_SIZE - pos;
      read_core(content, n_avail);
      length -= n_avail;
      content += n_avail;
      load();
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

  void read_core(char* content, const size_t length) {
    memcpy(content, buffer + pos, length);
    pos += length;
  }

  void load() {
      //const size_t oldpos=stream->tellg();
      stream->read(buffer, STREAM_INPUT_BUFFER_SIZE);
      //const size_t size=stream->tellg()-oldpos;
    //std::cout << "read: " << size << " " << binarytoHexa(buffer,size) << std::endl;
    pos = 0;
  }
};

}  // namespace hps
#endif
