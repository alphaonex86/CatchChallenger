#ifndef CATCHCHALLENGER_CerrLineAtomic_H
#define CATCHCHALLENGER_CerrLineAtomic_H

#include <streambuf>

namespace CatchChallenger {

/* Make one std::cerr/std::cout LINE atomic between threads.
 *
 * The Qt clients are multi threaded and several of those threads log: the
 * embedded server (QtServer), the SQL thread (QtDatabaseThread), the datapack
 * loader, the map visualiser, the path finder -- while the GUI thread logs
 * too. std::cerr gives NO line granularity: each operator<< is its own write,
 * so two threads logging at the same moment produce a spliced line ("[SQL
 * exec] SELECT ...[Thread 0x... exited]") or a torn one (a bare "leafgreen)").
 * Every splice is a one-off string, which is why test/client_output_check.py
 * had to grow rules to recognise the PIECES of known messages instead of the
 * messages themselves, and why a torn tail still failed testingclient.
 *
 * This buffers what a thread writes in a buffer OF ITS OWN and hands the
 * complete line to the real streambuf under one shared mutex, so a line is
 * written whole or not at all, and cerr cannot land inside a cout line either
 * (both share the mutex, which matters as soon as the two are merged into one
 * pipe).
 *
 * NOT a logging framework and not a new logging API: install() is called once
 * and the hundreds of existing "std::cerr << ... << std::endl" call sites stay
 * exactly as they are.
 *
 * Trade off: std::cerr is unit buffered, so it normally reaches the terminal
 * after EVERY operator<<. Here a line reaches it on its '\n'. A message
 * without a trailing newline therefore waits for the next one, and a hard
 * crash mid line loses that partial line -- worth it, since a torn line is a
 * message nobody can read or whitelist anyway. */
class CerrLineAtomic : public std::streambuf
{
public:
    explicit CerrLineAtomic(std::streambuf *target);
    /* Wrap std::cerr and std::cout. Idempotent; call once, as early as
     * possible in main(). The wrappers are intentionally never destroyed:
     * they must outlive every thread that may still log at exit. */
    static void install();
protected:
    int overflow(int character) override;
    std::streamsize xsputn(const char *data,std::streamsize size) override;
    int sync() override;
private:
    //the real buffer this one forwards whole lines to
    std::streambuf *target_;
};
}

#endif // CATCHCHALLENGER_CerrLineAtomic_H
