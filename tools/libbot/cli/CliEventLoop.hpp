#ifndef CATCHCHALLENGER_CLI_CLIEVENTLOOP_H
#define CATCHCHALLENGER_CLI_CLIEVENTLOOP_H

#include <stdint.h>
#include <string>
#include <vector>

namespace CatchChallenger {

class CliApiClient;

/** \brief minimal select() loop over a set of CliApiClient.
 *
 * select() is used instead of epoll to stay portable POSIX; it caps the fd
 * value at FD_SETSIZE, so run() refuses a client whose fd is out of range
 * rather than corrupting the stack (the classic select() footgun).
 *
 * The loop does not own the clients: the caller creates them, connects them
 * and reads their state afterwards.
 */
class CliEventLoop
{
public:
    CliEventLoop();
    ~CliEventLoop();

    void addClient(CliApiClient *client);
    /** \brief run until every client is finished or the deadline expires.
     * \param timeoutMs total wall-clock budget for the whole set
     * \return true when every client finished before the deadline */
    bool run(const uint32_t &timeoutMs);
    /** \brief saturation phase: every on-map client moves as fast as the
     * server drains its socket, for `seconds`.
     *
     * No sleep, no rate limit: the only pacing is the server itself, through
     * TCP back-pressure (a client whose send buffer is full waits for the fd to
     * be writable instead of spinning on EAGAIN). A client that gets
     * disconnected is dropped from the rotation and the others keep going.
     *
     * \param seconds       length of the measurement window
     * \param moves         [out] move packets handed to a socket, all clients
     * \param elapsedNs     [out] the window ACTUALLY measured (CLOCK_MONOTONIC)
     * \param survivors     [out] clients still on the map and connected at the end
     * \return false only when the loop itself failed (getError() is set) */
    bool runSpam(const uint32_t &seconds,uint64_t &moves,uint64_t &elapsedNs,size_t &survivors);
#ifdef CATCHCHALLENGER_BENCHMARK
    /** \brief latency phase: for `seconds`, drive the CliLatency chat probes
     * and drain every socket so the echoes are timestamped as they arrive.
     *
     * Deliberately NOT a saturation loop: it sends a few bytes per bot per
     * 1.5s and spends the rest of its time in select(), because the metric is
     * the tail of a QUIET path. Loading the server at the same time would
     * measure queueing delay instead (that is what runSpam() is for).
     *
     * Only compiled in the CATCHCHALLENGER_BENCHMARK build -- so is the
     * recorder it drives.
     *
     * \param seconds   measured window, started only by the caller once every
     *                  bot is on the map (startup excluded)
     * \param elapsedNs [out] the window ACTUALLY measured (CLOCK_MONOTONIC)
     * \return false only when the loop itself failed (getError() is set) */
    bool runLatency(const uint32_t &seconds,uint64_t &elapsedNs);
#endif
    /// \brief clients that reached the map
    size_t onMapCount() const;
    /// \brief set when run() bailed out on its own error (not a timeout)
    const std::string &getError() const;
private:
    /// \brief run the deferred reconnect / datapack work of every client
    void runPendingWork();
    /// \brief print the transitions accumulated since the last call
    void reportStateChanges();
    std::vector<CliApiClient *> clients;
    std::string errorString;
};

}

#endif // CATCHCHALLENGER_CLI_CLIEVENTLOOP_H
