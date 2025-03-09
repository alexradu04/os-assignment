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
#include <mqueue.h>     // for mq-stuff
#include <time.h>       // for time()

#include "messages.h"
#include "service2.h"

static void rsleep (int t);

char* name = "NO_NAME_DEFINED";
mqd_t dealer2worker;
mqd_t worker2dealer;


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
        fprintf(stderr, "Usage: %s <S2QueueName> <RspQueueName>\n", argv[0]);
        exit(1);
    }

    const char *s2QueueName  = argv[1];
    const char *rspQueueName = argv[2];

    // 1) Open the S1 queue in READ-ONLY mode
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

    printf("worker1: listening on '%s', will respond via '%s'.\n",
           s2QueueName, rspQueueName);

    // 3) Repeatedly read, process, and send results
    while (true)
    {
        // a) Read one job from the S1 queue
        RequestMessage req;
        ssize_t bytes_read = mq_receive(mq_s2, (char*)&req, sizeof(req), NULL);

        if (bytes_read == -1) {
            if (errno == EINTR) {
                // If interrupted by a signal, just retry
                continue;
            }
            perror("worker1: mq_receive(S1) failed, stopping");
            break;
        }
        if (bytes_read < (ssize_t)sizeof(req)) {
            fprintf(stderr, "worker1: incomplete message received, stopping.\n");
            break;
        }

        // Show that we've received a job
        printf("worker1: received job: a=%d, b=%d, c='%c'\n",
               req.id, req.serviceType, req.input);

        // b) Sleep a random time (simulate work)
        rsleep(10000);  // up to 10000 microseconds (10ms)

        // c) Do the job - example: fill in a response
        ResponseMessage rsp;
        int resultValue = service(req.input);
        rsp.id = req.id;
        rsp.result    = resultValue;

        // 6) Send the response to the Rsp queue
        if (mq_send(mq_rsp, (const char*)&rsp, sizeof(rsp), 0) == -1) {
            perror("worker_s1: mq_send(Rsp) failed");
            break;
        }
        printf("worker_s1: completed job ID=%d -> result=%d\n",
               rsp.id, rsp.result);

        /*
         * If you want a special 'shutdown' or 'stop' condition,
         * you can define it in the request. For example:
         * if (req.requestID < 0) {  // sentinel
         *     printf("worker_s1: shutdown request received.\n");
         *     break;
         * }
         */
    }

    // 7) Close the queues and finish
    mq_close(mq_rsp);
    mq_close(mq_s2);

    printf("worker_s2: finished.\n");
    return 0;
}

/*
 * rsleep(int t)
 *
 * The calling thread will be suspended for a random amount of time
 * between 0 and t microseconds
 * At the first call, the random generator is seeded with the current time
 */
static void rsleep (int t)
{
    static bool first_call = true;
    
    if (first_call == true)
    {
        srandom (time (NULL) % getpid ());
        first_call = false;
    }
    usleep (random() % t);
}
