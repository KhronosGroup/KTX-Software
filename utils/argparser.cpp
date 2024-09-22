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
#include <iostream>
#include "argparser.h"
#include <regex>

using namespace std;

/*
 * Construct from a string of arguments.
 */
argvector::argvector(const string& sArgs)
{
    const string sep(" \t");
    // std::regex does not support negative lookbehind assertions. Darn!
    // RE to extract argument between string start and separator.
    // - Match 0 is whole matching string including separator.
    // - Match 1 is the argument.
    // - Match 2 is an empty string or a backslash.
    // Use negative character class as it is easiest way to handle
    // utf-8 file names with non-Latin characters. $ must not be last
    // in the character class or it will be taken literally not as
    // end of string.
    // Use raw literal to avoid excess backslashes.
    regex re(R"--(^([^$\\ \t]+)(\\?)(?:[ \t]+|$))--");
    size_t pos;

    pos = sArgs.find_first_not_of(sep);
    assert(pos != string::npos);

    auto first = sArgs.begin() + pos;
    auto last = sArgs.end();
    bool continuation = false;
    for (smatch sm; regex_search(first, last, sm, re);) {
        bool needContinuation = false;
#if DEBUG_REGEX
        std::cout << "prefix: " << sm.prefix() << '\n';
        std::cout << "suffix: " << sm.suffix() << '\n';
        std::cout << "match size: " << sm.size() << '\n';
        for(uint32_t i = 0; i < sm.size(); i++) {
            std::cout << "match " << i << ": " << "\"" << sm.str(i) << "\"" << '\n';
        }
#endif
        string arg;
        // All this because std::regex does not support negative
        // lookbehind assertions.
        arg = sm.str(1);
        if (*sm[2].first == '\\') {
            arg += " ";
            needContinuation = true;
        }
        if (continuation) {
            this->back() += arg;
        } else {
            push_back(arg);
        }
        continuation = needContinuation;
        first = sm.suffix().first;
    }
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

