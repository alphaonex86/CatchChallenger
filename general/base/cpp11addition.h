#ifndef CATCHCHALLENGER_CPP11ADDITION_H
#define CATCHCHALLENGER_CPP11ADDITION_H

#include <vector>
#include <string>
#include <regex>

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

std::vector<std::string> stringregexsplit(const std::string& input, const std::regex& regex) {
    // passing -1 as the submatch index parameter performs splitting
    std::sregex_token_iterator
        first{input.begin(), input.end(), regex, -1},
        last;
    return {first, last};
}

std::vector<std::string> &stringsplit(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}


std::vector<std::string> stringsplit(const std::string &s, char delim) {
    std::vector<std::string> elems;
    stringsplit(s, delim, elems);
    return elems;
}

bool stringEndsWith(std::string const &fullString, std::string const &ending) {
    if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}

std::string& stringimplode(const std::vector<std::string>& elems, char delim, std::string& s)
{
    for (std::vector<std::string>::const_iterator ii = elems.begin(); ii != elems.end(); ++ii)
    {
        s += (*ii);
        if ( ii + 1 != elems.end() ) {
            s += delim;
        }
    }

    return s;
}

std::string stringimplode(const std::vector<std::string>& elems, char delim)
{
    std::string s;
    return stringimplode(elems, delim, s);
}

#endif
