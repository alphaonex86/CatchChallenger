#ifndef CATCHCHALLENGER_GUI_LOG_H
#define CATCHCHALLENGER_GUI_LOG_H

// Captures std::cout / std::cerr (and printf / write(1|2) when
// install_stdio_redirect=true) into a thread-safe ring of lines that
// the Qt admin GUI drains into ui->textEditConsole.  Plain C++ — no Qt.
// Gated by CATCHCHALLENGER_GUI_STATS so non-GUI binaries (epoll/login/
// master/headless server) compile without this code at all.
//
// Why streambuf-tee instead of dup2(): the engine code uses both
// std::cout (cooked C++ streams) and printf / write(1|2) (raw libc),
// and Qt's own qDebug() ultimately writes to fd 2.  A streambuf swap
// only covers the C++ stream APIs.  install_stdio_redirect() also
// runs `dup2()` against fd 1 / fd 2 so libc / qDebug / Qt's plugin
// loader output ALL flow into the ring — at the cost of losing the
// terminal echo for those channels.  Default behaviour is conservative
// (streambuf-only, terminal still gets everything) since dup2 is
// destructive.

#ifdef CATCHCHALLENGER_GUI_STATS

#include <deque>
#include <string>

namespace CatchChallenger {
namespace gui_log {

// Wire std::cout / std::cerr through a tee-streambuf.  Each line still
// goes to the original stream (so the operator's terminal keeps
// scrolling) and is also enqueued for drain_lines().  Idempotent:
// calling install() twice is a no-op.
//
// install_stdio_redirect=true ALSO dup2's fd 1 & fd 2 onto a pipe end
// so printf / write / qDebug land in the ring; the trade-off is the
// terminal won't see those channels anymore (only what install()'s
// pump-thread re-prints).  Use when running headless with no
// controlling terminal (testingqtserver --autostart) or when you want
// the GUI console to be the single source of truth.
void install(bool install_stdio_redirect = false);

// Drain up to `max` lines.  Cheap to poll once per second from the GUI
// timer; takes a small mutex.
std::deque<std::string> drain_lines(size_t max = 256);

// True if a line is one of "stderr" rather than "stdout".  We tag the
// origin in the queue so the GUI can paint stderr lines red.  No
// timestamp here either — the [HH:MM:SS] prefix is GUI-generated in
// server/qt/gui/MainWindow.cpp at drain time.
struct LogLine {
    std::string text;
    bool        is_err;
};
std::deque<LogLine> drain_classified_lines(size_t max = 256);

}  // namespace gui_log
}  // namespace CatchChallenger

#endif  // CATCHCHALLENGER_GUI_STATS

#endif  // CATCHCHALLENGER_GUI_LOG_H
