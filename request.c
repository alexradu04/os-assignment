/* 
 * Operating Systems (2INCO) Practical Assignment
 * Interprocess Communication
 *
 * Contains functions that are used by the clients
 *
 */

#include "request.h"

// Array of requests
const Request requests[] = {
    {1,  26, 1},
    {2,  5,  2},
    {3,  10, 2},
    {4,  42, 1},
    {5,  31, 2},
    {6,  12, 1},
    {7,  19, 2},
    {8,  7,  1},
    {9,  50, 2},
    {10, 3,  1},
    {11, 25, 2},
    {12, 8,  2},
    {13, 14, 1},
    {14, 44, 1},
    {15, 27, 2},
    {16, 100,2},
    {17, 45, 1},
    {18, 6,  2},
    {19, 9,  1},
    {20, 33, 2},
    {21, 15, 1},
    {22, 60, 2},
    {23, 21, 1},
    {24, 13, 2},
    {25, 89, 1},
    {26, 55, 2},
    {27, 77, 1},
    {28, 29, 2},
    {29, 38, 1},
    {30, 20, 2}
};


// Places the information of the next request in the parameters sent by reference.
// Returns NO_REQ if there is no request to make.
// Returns NO_ERR otherwise.
int getNextRequest(int* jobID, int* data, int* serviceID) {
	static int i = 0;
	static int N_REQUESTS = sizeof(requests) / sizeof(Request);
	if (i >= N_REQUESTS) 
		return NO_REQ;

	*jobID = requests[i].job;
	*data = requests[i].data;
	*serviceID = requests[i].service;		
	++i;
	return NO_ERR;
}
