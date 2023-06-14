#ifndef StringWithReplacement_H
#define StringWithReplacement_H

#include <string>
#include <vector>
#include <cstdint>

class StringWithReplacement
{
public:
    StringWithReplacement();
    StringWithReplacement(const std::string &query);
    ~StringWithReplacement();
    StringWithReplacement(const StringWithReplacement& other);// copy constructor
    StringWithReplacement(StringWithReplacement&& other);// move constructor
    StringWithReplacement& operator=(const StringWithReplacement& other);// copy assignment
    StringWithReplacement& operator=(StringWithReplacement&& other);// move assignment
    static uint16_t preparedQuerySize(const unsigned char * const preparedQuery);
    void operator=(const std::string &query);
    void set(const std::string &query);
    bool empty() const;
    std::string compose(const std::vector<std::string> &values) const;
    std::string originalQuery() const;
    uint8_t argumentsCount() const;
private:
    /* [0]: occurence to replace
     * [1,2]: total size of the String
     * List of: 16Bit header + string content */
    unsigned char * preparedQuery;
    static char composeBuffer[4096];
};

#endif // StringWithReplacement_H
