//==============================================================================
// Sleeper - a simple sleep class that allows interruptable sleep
//==============================================================================

#pragma once

#include <stdint.h>
#include <mutex>

// -----------------------------------------------------------------------------
class CSleeper
{

public:

    // Constructor & destructor
    CSleeper() {m_pipe_fd[0] = m_pipe_fd[1] = -1;};
    ~CSleeper();

    // Initialize sleeper
    void init();

    // Function that waits on select to do an interruptable sleep
    bool sleep(uint32_t timeout_ms);

    // Writes a message to the pipe to wake the sleeper up
    void wakeup(unsigned char code = 0);

    // Checks if there are any bytes in the pipe and drains it
    void drain();

protected:

    // A variable to hold data to send on the pipe
    int m_pipe_fd[2];

    // Mutex used for recursive locking of a thread
    std::recursive_mutex m_mtx;
};
// -----------------------------------------------------------------------------
