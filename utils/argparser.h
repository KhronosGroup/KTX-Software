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

class argvector : public std::vector<std::string> {
  public:
    argvector() { };
    argvector(const std::string& argstring);
    argvector(int argc, const char* const* argv);
};

class argparser {
  public:
    struct option {
        const char* name;
        enum has_arg_t {no_argument, required_argument, optional_argument} has_arg;
        int* flag;
        int val;

        option(const char* name, has_arg_t has_arg, int* flag, int val) : name(name), has_arg(has_arg), flag(flag), val(val) {}
    };

    std::string optarg;
    unsigned int optind;
    argvector argv;

    argparser(argvector& argv, unsigned int startindex = 0)
        : optind(startindex), argv(argv) { }

    argparser(int argc, const char* const* argv1)
        : optind(1), argv(argc, argv1)  { }

    int getopt(std::string* shortopts, const struct option* longopts,
               int* longindex = nullptr);
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
