//==============================================================================
// Sleeper - a simple sleep class that allows interruptable sleep
//==============================================================================

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "sleeper.h"

// -----------------------------------------------------------------------------
// Constructor
// -----------------------------------------------------------------------------
CSleeper::~CSleeper()
{
    if (m_pipe_fd[0] != -1) close(m_pipe_fd[0]);
    if (m_pipe_fd[1] != -1) close(m_pipe_fd[1]);
}
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Initialize sleeper
// -----------------------------------------------------------------------------
void CSleeper::init()
{
    // If it doesn't exist, create a pipe
    if (m_pipe_fd[0] == -1) pipe2(m_pipe_fd, O_CLOEXEC);
}
// -----------------------------------------------------------------------------



// -----------------------------------------------------------------------------
// sleep() - Function that waits on select to do an interruptable sleep
//           Returns true when timedout
//           Returns false if we're woken up
// -----------------------------------------------------------------------------
bool CSleeper::sleep(uint32_t timeout_ms)
{
    timeval timeout;
    fd_set rfds;

    int fd = m_pipe_fd[0];
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);

    // Assume for the moment that we are going to wait forever
    timeval* p_timeout = NULL;
    
    // If the caller wants us to wait for a finite amount of time...
    if (timeout_ms != -1)
    {
        // Convert milliseconds to microseconds
        int usecs = timeout_ms * 1000;

        // Determine the timeout in seconds and microseconds
        timeout.tv_sec  = usecs / 1000000;
        timeout.tv_usec = usecs % 1000000;

        // Point to the timeout structure we just initialized
        p_timeout = &timeout;
    }

    // If someone writes to the pipe, tell the caller we're interrupted
    if (select(fd+1, &rfds, NULL, NULL, p_timeout) != 0) return false;
    
    // If we get here, select timed out
    return true;
}
// -----------------------------------------------------------------------------



// -----------------------------------------------------------------------------
// wakeup() - Writes a message to the pipe to wake the sleeper up
// -----------------------------------------------------------------------------
void CSleeper::wakeup(unsigned char code)
{
    write(m_pipe_fd[1], &code, 1);
}
// -----------------------------------------------------------------------------



// -----------------------------------------------------------------------------
// drain() - Checks if there are any bytes in the pipe and drains it
// -----------------------------------------------------------------------------
void CSleeper::drain()
{
    char buffer[1];
    int count = 0;    

    // Check number of bytes available to read at the file descriptor
    while (ioctl(m_pipe_fd[0], FIONREAD, &count) == 0 && count > 0) 
    {
        // Now drain the pipe by reading it
        if (read(m_pipe_fd[0], buffer, 1) < 0) return;
    }
}
// -----------------------------------------------------------------------------