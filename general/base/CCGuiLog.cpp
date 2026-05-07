#include "CCGuiLog.hpp"

#ifdef CATCHCHALLENGER_GUI_STATS

#include <atomic>
#include <cstdio>
#include <iostream>
#include <mutex>
#include <streambuf>
#include <thread>
#include <vector>

#ifndef _WIN32
#include <unistd.h>
#include <fcntl.h>
#endif

namespace CatchChallenger {
namespace gui_log {

namespace {

constexpr size_t LOG_RING_CAPACITY = 4096;  // ~4k lines = bounded by tens of MB worst case.

std::mutex log_mu;
std::deque<LogLine> log_ring;

// Drop-oldest ring push.  Same policy as gui_stats::push_event.
void enqueue(const std::string &line, bool is_err)
{
    std::lock_guard<std::mutex> g(log_mu);
    if (log_ring.size() >= LOG_RING_CAPACITY)
        log_ring.pop_front();
    log_ring.push_back({line, is_err});
}

// Tee-streambuf: forward bytes to the original sink AND split into
// lines for the GUI ring.  std::streambuf overflow() is the canonical
// hook; sync() flushes our partial-line buffer.  Per-instance line
// buffer is fed character-by-character so std::endl flushes correctly.
class TeeBuf : public std::streambuf {
public:
    TeeBuf(std::streambuf *passthrough, bool is_err)
        : passthrough_(passthrough), is_err_(is_err) {}

protected:
    int overflow(int ch) override {
        if (ch == EOF)
            return 0;
        if (passthrough_)
            passthrough_->sputc(static_cast<char>(ch));
        std::lock_guard<std::mutex> g(line_mu_);
        if (ch == '\n') {
            enqueue(line_, is_err_);
            line_.clear();
        } else {
            line_.push_back(static_cast<char>(ch));
            // Cap per-line length so a runaway producer can't grow
            // the buffer unbounded.
            if (line_.size() > 16 * 1024)
                line_.resize(16 * 1024);
        }
        return ch;
    }

    int sync() override {
        if (passthrough_)
            passthrough_->pubsync();
        return 0;
    }

private:
    std::streambuf *passthrough_;
    bool is_err_;
    std::mutex line_mu_;
    std::string line_;
};

std::atomic<bool> installed{false};
TeeBuf *cout_tee = nullptr;
TeeBuf *cerr_tee = nullptr;
std::streambuf *cout_orig = nullptr;
std::streambuf *cerr_orig = nullptr;

#ifndef _WIN32
// dup2-based redirect helpers.  Used when install_stdio_redirect=true.
// We pipe fd 1 / fd 2 into a worker thread that reads, splits on '\n',
// and feeds the ring + the saved original fds.
struct StdioPump {
    int read_fd = -1;
    int orig_fd = -1;
    bool is_err = false;
    std::thread thr;
    std::atomic<bool> stop{false};
};
StdioPump pump_out, pump_err;

void pump_loop(StdioPump *p)
{
    std::string line;
    char buf[4096];
    while (!p->stop.load(std::memory_order_relaxed)) {
        ssize_t n = ::read(p->read_fd, buf, sizeof(buf));
        if (n <= 0)
            break;
        // Re-emit to the saved original fd so the terminal still sees
        // it (qDebug / printf / etc).
        if (p->orig_fd >= 0)
            (void)::write(p->orig_fd, buf, n);
        for (ssize_t i = 0; i < n; ++i) {
            char c = buf[i];
            if (c == '\n') {
                enqueue(line, p->is_err);
                line.clear();
            } else {
                line.push_back(c);
                if (line.size() > 16 * 1024)
                    line.resize(16 * 1024);
            }
        }
    }
    if (!line.empty())
        enqueue(line, p->is_err);
}

void start_stdio_redirect()
{
    int pipefd_out[2], pipefd_err[2];
    if (::pipe(pipefd_out) < 0)
        return;
    if (::pipe(pipefd_err) < 0) {
        ::close(pipefd_out[0]); ::close(pipefd_out[1]);
        return;
    }
    pump_out.orig_fd = ::dup(1);
    pump_err.orig_fd = ::dup(2);
    pump_out.read_fd = pipefd_out[0];
    pump_err.read_fd = pipefd_err[0];
    pump_out.is_err = false;
    pump_err.is_err = true;
    ::dup2(pipefd_out[1], 1);
    ::dup2(pipefd_err[1], 2);
    ::close(pipefd_out[1]);
    ::close(pipefd_err[1]);
    pump_out.thr = std::thread(pump_loop, &pump_out);
    pump_err.thr = std::thread(pump_loop, &pump_err);
}
#endif

}  // namespace

void install(bool install_stdio_redirect)
{
    bool expected = false;
    if (!installed.compare_exchange_strong(expected, true))
        return;

    cout_orig = std::cout.rdbuf();
    cerr_orig = std::cerr.rdbuf();
    cout_tee = new TeeBuf(cout_orig, /*is_err=*/false);
    cerr_tee = new TeeBuf(cerr_orig, /*is_err=*/true);
    std::cout.rdbuf(cout_tee);
    std::cerr.rdbuf(cerr_tee);

#ifndef _WIN32
    if (install_stdio_redirect)
        start_stdio_redirect();
#else
    (void)install_stdio_redirect;
#endif
}

std::deque<std::string> drain_lines(size_t max)
{
    std::deque<std::string> out;
    std::lock_guard<std::mutex> g(log_mu);
    size_t take = log_ring.size();
    if (take > max)
        take = max;
    while (take > 0) {
        out.push_back(log_ring.front().text);
        log_ring.pop_front();
        --take;
    }
    return out;
}

std::deque<LogLine> drain_classified_lines(size_t max)
{
    std::deque<LogLine> out;
    std::lock_guard<std::mutex> g(log_mu);
    size_t take = log_ring.size();
    if (take > max)
        take = max;
    while (take > 0) {
        out.push_back(log_ring.front());
        log_ring.pop_front();
        --take;
    }
    return out;
}

}  // namespace gui_log
}  // namespace CatchChallenger

#endif  // CATCHCHALLENGER_GUI_STATS
