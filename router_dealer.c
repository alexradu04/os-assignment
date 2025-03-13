/*(2INCO)  Practical Assignment
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
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>    // for execlp
#include <mqueue.h>    // for mq
#include <time.h>

#include "settings.h"  
#include "messages.h"  


char client2dealer_name[64];
char dealer2worker1_name[64];
char dealer2worker2_name[64];
char worker2dealer_name[64];

static void create_queue_names(void)
{
    sprintf(client2dealer_name,   "/mq_req_group112_%d", getpid());
    sprintf(dealer2worker1_name, "/mq_s1_group112_%d",  getpid());
    sprintf(dealer2worker2_name, "/mq_s2_group112_%d",  getpid());
    sprintf(worker2dealer_name,   "/mq_rep_group112_%d", getpid());
}

static void show_queue_info(mqd_t qd)
{
    struct mq_attr attr;
    if (mq_getattr(qd, &attr) == -1) {
        perror("mq_getattr() failed");
        exit(1);
    }
    fprintf(stderr, 
            "PID %d: mqdes=%d max=%ld size=%ld current=%ld\n",
            getpid(),
            (int)qd,
            attr.mq_maxmsg,
            attr.mq_msgsize,
            attr.mq_curmsgs);
}


bool is_process_alive(pid_t pid)
{
    // If kill(pid, 0) == 0 process exists
    if (kill(pid, 0) == 0) {
        return true;
    }
    if (errno == ESRCH) {
        return false;
    }
    if (errno == EPERM) {
        // In case of EPERM, we need to check if the process is still in the process table
        char proc_path[64];
        snprintf(proc_path, sizeof(proc_path), "/proc/%d", pid);
        struct stat sts;
        if (stat(proc_path, &sts) == -1 && errno == ENOENT) {
            return false;
        }
        return true;
    }
    return false;
}

bool is_client_alive(pid_t client_pid) {
    // return is_process_alive(client_pid);
    int status;
    
    if (waitpid(client_pid, &status, WNOHANG) > 0) {
        return false;  
    }

    if (kill(client_pid, 0) == 0) {
        return true; 
    } else if (errno == ESRCH) {
        return false;
    } else if (errno == EPERM) {
        return true;  
    }
    
    return true; 
}

int main(int argc, char *argv[])
{

    // exit(0);
    if (argc != 1)
    {
        fprintf(stderr, "%s: invalid arguments\n", argv[0]);
        exit(1);
    }
    
    create_queue_names();
    RequestMessage req;
    ResponseMessage rsp;
    struct mq_attr attr_req, attr_s1, attr_s2, attr_rep;
    memset(&attr_req, 0, sizeof(attr_req));
    memset(&attr_s1, 0, sizeof(attr_s1));
    memset(&attr_s2, 0, sizeof(attr_s2));
    memset(&attr_rep, 0, sizeof(attr_rep));
    
    attr_req.mq_maxmsg  = 10;
    attr_req.mq_msgsize = sizeof(RequestMessage); 
    attr_s1.mq_maxmsg   = 10;
    attr_s1.mq_msgsize  = sizeof(RequestMessage);
    attr_s2.mq_maxmsg   = 10;
    attr_s2.mq_msgsize  = sizeof(RequestMessage);
    attr_rep.mq_maxmsg  = 10;
    attr_rep.mq_msgsize = sizeof(ResponseMessage);

    // Create the queues:
    attr_req.mq_flags = O_NONBLOCK;
    mqd_t qd_req = mq_open(client2dealer_name, O_RDONLY | O_CREAT | O_EXCL | O_NONBLOCK, 0600, &attr_req);
    mqd_t qd_s1 = mq_open(dealer2worker1_name, O_WRONLY | O_CREAT | O_EXCL | O_NONBLOCK, 0600, &attr_s1);
    mqd_t qd_s2 = mq_open(dealer2worker2_name, O_WRONLY | O_CREAT | O_EXCL | O_NONBLOCK, 0600, &attr_s2);
    mqd_t qd_rep = mq_open(worker2dealer_name, O_RDONLY | O_CREAT | O_EXCL | O_NONBLOCK, 0600, &attr_rep);

    // debug
    // show_queue_info(qd_req);
    // show_queue_info(qd_s1);
    // show_queue_info(qd_s2);
    // show_queue_info(qd_rep);

    pid_t client_pid = fork();
    if (client_pid < 0) {
        perror("fork client failed");
        exit(1);
    }
    if (client_pid == 0) {
        execlp("./client", "client", client2dealer_name, (char*)NULL);
        usleep(1);
        exit(0);
    }
    exit(0);
    pid_t worker1_pids[N_SERV1];
    for(int i=0;i<N_SERV1; ++ i) {
      pid_t worker1_pid = fork();
      if (worker1_pid < 0) {
          perror("fork worker1 failed");
          exit(1);
      }
      if (worker1_pid == 0) {
          execlp("./worker_s1",
       "worker_s1",        
       dealer2worker1_name,       
       worker2dealer_name,       
    (char*)NULL);
          exit(0);
      } else {
        worker1_pids[i] = worker1_pid;
      }
    }

    pid_t worker2_pids[N_SERV2];
    for(int i=0;i<N_SERV2; ++ i) {
      pid_t worker2_pid = fork();
      if (worker2_pid < 0) {
          perror("fork worker2 failed");
          exit(1);
      }
      if (worker2_pid == 0) {
          execlp("./worker_s2",
       "worker_s2",        
       dealer2worker2_name,        
       worker2dealer_name,       
    (char*)NULL);
          exit(0);
      } else {
        worker2_pids[i] = worker2_pid;
      }
    }
    time_t beforeWhile = time(NULL);
    while ((true) ){
        if((time(NULL) - beforeWhile >= 2))
        {
            exit(0);
        }
        RequestMessage req;
        if (mq_receive(qd_req, (char *)&req, sizeof(req), NULL) == -1) {
            if (errno == EAGAIN) {
                usleep(50000);
            } else {
                perror("dealer: mq_receive qd_req");
            }
        } else {
            if (req.id >= 0) {
                if (req.serviceType == 1) {
                    if (mq_send(qd_s1, (char *)&req, sizeof(req), 0) == -1) {
                        perror("dealer: mq_send qd_s1");
                    }
                } else if (req.serviceType == 2) {
                    if (mq_send(qd_s2, (char *)&req, sizeof(req), 0) == -1) {
                        perror("dealer: mq_send qd_s2");
                    }
                } else {
                    fprintf(stderr, "dealer: invalid serviceType %d\n", req.serviceType);
                }
            } else {
                fprintf(stderr, "dealer: received negative ID request \n");
            }
        }

        ResponseMessage rsp;
        if (mq_receive(qd_rep, (char *)&rsp, sizeof(rsp), NULL) == -1) {
            if (errno != EAGAIN) {
                perror("dealer: mq_receive qd_rep");
            }
        } else {
            printf("%d -> %d\n", rsp.id, rsp.result);
            fflush(stdout);
        }

        struct mq_attr attr_req_state;
        if (mq_getattr(qd_req, &attr_req_state) == -1) {
            perror("mq_getattr qd_req");
            break;
        }

        if (!is_client_alive(client_pid) && attr_req_state.mq_curmsgs == 0) {

            time_t startTime = time(NULL);
            while (time(NULL) - startTime < 1) {  
                if (mq_receive(qd_rep, (char *)&rsp, sizeof(rsp), NULL) == -1) {
                    if (errno == EAGAIN) {
                        usleep(50000);
                    } else {
                        perror("dealer: draining qd_rep");
                        break;
                    }
                } else {
                    printf("%d -> %d\n", rsp.id, rsp.result);
                    fflush(stdout);
                    startTime = time(NULL);
                }
            }
            break;
        }
    }
    while (is_process_alive(client_pid)) {
        usleep(1);
    


    waitpid(client_pid, NULL, 0);
    printf("client: Client has terminated.\n");
    for (int i = 0; i < N_SERV1; i++) {
        RequestMessage shutdown_request;
        shutdown_request.id = -1;  
        shutdown_request.serviceType = 0;  
        shutdown_request.input = 0; 

        if (mq_send(qd_s1, (const char*)&shutdown_request, sizeof(shutdown_request), 0) == -1) {
            perror("client: mq_send (shutdown request S1) failed");
        } else {
            printf("client: Sent shutdown request %d/%d to worker1 queue.\n", i + 1, N_SERV1);
        }
    }

    for (int i = 0; i < N_SERV2; i++) {
        RequestMessage shutdown_request;
        shutdown_request.id = -1;   
        shutdown_request.serviceType = 0; 
        shutdown_request.input = 0; 

        if (mq_send(qd_s2, (const char*)&shutdown_request, sizeof(shutdown_request), 0) == -1) {
            perror("client: mq_send (shutdown request S2) failed");
        } else {
            printf("client: Sent shutdown request %d/%d to worker2 queue.\n", i + 1, N_SERV2);
        }
    }
    for(int i=0;i<N_SERV1;++i) {
        waitpid(worker1_pids[i], NULL, 0);
    }
    for(int i=0;i<N_SERV2;++i) {
        waitpid(worker2_pids[i], NULL, 0);
    }

    //Clean-up
    mq_close(qd_req);
    mq_close(qd_s1);
    mq_close(qd_s2);
    mq_close(qd_rep);
    mq_unlink(client2dealer_name);
    mq_unlink(dealer2worker1_name);
    mq_unlink(dealer2worker2_name);
    mq_unlink(worker2dealer_name);

    fprintf(stderr, "Dealer: Cleaned up all queues. Exiting.\n");
    return 0;
}
}