/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2018-2020 Mark Callow, <khronos at callow dot im>.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @internal
 * @file ltexceptions.h
 * @~English
 *
 * @brief Custom exceptions for the load tests.
 */

#include <stdexcept>
#include <string>

#define OUT_OF_HOST_MEMORY -1
#define OUT_OF_DEVICE_MEMORY -2
#define FRAGMENTED_POOL -12
#define OUT_OF_POOL_MEMORY -1000069000

class bad_vulkan_alloc : public std::bad_alloc {
  public:
    bad_vulkan_alloc(int which, const char* _message) : std::bad_alloc() {
        if (which == FRAGMENTED_POOL) {
            message << "Pool fragmented when allocating for " << _message << ".";
        } else {
          std::string memtype;
            switch (which) {
              case OUT_OF_HOST_MEMORY: memtype = "host"; break;
              case OUT_OF_DEVICE_MEMORY: memtype = "device"; break;
              case OUT_OF_POOL_MEMORY: memtype = "pool"; break;
              default: break;
            }
            message << "Out of " << memtype << " memory for " << _message << ".";
        }
        _what = message.str();
    }
    virtual const char* what() const throw() {
        return _what.c_str();
    }
  protected:
    std::stringstream message;
    std::string _what;
};

class unsupported_ttype : public std::runtime_error {
  public:
    unsupported_ttype()
         : std::runtime_error("Implementation does not support needed operations on image format") { }
    unsupported_ttype(std::string& message) : std::runtime_error(message) { }
};

class unsupported_ctype : public std::runtime_error {
  public:
    unsupported_ctype()
         : std::runtime_error("Unsupported compression format") { }
};
