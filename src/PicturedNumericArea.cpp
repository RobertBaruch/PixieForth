/*
 * PicturedNumericArea.cpp
 *
 *  Created on: Oct 30, 2016
 *      Author: Robert
 */

#include <PicturedNumericArea.h>

namespace forth {

PicturedNumericArea& PicturedNumericArea::instance() {
  static PicturedNumericArea instance;
  return instance;
}

} // namespace forth
