 #ifdef LIBEXPORT
    #if defined _WIN32 || defined __CYGWIN__
        #ifdef __GNUC__
        #define DLL_PUBLIC __attribute__ ((dllexport))
        #else
        #define DLL_PUBLIC __declspec(dllexport) // Note: actually gcc seems to also supports this syntax.
        #endif
    #else
        #define DLL_PUBLIC __attribute__ ((visibility ("default")))
        #define DLL_LOCAL  __attribute__ ((visibility ("hidden")))
    #endif 
#else
    #ifdef LIBIMPORT
        #if defined _WIN32 || defined __CYGWIN__
            #ifdef __GNUC__
            #define DLL_PUBLIC __attribute__ ((dllimport))
            #else
            #define DLL_PUBLIC __declspec(dllimport) // Note: actually gcc seems to also supports this syntax.
            #endif
        #else
            #define DLL_PUBLIC
            #define DLL_LOCAL
        #endif
    #else
            #define DLL_PUBLIC
            #define DLL_LOCAL
    #endif
#endif
