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

#include "messages.h"   // Suppose this defines MQ_REQUEST_MESSAGE, MQ_RESPONSE_MESSAGE
#include "service1.h"   // For any Service-1-specific logic (if needed)

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
     *  - We receive two arguments: the name of the S1 queue, and the name of the Rsp queue.
     *  - We repeatedly:
     *      1) read from the S1 queue (the new job)
     *      2) wait a random amount of time (rsleep(10000); for example)
     *      3) do the job
     *      4) write the results to the Rsp queue
     *  - Then close the message queues.
     */

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <S1QueueName> <RspQueueName>\n", argv[0]);
        exit(1);
    }

    const char *s1QueueName  = argv[1];
    const char *rspQueueName = argv[2];

    // 1) Open the S1 queue in READ-ONLY mode
    mqd_t mq_s1 = mq_open(s1QueueName, O_RDONLY);
    if (mq_s1 == (mqd_t)-1) {
        perror("worker1: mq_open(S1) failed");
        exit(1);
    }

    // 2) Open the Rsp queue in WRITE-ONLY mode
    mqd_t mq_rsp = mq_open(rspQueueName, O_WRONLY);
    if (mq_rsp == (mqd_t)-1) {
        perror("worker1: mq_open(Rsp) failed");
        mq_close(mq_s1);
        exit(1);
    }

    // printf("worker1: listening on '%s', will respond via '%s'.\n",
    //        s1QueueName, rspQueueName);
    RequestMessage req;
    ssize_t bytes_read;
    // 3) Repeatedly read, process, and send results
    int cnt = 0;
    while (true)
    {
        // printf("BEFORE\n");
        struct mq_attr attr;
        mq_getattr(mq_s1, &attr);  // Get the current attributes of the queue

        // printf("BEFORE\n");
        if (attr.mq_curmsgs == 0) {
            // usleep(100000); // Sleep for 100ms to avoid busy-waiting
            // break;
            continue;
        }

        size_t expected_size = sizeof(RequestMessage);

        
        // printf("mq_msgsize = %ld, expected size = %ld\n", attr.mq_msgsize, expected_size);
        // printf("MID\n");
        memset(&req, 0, sizeof(RequestMessage));
        bytes_read = mq_receive(mq_s1, (char*)&req, sizeof(RequestMessage), NULL);
        // a) Read one job from the S1 queue
        
        // printf("AFTER\n");
        // bytes_read = mq_receive(mq_s1, (char*)&req, sizeof(RequestMessage), NULL);
        if (bytes_read < (ssize_t)sizeof(req)) {
            // fprintf(stderr, "worker1: incomplete message received, stopping.\n");
            break;
        }

        // Show that we've received a job
        // printf("worker1: received job: a=%d, b=%d, c='%d'\n",
        //        req.id, req.serviceType, req.input);

        // b) Sleep a random time (simulate work)
        rsleep(100000);  // up to 10000 microseconds (10ms)

        // c) Do the job - example: fill in a response
        ResponseMessage rsp;
        int resultValue = service(req.input);
        rsp.id = req.id;
        rsp.result = resultValue;

        // 6) Send the response to the Rsp queue
        if (mq_send(mq_rsp, (const char*)&rsp, sizeof(rsp), 0) == -1) {
            perror("worker_s1: mq_send(Rsp) failed");
            break;
        }
        // cnt++;
        // printf("cnt: %d worker_s1: completed job ID=%d -> result=%d\n",cnt ,
        //        rsp.id, rsp.result);

        /*
         * If you want a special 'shutdown' or 'stop' condition,
         * you can define it in the request. For example:
         * if (req.requestID < 0) {  // sentinel
         *     printf("worker_s1: shutdown request received.\n");
         *     break;
         * }
         */
        if (req.id < 0) {  // sentinel
              printf("worker_s1: shutdown request received.\n");
              break;
          }
    }

    // 7) Close the queues and finish
    mq_close(mq_rsp);
    mq_close(mq_s1);

    printf("worker_s1: finished.\n");
    return 0;
}