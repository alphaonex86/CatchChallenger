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
