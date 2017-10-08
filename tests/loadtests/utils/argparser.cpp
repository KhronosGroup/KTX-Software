/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4: */

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

/**
 * @internal
 * @class argparser
 * @~English
 *
 * @brief parse a command argument list.
 *
 * Can't use getopt_long because it declares argv as char* const*. We
 * need const char* const* since we're often parsing an embedded string.
 *
 * @author Mark Callow, www.edgewise-consulting.com.
 */

#include <assert.h>
#include "argparser.h"

/*
 * Construct from a string of arguments.
 */
argvector::argvector(const std::string& sArgs)
{
    const char *sep = " \t\n\r\v\f";
    size_t pos;

    pos = sArgs.find_first_not_of(sep);
    assert(pos != std::string::npos);
    
    do {
        size_t epos = sArgs.find_first_of(sep, pos);
        size_t len = epos == std::string::npos ? epos : epos - pos;
        push_back(sArgs.substr(pos, len));
        pos = sArgs.find_first_not_of(sep, epos);
    } while (pos != std::string::npos);
}

/*
 * Construct from an array of C strings
 */
argvector::argvector(int argc, const char* const* argv)
{
    for (int i = 0; i < argc; i++) {
        push_back(argv[i]);
    }
}

/*
 * Functions the same as getopt_long. See `man 3 getopt_long`.
 */
int
argparser::getopt(std::string* shortopts, const struct option* longopts,
                  int* longindex)
{
    if (optind == argv.size())
        return -1;

    std::string arg;
    arg = argv[optind];
    if (arg[0] != '-')
        return -1;
    optind++;
    
    int retval = '?';
    if (arg.compare(0, 2, "--") == 0) {
        const struct option* opt = longopts;
        while (opt->name != nullptr) {
            if (arg.compare(2, std::string::npos, opt->name) == 0) {
                retval = opt->val;
                if (opt->has_arg != option::no_argument) {
                    if (optind >= argv.size() || (optarg = argv[optind++])[0] == '-') {
                        optarg.clear();
                        optind--;
                        if (opt->has_arg == option::required_argument)
                            retval = ':';
                    }
                }
                if (opt->flag != nullptr) {
                    *opt->flag = opt->val;
                    retval = 0;
                }
                break;
            }
        }
    } else if (shortopts != nullptr && arg.compare(0, 1, "-") == 0) {
        size_t pos = shortopts->find(arg.substr(1, 1));
        if (pos != std::string::npos) {
            retval = (*shortopts)[pos];
            if ((pos < shortopts->length()) && (*shortopts)[++pos] == ':') {
                if (optind >= argv.size() || (optarg = argv[optind++])[0] == '-') {
                    optarg.clear();
                    optind--;
                    retval = ':';
                }
            }
        }
    }

    return retval;
}
