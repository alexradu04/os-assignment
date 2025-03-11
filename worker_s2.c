/* 
 * Operating Systems (2INCO) Practical Assignment
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
#include <errno.h>      // perror()
#include <unistd.h>     // getpid()
#include <mqueue.h>     // mq_open, mq_receive, mq_send, mq_close
#include <time.h>       // time()
#include <string.h>     // strcpy, etc.

#include "messages.h"   // Defines MQ_REQUEST_MESSAGE, MQ_RESPONSE_MESSAGE
#include "service2.h"   // For any Service-2-specific logic (if needed)

static void rsleep (int t)
{
    static bool first_call = true;
    
    if (first_call) {
        srandom(time(NULL) ^ getpid());
        first_call = false;
    }
    usleep(random() % t);
}

int main(int argc, char *argv[])
{
    /*
     * According to the instructions:
     *  - We receive two arguments: the name of the S2 queue, and the name of the Rsp queue.
     *  - We repeatedly:
     *      1) Read from the S2 queue (the new job)
     *      2) Wait a random amount of time (rsleep(10000))
     *      3) Process the job
     *      4) Write the results to the Rsp queue
     *  - Then close the message queues.
     */

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <S2QueueName> <RspQueueName>\n", argv[0]);
        exit(1);
    }

    const char *s2QueueName  = argv[1];
    const char *rspQueueName = argv[2];

    // 1) Open the S2 queue in READ-ONLY mode
    mqd_t mq_s2 = mq_open(s2QueueName, O_RDONLY);
    if (mq_s2 == (mqd_t)-1) {
        perror("worker2: mq_open(S2) failed");
        exit(1);
    }

    // 2) Open the Rsp queue in WRITE-ONLY mode
    mqd_t mq_rsp = mq_open(rspQueueName, O_WRONLY);
    if (mq_rsp == (mqd_t)-1) {
        perror("worker2: mq_open(Rsp) failed");
        mq_close(mq_s2);
        exit(1);
    }

    // printf("worker2: listening on '%s', will respond via '%s'.\n",
    //        s2QueueName, rspQueueName);

    RequestMessage req;
    ssize_t bytes_read;
    
    // 3) Repeatedly read, process, and send results
    int cnt=0;
    while (true)
    {
        struct mq_attr attr;
        mq_getattr(mq_s2, &attr);  // Get the current attributes of the queue
        
        // printf("MID\n");
        memset(&req, 0, sizeof(RequestMessage));
        bytes_read = mq_receive(mq_s2, (char*)&req, sizeof(RequestMessage), NULL);
        
        // printf("AFTER\n");

        if (bytes_read < (ssize_t)sizeof(req)) {
            fprintf(stderr, "worker2: incomplete message received, stopping.\n");
            break;
        }

        // Show that we've received a job
        // printf("worker2: received job: id=%d, serviceType=%d, input='%d'\n",
        //        req.id, req.serviceType, req.input);

        // b) Sleep a random time (simulate work)
        rsleep(10000);  // up to 10000 microseconds (10ms)

        // c) Process the job - example: fill in a response
        ResponseMessage rsp;
        int resultValue = service(req.input);
        rsp.id = req.id;
        rsp.result = resultValue;

        // 6) Send the response to the Rsp queue
        if (mq_send(mq_rsp, (const char*)&rsp, sizeof(rsp), 0) == -1) {
            perror("worker2: mq_send(Rsp) failed");
            break;
        }
        cnt++;
        // printf("cnt: %d worker2: completed job ID=%d -> result=%d\n",cnt,
        //        rsp.id, rsp.result);
        if (req.id < 0) {  // sentinel
              printf("worker_s1: shutdown request received.\n");
              break;
          }
    }

    // 7) Close the queues and finish
    mq_close(mq_rsp);
    mq_close(mq_s2);

    printf("worker2: finished.\n");
    return 0;
}