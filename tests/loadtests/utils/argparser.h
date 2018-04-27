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
