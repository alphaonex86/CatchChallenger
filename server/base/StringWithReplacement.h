#ifndef StringWithReplacement_H
#define StringWithReplacement_H

#include <string>

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
    std::string compose(const std::string &arg1) const;
    std::string compose(const std::string &arg1,
                        const std::string &arg2
                        ) const;
    std::string compose(const std::string &arg1,
                        const std::string &arg2,
                        const std::string &arg3
                        ) const;
    std::string compose(const std::string &arg1,
                        const std::string &arg2,
                        const std::string &arg3,
                        const std::string &arg4
                        ) const;
    std::string compose(const std::string &arg1,
                        const std::string &arg2,
                        const std::string &arg3,
                        const std::string &arg4,
                        const std::string &arg5
                        ) const;
    std::string compose(const std::string &arg1,
                        const std::string &arg2,
                        const std::string &arg3,
                        const std::string &arg4,
                        const std::string &arg5,
                        const std::string &arg6
                        ) const;
    std::string compose(const std::string &arg1,
                        const std::string &arg2,
                        const std::string &arg3,
                        const std::string &arg4,
                        const std::string &arg5,
                        const std::string &arg6,
                        const std::string &arg7
                        ) const;
    std::string compose(const std::string &arg1,
                        const std::string &arg2,
                        const std::string &arg3,
                        const std::string &arg4,
                        const std::string &arg5,
                        const std::string &arg6,
                        const std::string &arg7,
                        const std::string &arg8
                        ) const;
    std::string compose(const std::string &arg1,
                        const std::string &arg2,
                        const std::string &arg3,
                        const std::string &arg4,
                        const std::string &arg5,
                        const std::string &arg6,
                        const std::string &arg7,
                        const std::string &arg8,
                        const std::string &arg9
                        ) const;
    std::string compose(const std::string &arg1,
                        const std::string &arg2,
                        const std::string &arg3,
                        const std::string &arg4,
                        const std::string &arg5,
                        const std::string &arg6,
                        const std::string &arg7,
                        const std::string &arg8,
                        const std::string &arg9,
                        const std::string &arg10
                        ) const;
    std::string compose(const std::string &arg1,
                        const std::string &arg2,
                        const std::string &arg3,
                        const std::string &arg4,
                        const std::string &arg5,
                        const std::string &arg6,
                        const std::string &arg7,
                        const std::string &arg8,
                        const std::string &arg9,
                        const std::string &arg10,
                        const std::string &arg11
                        ) const;
    std::string compose(const std::string &arg1,
                        const std::string &arg2,
                        const std::string &arg3,
                        const std::string &arg4,
                        const std::string &arg5,
                        const std::string &arg6,
                        const std::string &arg7,
                        const std::string &arg8,
                        const std::string &arg9,
                        const std::string &arg10,
                        const std::string &arg11,
                        const std::string &arg12
                        ) const;
    std::string compose(const std::string &arg1,
                        const std::string &arg2,
                        const std::string &arg3,
                        const std::string &arg4,
                        const std::string &arg5,
                        const std::string &arg6,
                        const std::string &arg7,
                        const std::string &arg8,
                        const std::string &arg9,
                        const std::string &arg10,
                        const std::string &arg11,
                        const std::string &arg12,
                        const std::string &arg13
                        ) const;
    std::string compose(const std::string &arg1,
                        const std::string &arg2,
                        const std::string &arg3,
                        const std::string &arg4,
                        const std::string &arg5,
                        const std::string &arg6,
                        const std::string &arg7,
                        const std::string &arg8,
                        const std::string &arg9,
                        const std::string &arg10,
                        const std::string &arg11,
                        const std::string &arg12,
                        const std::string &arg13,
                        const std::string &arg14
                        ) const;
    std::string compose(const std::string &arg1,
                        const std::string &arg2,
                        const std::string &arg3,
                        const std::string &arg4,
                        const std::string &arg5,
                        const std::string &arg6,
                        const std::string &arg7,
                        const std::string &arg8,
                        const std::string &arg9,
                        const std::string &arg10,
                        const std::string &arg11,
                        const std::string &arg12,
                        const std::string &arg13,
                        const std::string &arg14,
                        const std::string &arg15
                        ) const;
    std::string originalQuery() const;
private:
    /* [0]: occurence to replace
     * [1,2]: total size of the String
     * List of: 16Bit header + string content */
    unsigned char * preparedQuery;
    static char composeBuffer[4096];
};

#endif // StringWithReplacement_H
