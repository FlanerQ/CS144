#include "retrans_timer.hh"

Timer::Timer(const size_t timeout) : state{TimerState::STOPPED}, _initial_rto(timeout), _rto(timeout) {}

void Timer::tick(const size_t ms_since_last_tick) {
    if (state == TimerState::RUNNING) {
        _accumulate_time += ms_since_last_tick;
    }
}

bool Timer::timer_expired() { return state == TimerState::RUNNING && _accumulate_time >= _rto; }

void Timer::reset_timer() {
    _rto = _initial_rto;
    _accumulate_time = 0;
}

void Timer::start_timer() {
    if (state == TimerState::STOPPED) {
        state = TimerState::RUNNING;
        reset_timer();
    }
}

void Timer::stop_timer() {
    if (state == TimerState::RUNNING) {
        state = TimerState::STOPPED;
        reset_timer();
    }
}

void Timer::handle_expired() {
    _rto *= 2;
    _accumulate_time = 0;
}