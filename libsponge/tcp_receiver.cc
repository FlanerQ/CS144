#include "tcp_receiver.hh"

#include "tcp_header.hh"
#include "wrapping_integers.hh"

#include <cstdint>
#include <optional>

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    const TCPHeader &header = seg.header();
    if (!_isn.has_value()) {
        if (!header.syn) {
            return;
        }
        _isn = header.seqno;
    }
    // set checkpoint to first unassembled byte index
    uint64_t checkpoint = _reassembler.stream_out().bytes_written();
    uint64_t abs_seq = unwrap(header.seqno, _isn.value(), checkpoint);

    // if SYN is set, index should be 0 - 1 = -1
    uint64_t stream_index = abs_seq - 1 + header.syn;
    _reassembler.push_substring(seg.payload().copy(), stream_index, header.fin);
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (!_isn.has_value()) {
        return nullopt;
    }
    if (_reassembler.stream_out().input_ended()) {
        return wrap(_reassembler.stream_out().bytes_written() + 1, _isn.value()) + 1;
    } else {
        return wrap(_reassembler.stream_out().bytes_written() + 1, _isn.value());
    }
}

size_t TCPReceiver::window_size() const { return _capacity - _reassembler.stream_out().buffer_size(); }
