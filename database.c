

#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>
#include <llvm-c/Transforms/IPO.h>
#include <llvm-c/Transforms/Scalar.h>
#include <llvm-c/Transforms/Vectorize.h>

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <ctype.h>

#include "table.h"
#include "parser.h"

#include "target_machine.h"

static void Initialize(void);
static char* ReadQuery(void);
static Table *ExecuteQuery(Query *query);
static void Cleanup(void); 
static LLVMPassManagerRef InitializePassManager(LLVMModuleRef module);

static bool enable_optimizations = false;
static bool print_result = true;
static bool print_llvm = true;
static bool execute_statement = false;
static char* statement;

static Table*
ExecuteQuery(Query *query) {
    // TODO: Implement the execution engine of the database

    // This function takes a Query object as input, and should return a table
    // The query object has the following structure:
    /*
     *  typedef struct {
     *      Operation *select;
     *      char *table;
     *      Operation *where;
     *      ColumnList *columns;
     *  } Query;
     */
    // "select" is the operation that describe the SELECT part of the query
    //   for simplicity, we only allow one statement in the SELECT clause
    // "table" is just the table name in the FROM clause
    // "where" is the operation that describes the WHERE condition
    // "columns" is a list of all columns that are part of the query

    // Both "Select" and "Where" are an Operation structure,
    // this structure contains the Operation that has to be performed
    // An operation is one of the following types:
    // 
    // 1) ConstantOperation (op->type == OPTYPE_const)
    //    Holds a constant double value: ((ConstantOperation*)op)->value
    // 2) ColumnOperation (op->type == OPTYPE_colmn)
    //    Holds a reference to a column that occurs in the table
    //      ((ColumnOperation*)op)->column
    //    - The column stores an array of doubles in column->data
    //    - The amount of values in the column is available in column->size
    // 3) BinaryOperation (op->type == OPTYPE_binop)
    //    Holds an actual binary operation
    //    - The type of the operation is stored in ((BinaryOperation*)op)->optype
    //       a list of supported types can be found at the top of parser.h
    //       note that there are no "difficult" operations (i.e. no pipeline breakers)
    //    - The LHS of the operation ((BinaryOperation*)op)->left
    //    - The RHS of the operation ((BinaryOperation*)op)->right
    //    Binary operations can be nested to create complex expressions
    //      i.e. ->left can be a BinaryOperation as well

    // To correctly answer queries, you should handle both query->select and 
    //   query->where using JIT compilation

    // Hint: Write a function to handle to handle a specific query in C,
    //  I recommend starting with only selections
    //  such as 'SELECT x+y+z FROM demo;'
    // then use the command "clang file.c -S -emit-llvm -o -"
    // to compile the C function to LLVM IR to get an idea of how to do it in LLVM

    // note that your function will have to accept an arbitrary amount of input columns
    // (how would you do that in C?)

    // For a simple for-loop implementation using LLVM JIT Compilation, see llvmtest.c
    // You can copy most of the code from there as a start to your execution engine

    // For now, we implement a basic engine that only implements selection

     (void) InitializePassManager;
     (void) LLVMOptimizeModuleForTarget;

    if (query->where != NULL) {
        fprintf(stdout, "Error: WHERE operation is currently not supported.\n");
        return NULL;
    }

    Operation* select = query->select;
    Column *column = NULL;
    if (select->type != OPTYPE_colmn) {
        fprintf(stdout, "Error: Currently only supports column selections.\n");
        return NULL;
    } else {
        ColumnOperation *colop = (ColumnOperation*) select;
        column = CreateColumn(colop->column->data, colop->column->size);
    }
    return CreateTable("Result", column);
}

int main(int argc, char** argv) {
    for(int i = 1; i < argc; i++) {
        char *arg = argv[i];
        if (strcmp(arg, "--help") == 0) {
            fprintf(stdout, "RembranDB Options.\n");
            fprintf(stdout, "  -opt              Enable  LLVM optimizations.\n");
            fprintf(stdout, "  -no-print         Do not print query results.\n");
            fprintf(stdout, "  -no-llvm          Do not print LLVM instructions.\n");
            fprintf(stdout, "  -s \"stmnt\"        Execute \"stmnt\" and exit.\n");
            return 0;
        } else if (strcmp(arg, "-opt") == 0) {
            fprintf(stdout, "Optimizations enabled.\n");
            enable_optimizations = true;
        } else if (strcmp(arg, "-no-print") == 0) {
            fprintf(stdout, "Printing output disabled.\n");
            print_result = false;
        } else if (strcmp(arg, "-no-llvm") == 0) {
            fprintf(stdout, "Printing LLVM disabled.\n");
            print_llvm = false;
        } else if (strcmp(arg, "-s") == 0) {
            execute_statement = true;
        } else if (execute_statement) {
            statement = arg;
        } else {
            fprintf(stdout, "Unrecognized command line option \"%s\".\n", arg);
            exit(1);
        }
    }
    if (!execute_statement) {
        fprintf(stdout, "# RembranDB server v0.0.0.1\n");
        fprintf(stdout, "# Serving table \"demo\", with no support for multithreading\n");
        fprintf(stdout, "# Did not find any available memory (didn't look for any either)\n");
        fprintf(stdout, "# Not listening to any connection requests.\n");
        fprintf(stdout, "# RembranDB/SQL module loaded\n");
    }
    Initialize();

    while(true) {
        char *query_string;
        if (!execute_statement) {
            query_string = ReadQuery();
        } else {
            query_string = statement;
        }

        if (strcmp(query_string, "\\q") == 0 || (strlen(query_string) > 0 && query_string[0] == '^')) break;
        if (strcmp(query_string, "\\d") == 0) {
            PrintTables();
            continue;
        }
        Query *query = ParseQuery(query_string);
        
        if (query) {
            clock_t tic = clock();
            Table *tbl = ExecuteQuery(query);
            clock_t toc = clock();

            fprintf(stdout, "Total Runtime: %f seconds\n", (double)(toc - tic) / CLOCKS_PER_SEC);

            if (print_result) {
                PrintTable(tbl);
            }
        }
        if (execute_statement) break;
    }

    Cleanup();
}

static LLVMPassManagerRef InitializePassManager(LLVMModuleRef module) {
    LLVMPassManagerRef passManager = LLVMCreateFunctionPassManagerForModule(module);
    // This set of passes was copied from the Julia people (who probably know what they're doing)
    // Julia Passes: https://github.com/JuliaLang/julia/blob/master/src/jitlayers.cpp

    LLVMAddTargetMachinePasses(passManager);
    LLVMAddCFGSimplificationPass(passManager);
    LLVMAddPromoteMemoryToRegisterPass(passManager);
    LLVMAddInstructionCombiningPass(passManager);
    LLVMAddScalarReplAggregatesPass(passManager);
    LLVMAddScalarReplAggregatesPassSSA(passManager);
    LLVMAddInstructionCombiningPass(passManager);
    LLVMAddJumpThreadingPass(passManager);
    LLVMAddInstructionCombiningPass(passManager);
    LLVMAddReassociatePass(passManager);
    LLVMAddEarlyCSEPass(passManager);
    LLVMAddLoopIdiomPass(passManager);
    LLVMAddLoopRotatePass(passManager);
    LLVMAddLICMPass(passManager);
    LLVMAddLoopUnswitchPass(passManager);
    LLVMAddInstructionCombiningPass(passManager);
    LLVMAddIndVarSimplifyPass(passManager);
    LLVMAddLoopDeletionPass(passManager);
    LLVMAddLoopUnrollPass(passManager);
    LLVMAddLoopVectorizePass(passManager);
    LLVMAddInstructionCombiningPass(passManager);
    LLVMAddGVNPass(passManager);
    LLVMAddMemCpyOptPass(passManager);
    LLVMAddSCCPPass(passManager);
    LLVMAddInstructionCombiningPass(passManager);
    LLVMAddSLPVectorizePass(passManager);
    LLVMAddAggressiveDCEPass(passManager);
    LLVMAddInstructionCombiningPass(passManager);

    LLVMInitializeFunctionPassManager(passManager);
    return passManager;
}

static void Initialize(void) {
    // LLVM boilerplate initialization code
    LLVMLinkInMCJIT();
    LLVMInitializeNativeTarget();
    LLVMInitializeAllTargetMCs();
    LLVMInitializeAllAsmPrinters();
    LLVMInitializeAllAsmParsers();
    // Load data, demo table = small table (20 entries per column)
    InitializeTable("demo");
}

static char *
ReadQuery(void) {
    char *buffer = malloc(5000 * sizeof(char));
    size_t buffer_pos = 0;
    char c;
    printf("> ");
    while((c = getchar()) != EOF) {
        if (c == '\n') {
            if (buffer[0] == '\\') {
                return buffer;
            } else {
                buffer[buffer_pos++] = ' ';
                printf("> ");
                continue;
            }
        } else if (c == ';') {
            buffer[buffer_pos++] = '\0';
            return buffer;
        } else {
            buffer[buffer_pos++] = c;
        }
    }
    return strdup("\\q");
}

static void 
Cleanup(void) {

}
