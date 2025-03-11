/*(2INCO)  Practical Assignment
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
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>    // for execlp
#include <mqueue.h>    // for mq_open, mq_send, mq_receive, etc.

#include "settings.h"  // Suppose it defines N_SERV1, N_SERV2, etc.
#include "messages.h"  // Suppose it defines the request/response structures

/* 
 * We'll store queue names in global (or file-scoped) arrays.
 * Typically, each queue name is unique to avoid collisions.
 */
char client2dealer_name[64];
char dealer2worker1_name[64];
char dealer2worker2_name[64];
char worker2dealer_name[64];

static void create_queue_names(void)
{
    // Example: /mq_req_grpX_<pid>
    sprintf(client2dealer_name,   "/mq_req_group112_%d", getpid());
    sprintf(dealer2worker1_name, "/mq_s1_group112_%d",  getpid());
    sprintf(dealer2worker2_name, "/mq_s2_group112_%d",  getpid());
    sprintf(worker2dealer_name,   "/mq_rep_group112_%d", getpid());
}

/* 
 * For debugging: retrieve and print queue attributes 
 * (similar to 'getattr' in the template).
 */
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

/*
 * Optional helper to check if client is still running:
 * E.g. if you store the client PID, you can use kill(pid, 0).
 */
bool is_process_alive(pid_t pid)
{
    // If kill(pid, 0) == 0, process exists; if ESRCH, it's gone
    if (kill(pid, 0) == 0) {
        return true;
    }
    if (errno == ESRCH) {
        return false;
    }
    // if kill() fails for something else, treat as alive 
    // (or handle differently, depending on your design)
    return true;
}

bool is_client_alive(pid_t client_pid) {
    int status;
    
    // Check if the process has already terminated (non-blocking)
    if (waitpid(client_pid, &status, WNOHANG) > 0) {
        return false;  // Process has exited
    }

    // Check if process exists
    if (kill(client_pid, 0) == 0) {
        return true;  // Process is still alive
    } else if (errno == ESRCH) {
        return false; // Process does not exist
    } else if (errno == EPERM) {
        return true;  // Process exists but we lack permission to check
    }
    
    return false;  // Default to false for unexpected errors
}

int main(int argc, char *argv[])
{
    // 1) Command-line argument check (as in the template)
    if (argc != 1)
    {
        fprintf(stderr, "%s: invalid arguments\n", argv[0]);
        exit(1);
    }
    
    // 2) Create unique message queue names
    create_queue_names();

    // 3) Define queue attributes
    RequestMessage req;
    ResponseMessage rsp;
    struct mq_attr attr_req, attr_s1, attr_s2, attr_rep;
    memset(&attr_req, 0, sizeof(attr_req));
    memset(&attr_s1, 0, sizeof(attr_s1));
    memset(&attr_s2, 0, sizeof(attr_s2));
    memset(&attr_rep, 0, sizeof(attr_rep));
    
    // For illustration, allow up to 10 messages in each queue
    attr_req.mq_maxmsg  = 10;
    attr_req.mq_msgsize = sizeof(RequestMessage); // from messages.h
    attr_s1.mq_maxmsg   = 10;
    attr_s1.mq_msgsize  = sizeof(RequestMessage);
    attr_s2.mq_maxmsg   = 10;
    attr_s2.mq_msgsize  = sizeof(RequestMessage);
    attr_rep.mq_maxmsg  = 10;
    attr_rep.mq_msgsize = sizeof(ResponseMessage);

    // 4) Create the four queues:
    //    1) client -> dealer (Req)
    //    2) dealer -> worker1 (S1)
    //    3) dealer -> worker2 (S2)
    //    4) worker -> dealer (Rep)
    attr_req.mq_flags = O_NONBLOCK;
    mqd_t qd_req = mq_open(client2dealer_name, O_RDONLY | O_CREAT | O_EXCL, 0600, &attr_req);
    mqd_t qd_s1 = mq_open(dealer2worker1_name, O_WRONLY | O_CREAT | O_EXCL, 0600, &attr_s1);
    mqd_t qd_s2 = mq_open(dealer2worker2_name, O_WRONLY | O_CREAT | O_EXCL, 0600, &attr_s2);
    mqd_t qd_rep = mq_open(worker2dealer_name, O_RDONLY | O_CREAT | O_EXCL, 0600, &attr_rep);

    // Print debug info
    // show_queue_info(qd_req);
    // show_queue_info(qd_s1);
    // show_queue_info(qd_s2);
    // show_queue_info(qd_rep);

    // 5) Fork the client process
    //    (In reality, you might do something like execlp("./client", ...) 
    //     or store its PID. For now, we’ll demonstrate a minimal approach.)
    pid_t client_pid = fork();
    if (client_pid < 0) {
        perror("fork client failed");
        exit(1);
    }
    if (client_pid == 0) {
        // In child: client code
        // e.g., execlp("./client", "client", (char*)NULL);
        // or do some test code that sends messages to qd_req...
        execlp("./client", "client", client2dealer_name, (char*)NULL);
        // Sleep or exit to simulate client finishing
        usleep(1);
        exit(0);
    }

    // 6) Fork the worker processes
    //    For instance, one worker_s1 that reads from S1,
    //    and one worker_s2 that reads from S2.
    //    In practice, you'd do execlp("./worker_s1", ...), or similar.
    pid_t worker1_pids[N_SERV1];
    for(int i=0;i<N_SERV1; ++ i) {
      pid_t worker1_pid = fork();
      if (worker1_pid < 0) {
          perror("fork worker1 failed");
          exit(1);
      }
      if (worker1_pid == 0) {
          // Child: Worker1 code
          // Example: open S1 for reading, Rep for writing
          execlp("./worker_s1",
       "worker_s1",        // argv[0] in the new process
       dealer2worker1_name,        // argv[1] (S1 queue)
       worker2dealer_name,       // argv[2] (Rsp queue)
    (char*)NULL);
          // Close and exit
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
          // Child: Worker1 code
          // Example: open S1 for reading, Rep for writing
          execlp("./worker_s2",
       "worker_s2",        // argv[0] in the new process
       dealer2worker2_name,        // argv[1] (S1 queue)
       worker2dealer_name,       // argv[2] (Rsp queue)
    (char*)NULL);
          // Close and exit
          exit(0);
      } else {
        worker2_pids[i] = worker2_pid;
      }
    }
    
    while (true) {
        // printf("pula router\n");
            // Receive from qd_req in non-blocking mode
            if (mq_receive(qd_req, (char *)&req, sizeof(req), NULL) == -1) {
                if (errno == EAGAIN) {
                    printf("client queue empty\n");
                    // No request is currently available.
                    // We do NOT break; we just skip routing.
                } else {
                    perror("mq_receive qd_req");
                }
            } else {
                // We got a request (req) successfully, so handleentAlive){reqCount++;}
                if (req.serviceType == 1) {
                    mq_send(qd_s1, (char *)&req, sizeof(req), 0);
                } else if (req.serviceType == 2) {
                    mq_send(qd_s2, (char *)&req, sizeof(req), 0);
                } else {
                    printf("Invalid service type\n");
                }
            }
                    // 2) Receive from qd_rep in non-blocking mode
            if (mq_receive(qd_rep, (char *)&rsp, sizeof(rsp), NULL) == -1) {
                if (errno == EAGAIN) {
                    // No response is currently available.
                } else {
                    perror("mq_receive qd_rep");
                }
            } else {
                // We got a response (rsp)
                printf("%d -> %d\n",
                     rsp.id, rsp.result);
            }
            // printf("%d", rsp.id);
            // 3) Check if client is alive:
            struct mq_attr attr;
            mq_getattr(qd_req, &attr);
            if (!is_client_alive(client_pid) && attr.mq_curmsgs == 0) {
                // printf("client: Client has terminated.\n");
                // The client is gone, time to shut down
                // clientAlive = false;
                break;
            }
    }
    
    // 8) Wait until the client has stopped
    //    In a loop, you could do something like:
    // printf("client: Client has terminated.\n");
    while (is_process_alive(client_pid)) {
        // Poll for more requests, route them, etc.
        // This code is just a stub:
        usleep(1);
    

    // 9) Wait for workers to finish any outstanding jobs, 
    //    or send them a termination message if you prefer.

    // 10) Wait for each child to terminate
    waitpid(client_pid, NULL, 0);
    printf("client: Client has terminated.\n");
    for (int i = 0; i < N_SERV1; i++) {
        RequestMessage shutdown_request;
        shutdown_request.id = -1;   // Sentinel value for termination
        shutdown_request.serviceType = 0;  // Any value (ignored by worker)
        shutdown_request.input = 0; // Any value (ignored by worker)

        if (mq_send(qd_s1, (const char*)&shutdown_request, sizeof(shutdown_request), 0) == -1) {
            perror("client: mq_send (shutdown request S1) failed");
        } else {
            printf("client: Sent shutdown request %d/%d to worker1 queue.\n", i + 1, N_SERV1);
        }
    }

    for (int i = 0; i < N_SERV2; i++) {
        RequestMessage shutdown_request;
        shutdown_request.id = -1;   // Sentinel value for termination
        shutdown_request.serviceType = 0;  // Any value (ignored by worker)
        shutdown_request.input = 0; // Any value (ignored by worker)

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

    // 11) Clean up the message queues
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