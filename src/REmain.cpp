/* 
 * Receptor
 * No license provided yet, you can't use this file without
 * the explicit permission given by the owners.
 */

#include <iostream>
#include <string>

#include "Receptor.h"

using std::string;

int main(int argc, const char * argv[])
{
    Receptor& aReceptor = Receptor::GetInstance();
    aReceptor.printConfiguration();
    aReceptor.startServer();
    
    return 0;
}

