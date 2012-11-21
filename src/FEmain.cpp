//
//  main.cpp
//  Frontend_bin
//
//  Created by Massimo Gengarelli on 18/11/12.
//  Copyright (c) 2012 Massimo Gengarelli. All rights reserved.
//

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

