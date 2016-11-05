/*
 * PicturedNumericArea.h
 *
 *  Created on: Oct 30, 2016
 *      Author: Robert
 */

#ifndef SRC_PICTUREDNUMERICAREA_H_
#define SRC_PICTUREDNUMERICAREA_H_

#include <cstring>

namespace forth {

class PicturedNumericArea {
private:
    static constexpr int buff_size_ = 128;
    char buff_[buff_size_];
    char *ptr_;

    PicturedNumericArea() : ptr_(buff_) { Reset(); }

public:
    static PicturedNumericArea& instance();

    void Reset() { std::memset(buff_, 0, buff_size_); }

    PicturedNumericArea(PicturedNumericArea&&) = delete;
    PicturedNumericArea(const PicturedNumericArea&) = delete;
    void operator=(PicturedNumericArea&&) = delete;
    void operator=(const PicturedNumericArea&) = delete;
};

} // namespace forth

#endif /* SRC_PICTUREDNUMERICAREA_H_ */
