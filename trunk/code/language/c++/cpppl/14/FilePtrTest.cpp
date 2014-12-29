/*
 *
 *
 *
 */

#include "FilePtr.hpp"

int main()
{
    try {
	FilePtr           fp("test", "w");
	
	throw FilePtr::FilePtrErr("a error ocurred");
    }
    catch (FilePtr::FilePtrErr& e) {
	e.print_debug(); 
	return -1;
    }
    
    return 0;
}







