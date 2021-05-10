#include "mycode.h"
#define PRINTTOFILE 0
#define LOG printf

pid_t perf_pid;

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
    pid_t fork_id = fork();
    LOG("Dumping core using gcore\n");

    if (fork_id == -1) {
        LOG("Error during core dump: cannot fork\n");
        return;
    } if (fork_id == 0) {
        LOG("Gcore fork PID: %d\n", getpid());

        char* gcore_string;
        asprintf(&gcore_string, "/usr/bin/gcore %d", getpid());

        int ret = system(gcore_string);
        if(ret != 0)
            LOG("Error during core dump: gcore exit status %d\n", ret);

        exit(0);
    }
}

void perf() {
    LOG("Signalling perf (PID %d) to create snapshot\n", perf_pid);

    kill(perf_pid, SIGUSR2);
}

void mycode::attest(CFICheckFailData *Data){
    LOG("\033[1;33m~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\033[0m\n");

    LOG("CFI violation\n");
    perf();
    print_trace();
    dump_core();

    LOG("\033[1;33m~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\033[0m\n");
}

void __attribute__ ((constructor)) premain()
{
    printf("In premain()\n");
    LOG("Original pid: %d\n", getpid());
    char* pid_string;
    asprintf(&pid_string, "%d", getpid());

    perf_pid = fork();

    if (perf_pid == -1) {
        LOG("Error during perf record: cannot fork\n");
        return;
    } if (perf_pid == 0) {
        LOG("Executing %s from child process %d\n", "perf", getpid());


        int ret = execl("/usr/bin/perf", "perf", "record", "-v", "-e", "intel_pt//u", "-S", "--switch-output=signal", "-p",pid_string, (char *) NULL);
        if(ret != 0)
            LOG("Error during perf record: %d\n", ret);

        exit(0);
    }
    else {
        sleep(5);
    }
}

