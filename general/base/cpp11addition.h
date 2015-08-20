#ifndef CATCHCHALLENGER_CPP11ADDITION_H
#define CATCHCHALLENGER_CPP11ADDITION_H

struct pairhash {
public:
  template <typename T, typename U>
  std::size_t operator()(const std::pair<T, U> &x) const
  {
      return (x.first << 8) + x.second;
    //return std::hash<T>()(x.first) ^ std::hash<U>()(x.second);
  }
};

bool replace(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = str.find(from);
    if(start_pos == std::string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}

uint8_t replaceAll(std::string& str, const std::string& from, const std::string& to) {
    if(from.empty())
        return 0;
    size_t start_pos = 0;
    uint8_t count=0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
        count++;
    }
    return count;
}

#endif
