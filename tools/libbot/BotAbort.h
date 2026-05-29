#ifndef CATCHCHALLENGER_BOTABORT_H
#define CATCHCHALLENGER_BOTABORT_H

#include <cstdlib>
#include <iostream>

// BOT_ABORT(): drop-in replacement for `abort();` that first writes a
// flushed file:line:function marker to std::cerr, then aborts.  Two
// motivations:
//   * benchmarkbotactions.py runs the bot under gdb and dumps the last
//     50 lines of std::cerr on any crash -- without this line the only
//     hint is the gdb backtrace, which is sometimes truncated/stripped.
//   * the bot binary is also used standalone (no gdb attached); without
//     this line a SIGABRT shows up in the kernel log with no source-
//     level context at all.
// The flush() before abort() is what guarantees the cerr line is on
// disk when the core dump happens -- abort() bypasses the stdio
// at-exit handlers.
#define BOT_ABORT() do { \
    std::cerr << "BOT_ABORT " << __FILE__ << ":" << __LINE__ \
              << " in " << __func__ << "()" << std::endl; \
    std::cerr.flush(); \
    std::abort(); \
} while(0)

#endif
