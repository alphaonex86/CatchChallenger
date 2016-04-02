#ifndef StringWithReplacement_H
#define StringWithReplacement_H

#include <string>

class StringWithReplacement
{
public:
    StringWithReplacement();
    StringWithReplacement(const std::string &query);
    ~StringWithReplacement();
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
private:
    char * preparedQuery;
    static char composeBuffer[4096];
};

#endif // StringWithReplacement_H
