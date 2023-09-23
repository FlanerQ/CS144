#ifndef SPONGE_LIBSPONGE_RESTRANS_TIMER
#define SPONGE_LIBSPONGE_RESTRANS_TIMER

#include <cstddef>

enum class TimerState {
    RUNNING,
    STOPPED,
};

class Timer {
  private:
    TimerState state;
    size_t _initial_rto;
    size_t _rto;
    size_t _accumulate_time = 0;

  public:
    //! \brief constructor
    Timer(const size_t timeout);
    
    //! \brief Timer tick
    void tick(const size_t ms_since_last_tick);

    //! \brief check whether the time is expired
    bool timer_expired();

    //! \brief when receiving a valid ack, reset the timer
    void reset_timer();

    //! \brief when the timer is expired we shoud handle this situation
    void handle_expired();

    //! \brief start timer
    void start_timer();

    //! \brief start timer
    void stop_timer();
};

#endif