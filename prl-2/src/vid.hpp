/*
 * VISIBILITY
 * 
 * Brno University of Technology,
 * Parallel and Distributed Algorithms course
 *
 * Martin Cagas <xcagas01@stud.fit.vutbr.cz>
 */

#ifndef _VID_HPP_
#define _VID_HPP_

// LIBRARY INCLUDES
#include <mpi.h>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <math.h>
#include <errno.h>
#include <chrono>

// TODO: Doxygen

// RETURN CODES
#define RET_OK              0
#define RET_INPUT_ERR       11
#define RET_FILE_NOT_FOUND  404

// FUNCTION PROTOTYPES
int main(int argc, char * argv[]);

#endif // _VID_HPP_
