/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4: */

#ifndef ARGPARSER_H
#define ARGPARSER_H

/*
 * Copyright (c) 2017, Mark Callow, www.edgewise-consulting.com.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and/or associated documentation files (the
 * "Materials"), to deal in the Materials without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Materials, and to
 * permit persons to whom the Materials are furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * unaltered in all copies or substantial portions of the Materials.
 * Any additions, deletions, or changes to the original source files
 * must be clearly indicated in accompanying documentation.
 *
 * If only executable code is distributed, then the accompanying
 * documentation must state that "this software is based in part on the
 * work of Mark Callow."
 *
 * THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
 */

#include <ios>
#include <sstream>
#include <string>
#include <vector>

class argvector : public std::vector<std::string> {
  public:
    argvector(const std::string& argstring);
    argvector(int argc, const char* const* argv);
};

class argparser {
  public:
    struct option {
        const char* name;
        enum {no_argument, required_argument, optional_argument} has_arg;
        int* flag;
        int val;
    };

    std::string optarg;
    unsigned int optind;
    
    argparser(argvector& argv) : argv(argv) {
        optind = 0;
    }
    int getopt(std::string* shortopts, const struct option* longopts,
                int* longindex);
    
  protected:
    argvector& argv;
};

//================== Helper for apps' processArgs ========================

// skips the number of characters equal to the length of given text
// does not check whether the skipped characters are the same as it
struct skip
{
    const char* text;
    skip(const char* text) : text(text) {}
};

std::istream& operator >> (std::istream& stream, const skip& x);

#endif /* ARGPARSER_H */
