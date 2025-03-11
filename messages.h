/* 
 * Operating Systems  (2INCO)  Practical Assignment
 * Interprocess Communication
 *
 * STUDENT_NAME_1 (STUDENT_NR_1)
 * STUDENT_NAME_2 (STUDENT_NR_2)
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

/* 
 * Define the structures used for communication between
 * the client, the router-dealer, and the workers.
 * Make sure the size of each structure matches (or is less than)
 * the message queue's mq_msgsize you specify at mq_open().
 */

/* 
 * Example: A request message from the client to the dealer.
 * You can add/remove fields depending on what data you need. 
 */
#pragma pack(1)
typedef struct {
    int  id;
    int  serviceType;
    // text field for passing strings
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
