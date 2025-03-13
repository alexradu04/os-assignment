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
#include "service2.h"   

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
        fprintf(stderr, "Usage: %s <S2QueueName> <RspQueueName>\n", argv[0]);
        exit(1);
    }

    const char *s2QueueName  = argv[1];
    const char *rspQueueName = argv[2];

    mqd_t mq_s2 = mq_open(s2QueueName, O_RDONLY);
    if (mq_s2 == (mqd_t)-1) {
        perror("worker2: mq_open(S2) failed");
        exit(1);
    }

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
    
    // int cnt=0;
    time_t beforeWhile = time(NULL);
    while (true)
    {
        if((time(NULL) - beforeWhile >= 2))
        {
            exit(0);
        }
        struct mq_attr attr;
        mq_getattr(mq_s2, &attr);  
        
        // printf("MID\n");
        memset(&req, 0, sizeof(RequestMessage));
        bytes_read = mq_receive(mq_s2, (char*)&req, sizeof(RequestMessage), NULL);
        
        // printf("AFTER\n");

        if (bytes_read < (ssize_t)sizeof(req)) {
            fprintf(stderr, "worker2: incomplete message received, stopping.\n");
            break;
        }

        // printf("worker2: received job: id=%d, serviceType=%d, input='%d'\n",
        //        req.id, req.serviceType, req.input);

        rsleep(10000); 

        ResponseMessage rsp;
        int resultValue = service(req.input);
        rsp.id = req.id;
        rsp.result = resultValue;

        if (mq_send(mq_rsp, (const char*)&rsp, sizeof(rsp), 0) == -1) {
            perror("worker2: mq_send(Rsp) failed");
            break;
        }
        // cnt++;
        // printf("cnt: %d worker2: completed job ID=%d -> result=%d\n",cnt,
        //        rsp.id, rsp.result);
        if (req.id < 0) {  
              printf("worker_s1: shutdown request received.\n");
              break;
          }
    }

    mq_close(mq_rsp);
    mq_close(mq_s2);

    printf("worker2: finished.\n");
    return 0;
}