/*
 *  Architecture - A simple (yet not working) architecture for cloud computing
 *  Copyright (C) 2013 Massimo Gengarelli <massimo.gengarelli@gmail.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>
#include <boost/program_options.hpp>
#include <string>

#include "FrontendItf.h"

using namespace boost::program_options;

int main(int argc, const char * argv[])
{
    options_description anOptDesc("Allowed options");
    anOptDesc.add_options()
        ("help,h", "produces this help message")
        ("frontendid,f", value<std::string>(), "create a frontend using the ID passed as argument");
    
    variables_map aVariablesMap;
    store(parse_command_line(argc, argv, anOptDesc), aVariablesMap);
    
    if (aVariablesMap.count("help")) {
        std::cout << anOptDesc << std::endl;
        return 0;
    }
    
    if (aVariablesMap.count("frontendid")) {
        std::string aFrontendId(aVariablesMap["frontendid"].as<std::string>());
        FrontendItf aFrontend(aFrontendId);
        aFrontend.configure();
        aFrontend.start();
    }
    else {
        std::cout << anOptDesc << std::endl;
        return 0;
    }
    
    return 0;
}

