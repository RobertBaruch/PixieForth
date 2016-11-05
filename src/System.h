/*
 * System.h
 *
 *  Created on: Oct 30, 2016
 *      Author: Robert
 */

#ifndef SRC_SYSTEM_H_
#define SRC_SYSTEM_H_

#include <PicturedNumericArea.h>
#include <DataStack.h>

namespace forth {

class System {
private:
    PicturedNumericArea& pictured_numeric_area_;
    static DataStack& stack_;

    System() :
        pictured_numeric_area_(PicturedNumericArea::instance()) { }

public:
    static System& instance();
    static void Push(uint32_t val) { stack_.Push(val); }
    static uint32_t **GetSPPtr() { return stack_.GetSPPtr(); }
    static uint32_t Pop();
    static void Add();
    static void Store();

    PicturedNumericArea& GetPicturedNumericArea() { return pictured_numeric_area_; }
    DataStack& GetDataStack() { return stack_; }

    System(System&&) = delete;
    System(const System&) = delete;
    void operator=(System&&) = delete;
    void operator=(const System&) = delete;
};

} // namespace forth
#endif /* SRC_SYSTEM_H_ */
