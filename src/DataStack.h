/*
 * DataStack.h
 *
 *  Created on: Oct 30, 2016
 *      Author: Robert
 */

#ifndef SRC_DATASTACK_H_
#define SRC_DATASTACK_H_

#include <cstdint>

namespace forth {

class DataStack {
private:
    static constexpr int stack_size_ = 1024;
    static std::uint32_t *sp_;
    static std::uint32_t data_stack_[stack_size_];

    DataStack() { Reset(); }

public:
    static DataStack& instance();

    void Reset() { sp_ = data_stack_; }
    std::uint32_t **GetSPPtr() { return &sp_; }
    void Push(std::uint32_t val) { *sp_++ = val; }
    std::uint32_t Pop() { return *--sp_; }
    std::uint32_t Peek(int offset) { return *(sp_ - offset - 1); }
    void Put(int offset, std::uint32_t val) { *(sp_ - offset - 1) = val; }

    DataStack(DataStack&&) = delete;
    DataStack(const DataStack&) = delete;
    void operator=(DataStack&&) = delete;
    void operator=(const DataStack&) = delete;
};

} // namespace forth

#endif /* SRC_DATASTACK_H_ */
