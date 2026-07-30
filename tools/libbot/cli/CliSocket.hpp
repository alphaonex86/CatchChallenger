#ifndef CATCHCHALLENGER_CLI_CLISOCKET_H
#define CATCHCHALLENGER_CLI_CLISOCKET_H

#include <string>
#include <stdint.h>
#include <sys/types.h>

namespace CatchChallenger {

/** \brief Plain POSIX TCP socket for the Qt-free CLI client core.
 *
 * The fd is non-blocking, so:
 *  - startConnect() only STARTS the connect (EINPROGRESS); the event loop
 *    waits for the fd to become writable and then calls finishConnect(),
 *    which reads SO_ERROR to learn whether the connect succeeded.
 *  - readData() maps EAGAIN to 0 because that is exactly what
 *    ProtocolParsingInputOutput::parseIncommingData() treats as "nothing
 *    pending"; a peer close or a real error return -1.
 *  - writeSome() is a PARTIAL, never-blocking write: it hands as many bytes as
 *    the kernel accepts and reports EAGAIN as 0. The caller (CliApiClient)
 *    keeps the remainder in its own output buffer and waits for the fd to
 *    become writable again, so one bot that the server stopped draining never
 *    stalls the others. Blocking here (poll(POLLOUT)) would freeze the whole
 *    single-threaded fleet on the slowest connection.
 */
class CliSocket
{
public:
    CliSocket();
    ~CliSocket();

    /// \brief resolve host and start a non-blocking connect
    bool startConnect(const std::string &host,const uint16_t &port);
    /// \brief called once the fd is writable; false when the connect failed
    bool finishConnect();
    bool isConnecting() const;
    bool isValid() const;
    int fd() const;

    /// \return >0 bytes read, 0 nothing pending (EAGAIN), -1 peer closed or error
    ssize_t readData(char *data,const size_t &size);
    /// \return bytes the kernel accepted (0 when the send buffer is full), -1 on error
    ssize_t writeSome(const char * const data,const size_t &size);

    void closeSocket();
    const std::string &lastError() const;
private:
    /// \brief O_NONBLOCK + TCP_NODELAY on the freshly created fd
    bool applySocketOptions();
    int socketFd;
    bool connecting;
    std::string errorString;
};

}

#endif // CATCHCHALLENGER_CLI_CLISOCKET_H
