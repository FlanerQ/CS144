#include "tcp_sender.hh"

#include "buffer.hh"
#include "tcp_config.hh"
#include "tcp_segment.hh"
#include "wrapping_integers.hh"

#include <algorithm>
#include <cstdint>
#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity)
    , _timer(_initial_retransmission_timeout) {}

uint64_t TCPSender::bytes_in_flight() const { return _next_seqno - _receiver_ack; }

void TCPSender::fill_window() {
    // 'FIN' has been sent
    if (_end) {
        return;
    }
    TCPSegment segment{};

    // `_receiver_window_size` == 0
    uint64_t window_size = _receiver_window_size == 0 ? 1 : _receiver_window_size;

    // TCP connection
    // send SYN & initial _receiver_window_size to 1
    if (_next_seqno == 0) {
        segment.header().syn = true;
        segment.header().seqno = _isn + _next_seqno;
        _next_seqno += 1;
    }
    // TCP connection : establish, ready to read from from its input ByteStream
    // send as many bytes as we could in the form of TCPSegments
    else {
        uint64_t length =
            std::min(std::min(window_size - bytes_in_flight(), stream_in().buffer_size()), TCPConfig::MAX_PAYLOAD_SIZE);

        // TCP segment's payload is stored as a reference-counted read-only string (The `Buffer` Object).
        segment.payload() = Buffer{stream_in().read(length)};
        segment.header().seqno = _isn + _next_seqno;
        _next_seqno += length;

        // set `fin` to true when `stream_in` is end of file
        // Pay attention to check whether there is an enough window size
        if (stream_in().eof() && window_not_full(window_size)) {
            segment.header().fin = true;
            _end = true;
            _next_seqno++;
        }

        // Do nothing: [ receiver doesn't have space ] or [ bytestream doesn't have input yet ]
        if (length == 0 && !_end)
            return;
    }
    _segments_out.push(segment);
    _outstanding_segments.push_back(segment);
    _timer.start_timer();
    if (window_not_full(window_size)) {
        fill_window();
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    uint64_t abs_ackno = unwrap(ackno, _isn, next_seqno_absolute());
    if (abs_ackno > _next_seqno || abs_ackno < _receiver_ack) {
        return;
    }
    _receiver_window_size = window_size;
    bool is_ack_update = false;

    // remove segments before ackno from outstanding segments
    for (auto it = _outstanding_segments.begin(); it != _outstanding_segments.end();) {
        uint64_t abs_seqno = unwrap(it->header().seqno, _isn, next_seqno_absolute());
        if (abs_seqno + it->length_in_sequence_space() <= abs_ackno) {
            _receiver_ack = abs_seqno + it->length_in_sequence_space();
            it = _outstanding_segments.erase(it);
            is_ack_update = true;
        } else {
            it++;
        }
    }

    // Stop timer when there are no outstanding segments
    if (_outstanding_segments.empty()) {
        _timer.stop_timer();
    }

    // Reset timer if we get ackno updated
    if (is_ack_update) {
        _timer.reset_timer();

        _consecutive_retransmissions = 0;
    }

    if (window_not_full(window_size)) {
        fill_window();
    }
}

/*
    Use of Timer:

    当发送一个新的segment的时候，如果timer没有开启，那么需要开启timer。

    当在RTO内收到一个合法的ACK,有两种情况:
    如果sender没发完segments那么需要重启timer,重启的意思是timer从0开始计时。
    如果sender已经发完所有的segments了那么需要关闭timer

    当超时的情况发生,也是两种情况:
    window_size = 0 : 重启timer,重传segments。
    window_size != 0 : double RTO, 重启timer,重传segments。
*/

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    _timer.tick(ms_since_last_tick);
    if (_timer.timer_expired()) {
        if (_receiver_window_size == 0) {
            _timer.reset_timer();
        } else {
            _timer.handle_expired();
        }
        _consecutive_retransmissions++;
        segments_out().push(_outstanding_segments.front());
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retransmissions; }

void TCPSender::send_empty_segment() {
    TCPSegment empty{};
    empty.header().seqno = _isn + _next_seqno;
    segments_out().push(empty);
}
