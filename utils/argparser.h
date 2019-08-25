/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

#ifndef ARGPARSER_H
#define ARGPARSER_H

/*
 * ©2017 Mark Callow.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
        : argv(argv), optind(startindex) { }

    argparser(int argc, const _TCHAR* const* argv1)
       : argv(argc, argv1), optind(1) { }

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
