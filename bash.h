/*
 * Ostermann's shell header file
 */


#define MAX_ARGS 500

#include <stdbool.h>



/* This is the definition of a command */
struct command {
    char *command;
    int argc;
    char *argv[MAX_ARGS];
    char *infile;
    char *outfile;
    char *errfile;

    char output_append;		/* boolean: append stdout? */
    char error_append;		/* boolean: append stderr? */

    struct command *next;
};


/* externals */
extern int yydebug;
extern int debug;
extern int lines;  // defined and updated by parser, used by bash.cc


/* you should use THIS routine instead of malloc */
void *MallocZ(int numbytes);

/* global routine decls */
void doline(struct command *pass);
int yyparse(void);

// student made functions
void debug_cmd(struct command *pcmd);
void echo(struct command *pcmd);
void not_a_builtin(struct command *pcmd, char *file_path);
void run_command(struct command *pcmd);
void set_environment_variable(char *text);
char* remove_whitespace(char *text);
char* remove_quotes(char *text);
bool check_if_path_exists(char *path);
char* and_evaluation(char* text);
void ps1();
void cd(struct command *pcmd);
char* whence(struct command *pcmd, char *path);

