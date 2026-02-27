///////////////////////////////////////////////////////////////////////////////
// sw_version.h
// ============
// Lists the current software version number
//
//  AUTHOR: Scott Gray - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Jan. 1st, 2025
///////////////////////////////////////////////////////////////////////////////

#ifndef SW_VERSION_H
#define SW_VERSION_H

#include <iostream>         // error handling and output

// software version number	
const char* const SW_VERSION = "20260105";

// print the version to the console
static void printSofwareVersion() {
	std::cout << std::endl << "Version: " << SW_VERSION << std::endl;
}
#endif // SW_VERSION_H