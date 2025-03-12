/* 
 * Operating Systems  (2INCO)  Practical Assignment
 * Interprocess Communication
 *
 * Alexandru Radu (1953451)
 * Sebastian Georgescu (1926209)
 * Teodor Krajc (STUDENT_NR_3)
 *
 * Grading:
 * Your work will be evaluated based on the following criteria:
 * - Satisfaction of all the specifications
 * - Correctness of the program
 * - Coding style
 * - Report quality
 * - Deadlock analysis
 */

#ifndef MESSAGES_H
#define MESSAGES_H

// define the data structures for your messages here

#pragma pack(1)
typedef struct {
    int  id;
    int  serviceType;
    int input;
}RequestMessage;
#pragma pack()

#pragma pack(1)
typedef struct {
    int  id;
    int result;
} ResponseMessage;
#pragma pack()
#endif
