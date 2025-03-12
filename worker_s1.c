/* 
 * Operating Systems (2INCO) Practical Assignment
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
#include "service1.h"   

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

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <S1QueueName> <RspQueueName>\n", argv[0]);
        exit(1);
    }

    const char *s1QueueName  = argv[1];
    const char *rspQueueName = argv[2];

    mqd_t mq_s1 = mq_open(s1QueueName, O_RDONLY);
    if (mq_s1 == (mqd_t)-1) {
        perror("worker1: mq_open(S1) failed");
        exit(1);
    }

    mqd_t mq_rsp = mq_open(rspQueueName, O_WRONLY);
    if (mq_rsp == (mqd_t)-1) {
        perror("worker1: mq_open(Rsp) failed");
        mq_close(mq_s1);
        exit(1);
    }

    // printf("worker1: listening on '%s', will respond via '%s'.\n",
    //        s1QueueName, rspQueueName);
    RequestMessage req;
    ssize_t inputRead;
    int cnt = 0;
    while (true)
    {
        // printf("BEFORE\n");
        struct mq_attr attr;
        mq_getattr(mq_s1, &attr);  

        // printf("BEFORE\n");
        if (attr.mq_curmsgs == 0) {
            // usleep(100000); 
            // break;
            continue;
        }

        size_t expected_size = sizeof(RequestMessage);

        
        // printf("mq_msgsize = %ld, expected size = %ld\n", attr.mq_msgsize, expected_size);
        // printf("MID\n");
        memset(&req, 0, sizeof(RequestMessage));
        inputRead = mq_receive(mq_s1, (char*)&req, sizeof(RequestMessage), NULL);
        
        // printf("AFTER\n");
        // inputRead = mq_receive(mq_s1, (char*)&req, sizeof(RequestMessage), NULL);
        if (inputRead < (ssize_t)sizeof(req)) {
            // fprintf(stderr, "worker1: incomplete message received, stopping.\n");
            break;
        }

        // printf("worker1: received job: a=%d, b=%d, c='%d'\n",
        //        req.id, req.serviceType, req.input);

        rsleep(100000);  

        ResponseMessage rsp;
        int resultValue = service(req.input);
        rsp.id = req.id;
        rsp.result = resultValue;

        if (mq_send(mq_rsp, (const char*)&rsp, sizeof(rsp), 0) == -1) {
            perror("worker_s1: mq_send(Rsp) failed");
            break;
        }
        // cnt++;
        // printf("cnt: %d worker_s1: completed job ID=%d -> result=%d\n",cnt ,
        //        rsp.id, rsp.result);

        
        if (req.id < 0) {  
              printf("worker_s1: shutdown request received.\n");
              break;
          }
    }

    mq_close(mq_rsp);
    mq_close(mq_s1);

    printf("worker_s1: finished.\n");
    return 0;
}