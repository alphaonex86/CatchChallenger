#include "CerrLineAtomic.hpp"

#include <iostream>
#include <mutex>
#include <string>

using namespace CatchChallenger;

/* ONE mutex for cerr AND cout: the two end up in the same terminal (and in the
 * same pipe as soon as a caller does 2>&1), so serialising them separately
 * would still let a cerr line land inside a cout line. Function local so it is
 * built on first use and never destroyed before the last thread that logs. */
static std::mutex &outputMutex()
{
    //NEVER destroyed: a function local static is destroyed at exit, and a
    //thread still logging then would lock a dead mutex.
    static std::mutex *mutex=new std::mutex;
    return *mutex;
}

/* The line being composed BY THIS THREAD. Per thread, so two threads never
 * write into each other's message; it grows to this thread's longest line and
 * then stops allocating. */
static std::string &threadLine()
{
    //Never destroyed either, same reason: the order between a thread_local
    //destructor and a static destructor that still logs is not specified.
    //One small buffer per LOGGING thread, and this client creates each of its
    //threads once.
    static thread_local std::string *line=new std::string;
    return *line;
}

//Hand everything up to and including the last '\n' to the real buffer, in one
//locked call, and keep whatever follows it for the next line.
static void flushCompleteLines(std::streambuf *target,std::string &line)
{
    const size_t lastEnd=line.rfind('\n');
    if(lastEnd==std::string::npos)
        return;
    const size_t size=lastEnd+1;
    {
        std::lock_guard<std::mutex> guard(outputMutex());
        target->sputn(line.data(),static_cast<std::streamsize>(size));
        target->pubsync();
    }
    line.erase(0,size);
}

CerrLineAtomic::CerrLineAtomic(std::streambuf *target) :
    target_(target)
{
}

int CerrLineAtomic::overflow(int character)
{
    if(character==traits_type::eof())
        return traits_type::not_eof(character);
    std::string &line=threadLine();
    line.push_back(static_cast<char>(character));
    if(character=='\n')
        flushCompleteLines(target_,line);
    return character;
}

std::streamsize CerrLineAtomic::xsputn(const char *data,std::streamsize size)
{
    if(size<=0)
        return 0;
    std::string &line=threadLine();
    line.append(data,static_cast<size_t>(size));
    flushCompleteLines(target_,line);
    return size;
}

/* Deliberately does NOT flush a partial line. std::cerr is unit buffered, so
 * it syncs after EVERY operator<<: flushing here would push half messages out
 * and put back exactly the interleaving this class removes. */
int CerrLineAtomic::sync()
{
    flushCompleteLines(target_,threadLine());
    return 0;
}

void CerrLineAtomic::install()
{
    static bool installed=false;
    if(installed)
        return;
    installed=true;
    //never deleted on purpose, see the header
    std::cerr.rdbuf(new CerrLineAtomic(std::cerr.rdbuf()));
    std::cout.rdbuf(new CerrLineAtomic(std::cout.rdbuf()));
}
