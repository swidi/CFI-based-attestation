#include "mycode.h"
#define PRINTTOFILE 0
#define LOG printf
#define FORK_GCORE 1

pid_t perf_pid;
int wd, inotify_fd;
struct pollfd poll_fd[1];

void print_cfi_kind(CFICheckFailData *Data) {
    const char *CheckKindStr; 

    switch (Data->CheckKind) {
        case CFITCK_VCall:
            CheckKindStr = "virtual call";
            break;
        case CFITCK_NVCall:
            CheckKindStr = "non-virtual call";
            break;
        case CFITCK_DerivedCast:
            CheckKindStr = "base-to-derived cast";
            break;
        case CFITCK_UnrelatedCast:
            CheckKindStr = "cast to unrelated type";
            break;
        case CFITCK_VMFCall:
            CheckKindStr = "virtual pointer to member function call";
            break;
        case CFITCK_ICall:
            CheckKindStr = "indirect function call";
            break;
        case CFITCK_NVMFCall:
            CheckKindStr = "non-virtual pointer to member function call";
            break;
    }

    LOG("type %s failed during %s\n", Data->Type.getTypeName(), CheckKindStr);

    SourceLocation Loc = Data->Loc.acquire();
    LOG("Location: %s %d:%d\n", Loc.getFilename(), Loc.getLine(), Loc.getColumn());
}

void print_trace() {
    void *array[10];
    char **strings;
    int size, i;

    size = backtrace(array, 10);
    strings = backtrace_symbols(array, size);
    LOG("libc stack trace:\n");
    if (strings != NULL)
    {

        LOG("Obtained %d stack frames\n", size);
        for (i = 0; i < size; i++)
            LOG(strings[i]);
        LOG("\n");
    }

    free(strings);
}

void dump_core() {
#if FORK_GCORE
    pid_t fork_id = fork();
#endif

    LOG("Dumping core using gcore\n");

#if FORK_GCORE
    if (fork_id == -1) {
        LOG("Error during core dump: cannot fork\n");
        return;
    } if (fork_id == 0) {
        LOG("Gcore fork PID: %d\n", getpid());
#endif

        char* gcore_string;
        asprintf(&gcore_string, "/usr/bin/gcore %d", getpid());

        int ret = system(gcore_string);
        if(ret != 0)
            LOG("Error during core dump: gcore exit status %d\n", ret);

#if FORK_GCORE
        exit(0);
    }
#endif
}

static int handle_events(int fd, int wd) {
    /* Some systems cannot read integer variables if they are not
       properly aligned. On other systems, incorrect alignment may
       decrease performance. Hence, the buffer used for reading from
       the inotify file descriptor should have the same alignment as
       struct inotify_event. */

    char buf[4096]
        __attribute__ ((aligned(__alignof__(struct inotify_event))));
    const struct inotify_event *event;
    ssize_t len;

    /* Loop while events can be read from inotify file descriptor. */

    for (;;) {

        /* Read some events. */

        len = read(fd, buf, sizeof(buf));
        if (len == -1 && errno != EAGAIN) {
            perror("read");
            exit(EXIT_FAILURE);
        }

        /* If the nonblocking read() found no events to read, then
           it returns -1 with errno set to EAGAIN. In that case,
           we exit the loop. */

        if (len <= 0)
            break;

        /* Loop over all events in the buffer. */
        int ret = 0;
        for (char *ptr = buf; ptr < buf + len;
                ptr += sizeof(struct inotify_event) + event->len) {

            event = (const struct inotify_event *) ptr;

            /* Print event type. */

            if (event->mask & IN_OPEN)
                printf("IN_OPEN: ");
            if (event->mask & IN_CLOSE_NOWRITE)
                printf("IN_CLOSE_NOWRITE: ");
            if (event->mask & IN_CLOSE_WRITE) {
                printf("IN_CLOSE_WRITE: ");
            }
            if (event->mask & IN_MOVE_SELF)
                printf("IN_MOVE_SELF: ");

            /* Print the name of the file. */

            if (event->len)
                printf("%s\n", event->name);

            if(strstr(event->name, "perf.data") != NULL && event->mask & IN_OPEN) {
                printf("perf.data has been opened ~~~~~~~~~~~~~~~~~~\n");
                ret = 1;
            }
        }
        return ret;
    }
}

void wait_for_inotify_event() {
    int poll_num;
    printf("Listening for events.\n");
    while (1) {
        poll_num = poll(poll_fd, 1, -1);
        if (poll_num == -1) {
            if (errno == EINTR)
                continue;
            perror("poll");
            exit(EXIT_FAILURE);
        }

        if (poll_num > 0) {
            if (poll_fd[0].revents & POLLIN) {
                // Inotify events are available.
                if(handle_events(inotify_fd, wd))
                    break;
            }
        }
    }

    printf("Listening for events stopped.\n");

    /* Close inotify file descriptor. */
    // TODO: implement "destructor" that closes inotify
    //        close(inotify_fd);
}

void perf() {
    LOG("Signalling perf (PID %d) to create snapshot\n", perf_pid);

    kill(perf_pid, SIGUSR2);
    wait_for_inotify_event();
}

void mycode::attest(CFICheckFailData *Data){
    LOG("\033[1;33m~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\033[0m\n");

    LOG("CFI violation\n");
    perf();
    print_trace();
    dump_core();

    LOG("\033[1;33m~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\033[0m\n");
}


void init_inotify() {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("Current working dir: %s\n", cwd);
    } else {
        perror("getcwd() error");
        exit(EXIT_FAILURE);
    }

    /* Create the file descriptor for accessing the inotify API. */
    inotify_fd = inotify_init1(IN_NONBLOCK);
    if (inotify_fd == -1) {
        perror("inotify_init1");
        exit(EXIT_FAILURE);
    }

    /* Mark directories for events
       - file was opened
       - file was closed
       - file was moved*/
    wd = inotify_add_watch(inotify_fd, cwd,
            IN_OPEN | IN_CLOSE | IN_MOVE_SELF);
    if (wd == -1) {
        fprintf(stderr, "Cannot watch '%s': %s\n", cwd, strerror(errno));
        exit(EXIT_FAILURE);
    }

    /* Prepare for polling. */
    poll_fd[0].fd = inotify_fd;
    poll_fd[0].events = POLLIN;
}

void __attribute__ ((constructor)) premain()
{
    printf("In premain()\n");

    LOG("Original pid: %d\n", getpid());
    char* pid_string;
    asprintf(&pid_string, "%d", getpid());

    init_inotify();

    perf_pid = fork();

    if (perf_pid == -1) {
        LOG("Error during perf record: cannot fork\n");
        exit(EXIT_FAILURE);
    } if (perf_pid == 0) {
        LOG("Executing %s from child process %d\n", "perf", getpid());

        int ret = execl("/usr/bin/perf", "perf", "record", "-v", "-e", "intel_pt//u", "-S", "--switch-output=signal", "-p", pid_string, (char *) NULL);
        if(ret != 0)
            LOG("Error during perf record: %d\n", ret);

        exit(0);
    }
    else {
        wait_for_inotify_event();
        sleep(1);
    }
    // ... continue to original code
}

