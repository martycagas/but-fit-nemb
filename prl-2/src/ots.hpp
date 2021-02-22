/*
 * ODD-EVEN TRANSPOSITION SORT
 *
 * Brno University of Technology,
 * Parallel and Distributed Algorithms course
 *
 * Martin Cagas <xcagas01@stud.fit.vutbr.cz>
 */

#ifndef _OTS_HPP_
#define _OTS_HPP_

// library includes
#include <chrono>
#include <fstream>
#include <iostream>
#include <mpi.h>

// return codes
#define RET_OK 0
#define RET_FILE_NOT_FOUND 404

// function prototypes
int main(int argc, char *argv[]);

#endif // _OTS_HPP_
