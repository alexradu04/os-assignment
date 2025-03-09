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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>      // for perror()
#include <unistd.h>     // for getpid()
#include <mqueue.h>     // for mq_open, mq_send, mq_close
#include <time.h>       // for time()
#include <string.h>     // for strncpy, etc.

#include "messages.h"   // Suppose this defines MQ_REQUEST_MESSAGE, MQ_RESPONSE_MESSAGE
#include "request.h"    // Suppose this offers getNextRequest() or similar

/*
 * rsleep(t):
 *  This function can implement a short 'randomized' sleep or any 
 *  artificially introduced delay if desired. If you don't need it,
 *  you can remove calls to rsleep() from the code below.
 */
static void rsleep (int t)
{
    // Example: sleep for a random fraction of t seconds
    // or just do a regular sleep(t).
    // For demonstration, let's do a simple sleep:
    sleep(t);
}

int main (int argc, char * argv[])
{
    
    // (see message_queue_test() in interprocess_basic.c)
    //  * open the message queue (whose name is provided in the arguments)
    //  * repeatingly:
    //      - get the next job request 
    //      - send the request to the Req message queue
    //    until there are no more requests to send
    //  * close the message queue

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <ReqQueueName>\n", argv[0]);
        exit(1);
    }

    // 1) Open the existing request queue in WRITE-ONLY mode
    const char* reqQueueName = argv[1];
    mqd_t mq_fd_request = mq_open(reqQueueName, O_WRONLY);
    if (mq_fd_request == (mqd_t)-1) {
        perror("client: mq_open (Req queue) failed");
        exit(1);
    }
    printf("client: opened queue '%s' for writing.\n", reqQueueName);

    // 2) Repeatedly get the next request and send it
    //    Suppose request.h has a function like:
    //      int getNextRequest(MQ_REQUEST_MESSAGE* outReq);
    //    that returns 1 if there is a request, 0 if not.

    RequestMessage req;
    while (true)
    {
        // Hypothetical function that populates 'req' 
        // and returns 1 while there are requests left,
        // or 0 if no more requests to process.
        usleep(100000);
        int hasRequest = getNextRequest(&req.id, &req.input, &req.serviceType); 
        if (hasRequest) {
            // No more requests to send
            printf("client: no more requests to send, exiting loop.\n");
            break;
        }

        // Optionally do a short random sleep or delay
        rsleep(1);

        // Send the request to the Req queue
        if (mq_send(mq_fd_request, (const char *)&req, sizeof(req), 0) == -1) {
            perror("client: mq_send failed");
        } else {
            printf("client: sent request with a=%d, b=%d, c='%d'\n",
                   req.id, req.serviceType, req.input);
        }
    }

    // 3) Close the message queue
    mq_close(mq_fd_request);

    printf("client: all done, exiting.\n");
    return 0;
}