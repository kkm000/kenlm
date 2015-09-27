/* Like std::ofstream but without being incredibly slow.  Backed by a raw fd.
 * Supports most of the built-in types except for void* and long double.
 */
#ifndef UTIL_FAKE_OFSTREAM_H
#define UTIL_FAKE_OFSTREAM_H

#include "util/fake_ostream.hh"
#include "util/file.hh"
#include "util/scoped.hh"

#include <cassert>
#include <cstring>

#include <stdint.h>

namespace util {

class FakeOFStream : public FakeOStream<FakeOFStream> {
  public:
    FakeOFStream(int out = -1, std::size_t buffer_size = 8192)
      : buf_(util::MallocOrThrow(std::max<std::size_t>(buffer_size, kToStringMaxBytes))),
        current_(static_cast<char*>(buf_.get())),
        end_(current_ + std::max<std::size_t>(buffer_size, kToStringMaxBytes)),
        fd_(out) {}

    ~FakeOFStream() {
      flush();
    }

    void SetFD(int to) {
      flush();
      fd_ = to;
    }

    FakeOFStream &flush() {
      if (current_ != buf_.get()) {
        util::WriteOrThrow(fd_, buf_.get(), current_ - (char*)buf_.get());
        current_ = static_cast<char*>(buf_.get());
      }
      return *this;
    }

    // For writes of arbitrary size.
    FakeOFStream &write(const void *data, std::size_t length) {
      if (UTIL_LIKELY(current_ + length <= end_)) {
        std::memcpy(current_, data, length);
        current_ += length;
        return *this;
      }
      flush();
      if (current_ + length <= end_) {
        std::memcpy(current_, data, length);
        current_ += length;
      } else {
        util::WriteOrThrow(fd_, data, length);
      }
      return *this;
    }

    FakeOFStream &seekp(uint64_t to) {
      util::SeekOrThrow(fd_, to);
      return *this;
    }

  protected:
    friend class FakeOStream;
    // For writes directly to buffer guaranteed to have amount < buffer size.
    char *Ensure(std::size_t amount) {
      if (UTIL_UNLIKELY(current_ + amount > end_)) {
        flush();
        assert(current_ + amount <= end_);
      }
      return current_;
    }

    void AdvanceTo(char *to) {
      current_ = to;
      assert(current_ <= end_);
    }

  private:
    util::scoped_malloc buf_;
    char *current_, *end_;
    int fd_;
};

} // namespace

#endif
