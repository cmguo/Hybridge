#ifndef DEBUG_H
#define DEBUG_H

#include <assert.h>
#include <iostream>

template <typename ...Args>
static inline void warning(char const * msg , Args const & ...) { std::cout << msg << std::endl; }

#endif // DEBUG_H
