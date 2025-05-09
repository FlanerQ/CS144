#include "byte_stream.hh"

#include <cstddef>

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity)
    : _buffer(capacity + 1), _capacity(capacity), _written_cnt(0), _read_cnt(0), _head(0), _tail(0) {}

size_t ByteStream::write(const string &data) {
    size_t written = 0;
    for (auto value : data) {
        // buffer full
        if (remaining_capacity() == 0) {
            break;
        }
        _buffer[_tail++] = value;
        if (_tail == _buffer.size()) {
            _tail = 0;
        }
        ++_written_cnt;
        ++written;
    }
    return written;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    string res = "";
    size_t p1 = _head;
    for (size_t i = 0; i < len; i++) {
        if (p1 == _tail) {
            break;
        }
        res += _buffer[p1++];
        if (p1 == _buffer.size()) {
            p1 = 0;
        }
    }
    return res;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (buffer_empty()) {
            break;
        }
        _head++;
        _read_cnt++;  // Update _read_cnt
        if (_head == _buffer.size()) {
            _head = 0;
        }
    }
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    auto ret = peek_output(len);
    pop_output(len);
    return ret;
}

void ByteStream::end_input() { _input_ended_flag = true; }

bool ByteStream::input_ended() const { return _input_ended_flag; }

size_t ByteStream::buffer_size() const { return _tail >= _head ? _tail - _head : _tail + _buffer.size() - _head; }

bool ByteStream::buffer_empty() const { return buffer_size() == 0; }

bool ByteStream::eof() const { return _input_ended_flag && buffer_empty(); }

size_t ByteStream::bytes_written() const { return _written_cnt; }

size_t ByteStream::bytes_read() const { return _read_cnt; }

size_t ByteStream::remaining_capacity() const { return _capacity - buffer_size(); }
