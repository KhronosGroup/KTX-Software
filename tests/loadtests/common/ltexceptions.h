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

class unsupported_ctype : public std::runtime_error {
  public:
    unsupported_ctype()
         : std::runtime_error("Unsupported compression format") { }
};

class unsupported_ttype : public std::runtime_error {
  public:
    unsupported_ttype()
         : std::runtime_error("Implementation does not support needed operations on image format") { }
};
