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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>      // for perror()
#include <unistd.h>     // for getpid()
#include <mqueue.h>     // for mq-stuff
#include <time.h>       // for time()
#include <string.h>     

#include "messages.h"   
#include "request.h"    

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
    const char* reqQueueName = argv[1];
    mqd_t mq_fd_request = mq_open(reqQueueName, O_WRONLY);
    if (mq_fd_request == (mqd_t)-1) {
        perror("client: mq_open (Req queue) failed");
        exit(1);
    }
    // printf("client: opened queue '%s' for writing.\n", reqQueueName);

    RequestMessage req;
    time_t beforeWhile = time(NULL);
    while (true)
    {
        if((time(NULL) - beforeWhile >= 2))
        {
            exit(0);
        }
        usleep(1);
        int hasRequest = getNextRequest(&req.id, &req.input, &req.serviceType); 
        // printf("%d\n", hasRequest);
        if (hasRequest == -1) {
            // printf("client: no more requests to send, exiting loop.\n");
            break;
        }

        usleep(1);

        if (mq_send(mq_fd_request, (const char *)&req, sizeof(req), 0) == -1) {
            perror("client: mq_send failed");
        } else {
            // printf("client: sent request with a=%d, b=%d, c='%d'\n",
            //        req.id, req.serviceType, req.input);
        }
    }

    mq_close(mq_fd_request);

    // printf("client: all done, exiting.\n");
    exit(0);
    return 0;
}