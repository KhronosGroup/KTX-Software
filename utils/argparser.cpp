/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2017-2020 Mark Callow, www.edgewise-cosulting.com.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @internal
 * @class argparser
 * @~English
 *
 * @brief parse a command argument list.
 *
 * Can't use getopt_long because it declares argv as char* const*. We
 * need const char* const* when we're parsing an embedded string. Also
 * Windows C library does not have getopt_long.
 *
 * @author Mark Callow, www.edgewise-consulting.com.
 */

#include <assert.h>
#include "argparser.h"

using namespace std;

/*
 * Construct from a string of arguments.
 */
argvector::argvector(const string& sArgs)
{
    const string sep(" \t\n\r\v\f");
    size_t pos;

    pos = sArgs.find_first_not_of(sep);
    assert(pos != string::npos);

    do {
        size_t epos = sArgs.find_first_of(sep, pos);
        size_t len = epos == string::npos ? epos : epos - pos;
        push_back(sArgs.substr(pos, len));
        pos = sArgs.find_first_not_of(sep, epos);
    } while (pos != string::npos);
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
argparser::getopt(string* shortopts, const struct option* longopts,
                  int* /*longindex*/)
{
    if (optind == argv.size())
        return -1;

    string arg;
    arg = argv[optind];
    if (arg[0] != '-' || (arg[0] == '-' && arg.size() == 1))
        return -1;
    optind++;

    int retval = '?';
    if (arg.compare(0, 2, "--") == 0) {
        if (arg.size() == 2) return -1; // " -- " separates options and files
        const struct option* opt = longopts;
        while (opt->name != nullptr) {
            if (arg.compare(2, string::npos, opt->name) == 0) {
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
            opt++;
        }
    } else if (shortopts != nullptr && arg.compare(0, 1, "-") == 0) {
        size_t pos = shortopts->find(arg.substr(1, 1));
        if (pos != string::npos) {
            retval = (*shortopts)[pos];
            if (pos < shortopts->length()
                && ((*shortopts)[++pos] == ':' || (*shortopts)[pos] == ';')) {
                if (optind >= argv.size() || (optarg = argv[optind++])[0] == '-') {
                    optarg.clear();
                    optind--;
                    if ((*shortopts)[pos] == ':') // required argument
                    retval = ':';
                }
            }
        }
    }

    return retval;
}

//================== Helper for apps' processArgs ========================

std::istream& operator >> (std::istream& stream, const skip& x)
{
    std::ios_base::fmtflags f = stream.flags();
    stream >> std::noskipws;

    char c;
    const char* text = x.text;
    while (stream && *text++)
        stream >> c;

    stream.flags(f);
    return stream;
}

