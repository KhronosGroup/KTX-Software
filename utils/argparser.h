/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

#ifndef ARGPARSER_H
#define ARGPARSER_H

/*
 * Copyright 2017-2020 Mark Callow, www.edgewise-cosulting.com.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ios>
#include <sstream>
#include <string>
#include <vector>
#if defined(_WIN32)
  #include <tchar.h>
#else
  #define _TCHAR char
  #define _T(x) x
#endif
#if defined(_UNICODE)
  #define _tstring std::wstring
#else
  #define _tstring std::string
#endif


class argvector : public std::vector<_tstring> {
  public:
    argvector() { };
    argvector(const _tstring& argstring);
    argvector(int argc, const _TCHAR* const* argv);
};

class argparser {
  public:
    struct option {
        const char* name;
        enum {no_argument, required_argument, optional_argument} has_arg;
        int* flag;
        int val;
    };

    _tstring optarg;
    unsigned int optind;
    argvector argv;

    argparser(argvector& argv, unsigned int startindex = 0)
        : optind(startindex), argv(argv) { }

    argparser(int argc, const _TCHAR* const* argv1)
        : optind(1), argv(argc, argv1)  { }

    int getopt(_tstring* shortopts, const struct option* longopts,
               int* longindex = nullptr);
};

//================== Helper for apps' processArgs ========================

// skips the number of characters equal to the length of given text
// does not check whether the skipped characters are the same as it
struct skip
{
    const _TCHAR* text;
    skip(const _TCHAR* text) : text(text) {}
};

std::istream& operator >> (std::istream& stream, const skip& x);

#endif /* ARGPARSER_H */
