/**
 * @file abort.h
 * @author Basit Ayantunde <rlamarrr@gmail.com>
 * @brief
 * @version  0.1
 * @date 2020-04-26
 *
 * @copyright Copyright (c) 2020
 *
 */
#ifndef STX_PANIC_HANDLERS_ABORT_ABORT_H_
#define STX_PANIC_HANDLERS_ABORT_ABORT_H_

#include <cstdlib>

#include "stx/panic.h"

namespace stx {

/// Causes the abort instruction to be executed.
[[noreturn]] inline void panic_abort(
    std::string_view info,
    SourceLocation location = SourceLocation::current()) {
  (void)info;
  (void)location;

  std::abort();
}

};  // namespace stx

#endif  // STX_PANIC_HANDLERS_ABORT_ABORT_H_
