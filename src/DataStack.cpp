/*
 * DataStack.cpp
 *
 *  Created on: Oct 30, 2016
 *      Author: Robert
 */

#include <DataStack.h>

namespace forth {

DataStack& DataStack::instance() {
  static DataStack instance;
  return instance;
}

std::uint32_t *DataStack::sp_;
std::uint32_t DataStack::data_stack_[DataStack::stack_size_];
}
