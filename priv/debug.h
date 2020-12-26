#ifndef DEBUG_H
#define DEBUG_H

#include <assert.h>

template <typename ...Args>
static void warning(char const * , Args const & ...) {}

#endif // DEBUG_H
