/*
 * System.cpp
 *
 *  Created on: Oct 30, 2016
 *      Author: Robert
 */

#include <System.h>

namespace forth {

DataStack& System::stack_(DataStack::instance());

System& System::instance() {
  static System instance;
  return instance;
}

uint32_t System::Pop() {
    return stack_.Pop();
}

void System::Add() {
    std::uint32_t x = stack_.Pop();
    stack_.Put(0, stack_.Peek(0) + x);
}

void System::Store() {
    uint32_t *addr = (uint32_t *) Pop();
    *addr = Pop();
}

}
