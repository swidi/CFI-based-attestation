#include "mycode.h"
#define PRINTTOFILE 0

void print_cfi_kind(std::ostream& out, CFICheckFailData *Data) {
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

    out << "type " << Data->Type.getTypeName() << " failed during " << CheckKindStr << std::endl;

    SourceLocation Loc = Data->Loc.acquire();
    out << "Location: " << Loc.getFilename() << " " << Loc.getLine() << ":" << Loc.getColumn() << std::endl;
}

void print_trace(std::ostream& out) {
    void *array[10];
    char **strings;
    int size, i;

    size = backtrace(array, 10);
    strings = backtrace_symbols(array, size);
    if (strings != NULL)
    {

        out << "Obtained " << size << "stack frames." << std::endl;
        for (i = 0; i < size; i++)
            out << strings[i] << std::endl;
    }

    free(strings);
}

void mycode::attest(CFICheckFailData *Data){
    std::ostream& out;
#ifdef PRINTTOFILE
    std::ofstream outfile;
    outfile.open("report", std::ios::app);
    out = outfile;
#else
    out = std::cout
    out << "\033[1;33m~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\033[0m" << std:endl;
#endif

    out << "CFI violation" << std::endl;

    out << "libc stack trace:" << std::endl;
    print_trace(fp);
    out << "----------------------------------------" << std::endl;


#ifndef PRINTTOFILE
    out << "\033[1;33m~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\033[0m" << std::endl;
#endif
}
