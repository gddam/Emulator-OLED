#pragma once

#include <cstdint>

#define pgm_read_word(addr) (*reinterpret_cast<const uint16_t *>(addr))
#define pgm_read_ptr(addr) (*reinterpret_cast<const void *const *>(addr))
