/*
 * Name: Laura (Kai Sze) Luk
 * AndrewID: kluk
 */

/*
 * tsh - A tiny shell program that supports simple job control and 
 * I/O redirection. A job list was used for implementation.
 *
 * Running jobs in the background, and in the foreground, were implemented.
 * Built-in commands are supported:
 *      -quit: exits/terminates the shell
 *      -jobs: lists out all background jobs
 *      -bg [job]: restarts the job and runs it in the background
 *      -fg [job]: restarts the job and runs it in the foreground
 * 
 * Redirection of jobs is also supported. 
 * The shell runs each executable in its own child process, to avoid
 * corrupting its own state. This implementation makes sure to reap 
 * all zombie children jobs that terminated.
 * Signal blocking and signal handlers were implemented for SIGCHLD,
 * SIGINT, and SIGTSTP. Typing Ctrl-C, or Ctrl-Z, causes a SIGINT signal,
 * or a SIGTSTP signal, respectively, which should be forwarded to an entire
 * process group that contains the foreground job.
 * If none, there is no effect.
 *
 */

#include "csapp.h"
#include "tsh_helper.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

/*
 * If DEBUG is defined, enable contracts and printing on dbg_printf.
 */
#ifdef DEBUG
/* When debugging is enabled, these form aliases to useful functions */
#define dbg_printf(...) printf(__VA_ARGS__)
#define dbg_requires(...) assert(__VA_ARGS__)
#define dbg_assert(...) assert(__VA_ARGS__)
#define dbg_ensures(...) assert(__VA_ARGS__)
#else
/* When debugging is disabled, no code gets generated for these */
#define dbg_printf(...)
#define dbg_requires(...)
#define dbg_assert(...)
#define dbg_ensures(...)
#endif

/* Helper function prototypes */
void builtin_back_fore(struct cmdline_tokens token, sigset_t prev, 
                                                    int backg_flag);
void redirection(struct cmdline_tokens token);

/* Function prototypes */
void eval(const char *cmdline);

void sigchld_handler(int sig);
void sigtstp_handler(int sig);
void sigint_handler(int sig);
void sigquit_handler(int sig);
void cleanup(void);

/*
 * The main() function for our tiny shell implementation.
 * It takes in the number of arguments on the command line, and the array of
 * arguments. It returns an integer. 
 * The general purpose of the main() function is to initialize global 
 * variables, data structures (job list), install signal handlers, and parse
 * the command line in order to obtain arguments/data. 
 * It also prints out error messages, when applicable.
 * The eval() function is called to start our shell.
 */
int main(int argc, char **argv) {
    char c;
    char cmdline[MAXLINE_TSH]; // Cmdline for fgets
    bool emit_prompt = true;   // Emit prompt (default)

    // Redirect stderr to stdout (so that driver will get all output
    // on the pipe connected to stdout)
    if (dup2(STDOUT_FILENO, STDERR_FILENO) < 0) {
        perror("dup2 error");
        exit(1);
    }

    // Parse the command line.
    while ((c = getopt(argc, argv, "hvp")) != EOF) {
        switch (c) {
        case 'h': // Prints help message
            usage();
            break;
        case 'v': // Emits additional diagnostic info
            verbose = true;
            break;
        case 'p': // Disables prompt printing
            emit_prompt = false;
            break;
        default:
            usage();
        }
    }

    // Create environment variable.
    if (putenv("MY_ENV=42") < 0) {
        perror("putenv error");
        exit(1);
    }

    // Set buffering mode of stdout to line buffering.
    // This prevents lines from being printed in the wrong order.
    if (setvbuf(stdout, NULL, _IOLBF, 0) < 0) {
        perror("setvbuf error");
        exit(1);
    }

    // Initialize the job list
    init_job_list();

    // Register a function to clean up the job list on program termination.
    // The function may not run in the case of abnormal termination (e.g. when
    // using exit or terminating due to a signal handler), so in those cases,
    // we trust that the OS will clean up any remaining resources.
    if (atexit(cleanup) < 0) {
        perror("atexit error");
        exit(1);
    }

    // Install the signal handlers
    Signal(SIGINT, sigint_handler);   // Handles Ctrl-C
    Signal(SIGTSTP, sigtstp_handler); // Handles Ctrl-Z
    Signal(SIGCHLD, sigchld_handler); // Handles terminated or stopped child

    Signal(SIGTTIN, SIG_IGN);
    Signal(SIGTTOU, SIG_IGN);

    Signal(SIGQUIT, sigquit_handler);

    // Execute the shell's read/eval loop
    while (true) {
        if (emit_prompt) {
            printf("%s", prompt);

            // We must flush stdout since we are not printing a full line.
            fflush(stdout);
        }

        if ((fgets(cmdline, MAXLINE_TSH, stdin) == NULL) && ferror(stdin)) {
            perror("fgets error");
            exit(1);
        }

        if (feof(stdin)) {
            // End of file (Ctrl-D)
            printf("\n");
            return 0;
        }

        // Remove any trailing newline
        char *newline = strchr(cmdline, '\n');
        if (newline != NULL) {
            *newline = '\0';
        }

        // Evaluate the command line
        eval(cmdline);
    }

    return -1; // control never reaches here
}

/*
 * The eval() function is called within main in order to begin the
 * tiny shell simulation. Evaluation is dependent on incoming command
 * lines and their respective arguments.
 * 
 * Built-in functionalities were executed (quit, jobs, bg, fg). Otherwise,
 * a child process was forked, and executed. 
 * This implementation makes sure to reap all zombie children jobs
 * that terminated.
 * Signal blocking and signal handlers were implemented for SIGCHLD,
 * SIGINT, and SIGTSTP.
 * 
 */
void eval(const char *cmdline) {
    pid_t pid;
    jid_t jid;
    int fd, retval;
    parseline_return parse_result;
    struct cmdline_tokens token;
    sigset_t mask, prev;
    job_state state;

    // parse command line and determine what state the job is
    parse_result = parseline(cmdline, &token);

    if (parse_result == PARSELINE_ERROR || parse_result == PARSELINE_EMPTY) {
        return;
    }
    if (parse_result == PARSELINE_FG) {
        state = FG;
    }
    if (parse_result == PARSELINE_BG) {
        state = BG;
    }

    // initialize blocking masks/vectors
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTSTP);

    // not a built-in command
    if (token.builtin == BUILTIN_NONE) {

        // block signals
        sigprocmask(SIG_BLOCK, &mask, &prev);

        // child process
        if ((pid = fork()) == 0) {
            // unblock signals
            sigprocmask(SIG_UNBLOCK, &mask, NULL);

            // new process group per forked child to successfully ctrl-c
            setpgid(0, 0);

            // redirects input/output
            redirection(token);

            if (execve(token.argv[0], token.argv, environ) < 0) {
                if (errno == ENOENT) {
                    sio_printf("%s: No such file or directory\n", 
                                                            token.argv[0]);
                }
                if (errno == EACCES) {
                    sio_printf("%s: Permission denied\n", token.argv[0]);
                }
                exit(0);
            }

        // parent process
        } else {
            add_job(pid, state, cmdline);

            // background job
            if (state == BG) {
                jid = job_from_pid(pid);
                sio_printf("[%d] (%d) %s\n", jid, pid, cmdline);
            }
            // foreground job
            if (state == FG) {
                while (fg_job() > 0) {
                    sigsuspend(&prev);
                }
            }
        }
        // unblock signals
        sigprocmask(SIG_SETMASK, &prev, NULL);

    // built-in commands
    } else {
        // quit
        if (token.builtin == BUILTIN_QUIT)
            exit(0);

        // background job
        if (token.builtin == BUILTIN_BG) {
            // block signals
            sigprocmask(SIG_BLOCK, &mask, &prev);

            // call helper function with background flag = true
            builtin_back_fore(token, prev, 1);

            // unblock signals
            sigprocmask(SIG_SETMASK, &prev, NULL);
        }

        // builtin foreground job
        else if (token.builtin == BUILTIN_FG) {
            // block signals
            sigprocmask(SIG_BLOCK, &mask, &prev);

            // call helper function with background tag = false
            builtin_back_fore(token, prev, 0);

            // unblock signals
            sigprocmask(SIG_SETMASK, &prev, NULL);
        }

        // jobs
        else if (token.builtin == BUILTIN_JOBS) {

            // redirect output to file
            if (token.outfile != NULL) {
                fd = open(token.outfile, O_WRONLY | O_CREAT | O_TRUNC, 
                                                                DEF_MODE);
                if (fd < 0) {
                    if (errno == ENOENT) {
                        sio_printf("%s: No such file or directory\n",
                                   token.outfile);
                    }
                    if (errno == EACCES) {
                        sio_printf("%s: Permission denied\n", token.outfile);
                    }
                    return;
                }
                sigprocmask(SIG_BLOCK, &mask, &prev);
                list_jobs(fd);
                sigprocmask(SIG_SETMASK, &prev, NULL);

                retval = close(fd);
                if (retval < 0) {
                    perror("close");
                    exit(0);
                }

            } else {
                sigprocmask(SIG_BLOCK, &mask, &prev);
                list_jobs(STDOUT_FILENO);
                sigprocmask(SIG_SETMASK, &prev, NULL);
            }
            return;
        }
    }
}

/*
 * Responsible for possible redirection of input or output signals.
 * Function takes in working token struct from eval() function.
 * There is no (meaningful) return value, but error messages are
 * printed out, if applicable.
 *
 */
void redirection(struct cmdline_tokens token) {
    int retval, out_fd, in_fd;

    // output file redirection
    if (token.outfile != NULL) {
        out_fd = open(token.outfile, O_WRONLY | O_CREAT | O_TRUNC, DEF_MODE);
        if (out_fd < 0) {
            if (errno == ENOENT) {
                sio_printf("%s: No such file or directory\n", token.outfile);
            }
            if (errno == EACCES) {
                sio_printf("%s: Permission denied\n", token.outfile);
            }
            exit(0);
        
        // output redirection to terminal window
        } else {
            dup2(out_fd, STDOUT_FILENO);
            retval = close(out_fd);
            if (retval < 0) {
                perror("close");
                exit(0);
            }
        }
    }

    // input file redirection
    if (token.infile != NULL) {
        in_fd = open(token.infile, O_RDONLY, DEF_MODE);
        if (in_fd < 0) {
            if (errno == ENOENT) {
                sio_printf("%s: No such file or directory\n", token.infile);
            }
            if (errno == EACCES) {
                sio_printf("%s: Permission denied\n", token.infile);
            }
            exit(0);
        }
        // input redirection to terminal window  
        dup2(in_fd, STDIN_FILENO);
        retval = close(in_fd);
        if (retval < 0) {
            perror("close");
            exit(0);
        }
    }
}

/*
 * Responsible for the builtin commands of BUILTIN_BG, which is running
 * a job in the background, and BUILTIN_FG, which runs a job in the
 * foreground.
 *
 * Error checking and parsing the command line to obtain the working 
 * process/job ID is the same for running background and foreground jobs.
 *
 * The background flag is set in order to differentiate between
 * foreground and background jobs.
 * 
 * Both jobs require restarting the job by sending a SIGCONT signal, and 
 * updating the state of the job.
 * The background job requires printing the job details.
 * The foreground job requires the parent to wait for its children to 
 * terminate, if applicable.
 */
void builtin_back_fore(struct cmdline_tokens token, sigset_t prev,
                       int backg_flag) {
    jid_t jid;
    pid_t pid;
    const char *line;
    int scan;

    if (token.argv[1] != NULL) {

        // job identified by job ID
        if (token.argv[1][0] == '%') {
            scan = sscanf(token.argv[1], "%%%d", &jid);
            if (job_exists(jid)) {
                pid = job_get_pid(jid);

            // numerical job ID is illogical
            } else {
                sio_printf("%%%d: No such job\n", jid);
                sigprocmask(SIG_SETMASK, &prev, NULL);
                return;
            }

        // job identified by process ID
        } else {
            scan = sscanf(token.argv[1], "%d", &pid);
            jid = job_from_pid(pid);
        }

        // incorrect formatting results in sscanf() to return 0
        if (scan == 0) {
            if (backg_flag) {
                sio_printf("bg: argument must be a PID or %%jobid\n");
                sigprocmask(SIG_SETMASK, &prev, NULL);
                return;
            } else {
                sio_printf("fg: argument must be a PID or %%jobid\n");
                sigprocmask(SIG_SETMASK, &prev, NULL);
                return;
            }
        }
    // no arguments from command line (invalid)
    } else {
        if (backg_flag) {
            sio_printf("bg command requires PID or %%jobid argument\n");
            sigprocmask(SIG_SETMASK, &prev, NULL);
            return;
        } else {
            sio_printf("fg command requires PID or %%jobid argument\n");
            sigprocmask(SIG_SETMASK, &prev, NULL);
            return;
        }
    }
    // background job (built-in)
    if (backg_flag) {
        line = job_get_cmdline(jid);
        sio_printf("[%d] (%d) %s\n", jid, pid, line);

        // restart job by sending SIGCON signal
        kill(-pid, SIGCONT);
        jid = job_from_pid(pid);
        job_set_state(jid, BG);
        return;
    
    // foreground job 
    } else {
        // restart job by sending SIGCON signal
        kill(-pid, SIGCONT);
        jid = job_from_pid(pid);
        job_set_state(jid, FG);

        // wait for children to terminate
        while (fg_job() > 0) {
            sigsuspend(&prev);
        }
        return;
    }
}

/*****************
 * Signal handlers
 *****************/

/*
 * The SIGCHLD signal handler is responsible for handling SIGCHLD
 * signals. A SIGCHLD signal is set when a child process is stopped or
 * terminated. Both cases are handled with different job state updates,
 * and print statements. Signal blocking is implemented to avoid
 * corruption.
 */
void sigchld_handler(int sig) {

    // keep track of old errno value to update
    int old = errno;
    sigset_t mask, prev;

    // signal blocking masks
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTSTP);

    pid_t pid;
    jid_t jid;
    int status;

    // block signals
    sigprocmask(SIG_BLOCK, &mask, &prev);

    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
        jid = job_from_pid(pid);

        // job was eiher terminated by a signal, or exited normally
        if (WIFSIGNALED(status) || WIFEXITED(status)) {
            if (WIFSIGNALED(status)) {
                sio_printf("Job [%d] (%d) terminated by signal %d\n", jid,
                                                pid ,WTERMSIG(status));
            }
            // delete job from working list
            delete_job(job_from_pid(pid));
        }

        // job was stopped by a signal
        else if (WIFSTOPPED(status)) {
            job_state j = ST;
            job_set_state(jid, j);
            sio_printf("Job [%d] (%d) stopped by signal %d\n", jid, pid,
                                                WSTOPSIG(status));
        }
    }

    // unblock and update errno to old value (avoid corruption)
    sigprocmask(SIG_SETMASK, &prev, NULL);
    errno = old;
    return;
}

/*
 * The SIGINT signal handler is responsible for handling SIGINT signals.
 * A SIGINT signal is set when the user types in Ctrl-c to terminate
 * working jobs.
 * Signal blocking is implemented to avoid corruption.
 */
void sigint_handler(int sig) {

    // keep track of old errno value to update
    int old = errno;
    pid_t pid;
    jid_t jid;
    sigset_t mask, prev;

    // signal blocking masks
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTSTP);

    // block signals
    sigprocmask(SIG_BLOCK, &mask, &prev);

    // obtain designated job and terminate it
    jid = fg_job();
    if (jid > 0) {
        pid = job_get_pid(jid);
        if (pid) {
            kill(-pid, SIGINT);
        }
    }

    // unblock and update errno to old value (avoid corruption)
    sigprocmask(SIG_SETMASK, &prev, NULL);
    errno = old;
    return;
}

/*
 * The SIGTSTP signal handler is responsible for handling SIGTSTP
 * signals. A SIGTSTP signal is set when the user types in ctrl-z to
 * stop/suspend working jobs. Signal blocking is implemented to avoid
 * corruption.
 */
void sigtstp_handler(int sig) {

    // keep track of old errno value to update
    int old = errno;
    pid_t pid;
    jid_t jid;
    sigset_t mask, prev;

    // signal blocking masks
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTSTP);

    // block signals
    sigprocmask(SIG_BLOCK, &mask, &prev);

    // obtain designated job and stop/suspend it
    jid = fg_job();
    if (jid > 0) {
        pid = job_get_pid(jid);
        if (pid) {
            kill(-pid, SIGTSTP);
        }
    }

    // unblock and update errno to old value (avoid corruption)
    sigprocmask(SIG_SETMASK, &prev, NULL);
    errno = old;

    return;
}

/*
 * cleanup - Attempt to clean up global resources when the program
 * exits. In particular, the job list must be freed at this time, since
 * it may contain leftover buffers from existing or even deleted jobs.
 */
void cleanup(void) {
    // Signals handlers need to be removed before destroying the joblist
    Signal(SIGINT, SIG_DFL);  // Handles Ctrl-C
    Signal(SIGTSTP, SIG_DFL); // Handles Ctrl-Z
    Signal(SIGCHLD, SIG_DFL); // Handles terminated or stopped child

    destroy_job_list();
}
