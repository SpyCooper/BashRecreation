/*
 * Headstart for shell project made by Dr.Shawn Ostermann
 * Logic created by Josh Myers
 */
 
#include <string.h>
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <string>
#include <fcntl.h>

using namespace std;

// types and definitions live in "C land" and must be flagged
extern "C" {
#include "parser.tab.h"
#include "bash.h"
extern "C" void yyset_debug(int d);
}

// function prototypes (they had to be in here because they didn't work in bash.h)
string process_argument(string arg);
string process_variable_expansion(string arg);

// global debugging flag
int debug = 0;

int main(int argc, char *argv[])
{
    // set up the debugging flag
    if (argc > 1 && strcmp(argv[1], "-d") == 0)
        debug = 1;

    if (debug)
        yydebug = 1;  /* turn on ugly YACC debugging */

    /* parse the input file */
    yyparse();

    exit(0);
}


// runs for each command
void doline(struct command *pcmd)
{
    // check if there is a command
    if(pcmd == NULL)
        return;
    
    // if debug is turned on, print the debugging for the code (this was the old doline for shell1)
    if (debug)
        debug_cmd(pcmd);
    
    // set up the commands for checking
    char *command = pcmd->command;
    char *file_path = nullptr;

    // check if the command has a location
    char *last_slash = strrchr(command, '/');
    if (last_slash != NULL)
    {
        // split the command for the file path
        file_path = strndup(command, last_slash - command);
        command = last_slash + 1;
    }

    // check if the command has a environment variable in the arguments
    for (int i = 0; i < pcmd->argc; i++)
    {
        // process the argument
        string arg = pcmd->argv[i];
        arg = process_argument(arg);
        pcmd->argv[i] = strdup(arg.c_str());
    }

    // print the ps1
    ps1();

    // expand the environment variables in the file redirection
    if (pcmd->infile != NULL)
    {
        pcmd->infile = strdup(process_argument(pcmd->infile).c_str());
    }
    if (pcmd->outfile != NULL)
    {
        pcmd->outfile = strdup(process_argument(pcmd->outfile).c_str());
    }   
    if (pcmd->errfile != NULL)
    {
        pcmd->errfile = strdup(process_argument(pcmd->errfile).c_str());
    }

    // save the standard in, out, and error
    int saved_stdin = dup(STDIN_FILENO);
    int saved_stdout = dup(STDOUT_FILENO);
    int saved_stderr = dup(STDERR_FILENO);

    // standard in, out, and error redirection
    if (pcmd->infile != NULL)
    {
        // check if the file path exists
        if (!check_if_path_exists(pcmd->infile))
        {
            // if the path does not exist, return
            return;
        }

        // open the file
        int fd = open(pcmd->infile, O_RDONLY);
        if (fd == -1)
        {
            perror(pcmd->infile);
            return;
        }

        // redirect the standard in
        dup2(fd, STDIN_FILENO);
        close(fd);
    }
    // check if the outfile
    if (pcmd->outfile != NULL)
    {
        // set the flags
        int flags = O_WRONLY | O_CREAT | (pcmd->output_append ? O_APPEND : O_TRUNC);
        // open the file
        int fd = open(pcmd->outfile, flags, 0644);
        // check if the file was opened
        if (fd == -1)
        {
            perror(pcmd->outfile);
            return;
        }
        // redirect the standard out
        dup2(fd, STDOUT_FILENO);
        // close the file
        close(fd);
    }
    // check if the errfile
    if (pcmd->errfile != NULL)
    {
        // set the flags
        int flags = O_WRONLY | O_CREAT | (pcmd->error_append ? O_APPEND : O_TRUNC);
        // open the file
        int fd = open(pcmd->errfile, flags, 0644);
        // check if the file was opened
        if (fd == -1)
        {
            perror(pcmd->errfile);
            return;
        }
        // redirect the standard error
        dup2(fd, STDERR_FILENO);
        // close the file
        close(fd);
    }

    // check if the command is an echo
    if (strcmp(command, "echo") == 0)
    {
        // if the pcmd->command is /bin/echo, then run execv
        if (strcmp(pcmd->command, "/bin/echo") == 0)
        {
            // child process
            if (fork() == 0)
            {
                // run the command through execv
                execv(pcmd->command, pcmd->argv);
                perror(pcmd->command);
                exit(EXIT_FAILURE);
            }
            // parent process
            else
            {
                int status;
                waitpid(-1, &status, 0);
            }
        }
        // if the pcmd->command is not /bin/echo, then run the echo function
        else
        {
            echo(pcmd);
        }
    }
    // check if the command is a cd
    else if (strcmp(command, "cd") == 0)
    {
        // run the cd function
        cd(pcmd);
    }
    // if the command is not echo or cd, run the command through execv
    else
    {
        not_a_builtin(pcmd, file_path);
    }

    // flush the standard in, out, and error
    fflush(stdout);
    fflush(stderr);

    // reset the standard in, out, and error
    dup2(saved_stdin, STDIN_FILENO);
    dup2(saved_stdout, STDOUT_FILENO);
    dup2(saved_stderr, STDERR_FILENO);
}

// echo the commands (running echo not through execv())
void echo(struct command *pcmd)
{
    // print the arguments
    for (int i = 1; i < pcmd->argc; i++)
    {
        // print the argument
        string arg = pcmd->argv[i];
        cout << arg;

        // check if there is another argument
        if (i != pcmd->argc - 1)
        {
            cout << " ";
        }
    }

    // print a newline
    cout << endl;
}

// change the directory (not through cd)
void cd(struct command *pcmd)
{
    // check if there are too many arguments
    if (pcmd->argc > 2)
    {
        cerr << "'cd' requires exactly one argument" << endl;
    }
    // check if there is an argument
    else if (pcmd->argc > 1)
    {
        // change the directory to the argument
        int temp = chdir(pcmd->argv[1]);

        // check if the directory was changed
        if (temp == -1)
        {
            // print an error message
            perror(pcmd->argv[1]);
        }
    }
    // if there is no argument
    else
    {
        // change the directory to the home directory
        char* temp = getenv("HOME");

        // check if the home directory is set
        if (temp == NULL)
        {
            // print an error message
            perror("cd");
        }

        // change the directory
        int temp2 = chdir(getenv("HOME"));

        // check if the directory was changed
        if (temp2 == -1)
        {
            // print an error message
            perror("cd");
        }
    }
}

// prints out the PS1
void ps1()
{
    // get the ps1 environment variable and output
    char *ps1 = getenv("PS1");
    // check if the ps1 is not null
    if (ps1 != NULL)
    {
        // print the ps1
        fprintf(stdout, "%s", ps1);
        fflush(stdout);
    }
}

// process each of the arugments
string process_argument(string arg)
{
    //check to see if the argument has a variable
    if (arg.find('$') == string::npos)
    {
        return arg;
    }

    // if the argument is in single quote
    if (arg[0] == '\'' && arg[arg.size() - 1] == '\'')
    {
        // remove the quotes
        arg.erase(0, 1);
        if (arg[arg.size() - 1] == '\'')
        {
            arg.erase(arg.size() - 1, 1);
        }

        // return the argument
        return arg;
    }

    // if the argument is in double quote or not in quotes
    return process_variable_expansion(arg);
}

// process variable expansion
string process_variable_expansion(string arg)
{
    // go through each of the characters in the argument
    for (size_t j = 0; j < arg.size(); j++)
    {
        // check if the character is a dollar sign
        if (arg[j] == '$')
        {
            // check if the next character is a curly brace
            size_t end = j + 1;
            bool brace = false;
            if (arg[end] == '{')
            {
                brace = true;
                end++;
            }

            // find the end of the variable name
            while (end < arg.size() && (isalnum(arg[end]) || arg[end] == '_'))
                end++;

            // check if the variable name is in a curly brace
            if (brace && end < arg.size() && arg[end] == '}')
                end++;

            // get the enviornment variable name
            string var_name = arg.substr(j + (brace ? 2 : 1), end - j - (brace ? 3 : 1));
            // get the enviornment variable value
            const char* var_value = getenv(var_name.c_str());
            // check if the enviornment variable exists
            if (var_value)
            {
                // replace the variable with the value
                arg.replace(j, end - j, var_value);
                j += strlen(var_value) - 1;
            }
            // if the enviornment variable does not exist
            else
            {
                // remove the variable
                arg.replace(j, end - j, "");
                j--;
            }
        }
    }

    // Remove quotes from the beginning and end of the argument
    if (arg[0] == '"')
    {
        arg.erase(0, 1);
        if (arg[arg.size() - 1] == '"')
        {
            arg.erase(arg.size() - 1, 1);
        }
    }

    // return the argument
    return arg;
}

// run a command that is not a builtin
void not_a_builtin(struct command *pcmd, char *file_path)
{
    // fork the process
    pid_t pid = fork();

    // check if the fork failed
    if (pid == -1)
    {
        // fork failed
        perror("fork");
        return;
    }
    // child process
    else if (pid == 0)
    {
        // check if the file path is null
        if (file_path == nullptr)
        {
            // get the path
            char *path = getenv("PATH");
            char *path_copy = strdup(path);
            // if the path enviornment variable exists
            if (path_copy != NULL)
            {
                // get the path of the command
                path_copy = whence(pcmd, path_copy);
                // check if the path exists
                if (path_copy != NULL)
                {
                    // run the command through execv
                    execv(path_copy, pcmd->argv);
                    perror(pcmd->command);
                }
                // if the command cannot be found
                else
                {
                    // print an error message
                    cerr << pcmd->command << ": command not found" << endl;
                    exit(EXIT_FAILURE);
                }
            }
        }
        // if the file path is not null
        else
        {
            execv(pcmd->command, pcmd->argv);
            perror(pcmd->command);
            exit(EXIT_FAILURE);
        }
    }
    // parent process
    else
    {
        // wait for the child to finish
        int status;
        waitpid(pid, &status, 0);
    }
}

// find the path of the command
char* whence(struct command *pcmd, char *path)
{
    // create a buffer for the path
    char pathbuf[500];
    // get the program name
    char *progname = pcmd->command;

    // get the first token
    char *token = strtok(path, ":");
    // while there are more tokens
    while (token != NULL)
    {
        // create the path
        snprintf(pathbuf, sizeof(pathbuf), "%s/%s", token, progname);
        // check if the path exists
        if (access(pathbuf, X_OK) == 0)
        {
            // return the path
            return strdup(pathbuf);
        }
        // get the next token
        token = strtok(NULL, ":");
    }

    // return null if the path does not exist
    return NULL;
}

// set the environment variable
void set_environment_variable(char *text)
{
    // print ps1
    ps1();

    // check if the characters are too long
    if (strlen(text) > 500)
    {
        cerr << "Error on line " << lines + 1 << ": Path directory list too long" << endl;
        return;
    }

    // remove the white space in the text
    text = remove_whitespace(text);

    // find the equals sign
    char *equals = strchr(text, '=');

    // check if the text is in the correct format
    if (equals == NULL)
    {
        cerr << "Invalid environment variable format" << endl;
        return;
    }

    // split the text into variable name and value
    *equals = '\0';
    char *env_var = text;
    char *env_val = equals + 1;

    // remove the quotes from the value
    env_val = remove_quotes(env_val);
    
    // check if there is an environment variable in the value
    if (strchr(env_val, '$') != NULL)
    {
        string env_val_str = process_argument(string(env_val));
        env_val = strdup(env_val_str.c_str());
    }

    // create the environment string in the format "VAR=VALUE"
    size_t env_str_len = strlen(env_var) + strlen(env_val) + 2;
    char *env_str = (char *)malloc(env_str_len);
    // check if the environment string was created
    if (env_str == NULL)
    {
        perror("malloc");
        return;
    }
    // recombine the variable and value
    snprintf(env_str, env_str_len, "%s=%s", env_var, env_val);

    // check if the environment variable is DEBUG
    if (strcmp(env_var, "DEBUG") == 0)
    {
        // if the environment variable value is not empty, turn on debug
        if (strlen(env_val) > 0)
        {
            debug = 1;
        }
        // if the environment variable value is empty, turn off debug
        else
        {
            debug = 0;
        }
    }
    // set the environment variable using putenv
    if (putenv(env_str) != 0)
    {
        perror("putenv");
        free(env_str);
    }
}

// remove the white space from the text
char* remove_whitespace(char *text)
{
    // create a buffer for the text
    char *result = text;
    char *dst = text;
    bool in_single_quote = false;
    bool in_double_quote = false;

    // remove the white space
    while (*text)
    {
        // check if the text is in a single or double quote
        if (*text == '\'' && !in_double_quote)
        {
            in_single_quote = !in_single_quote;
        }
        else if (*text == '"' && !in_single_quote)
        {
            in_double_quote = !in_double_quote;
        }

        // check if the text is white space and not in a quote
        if (!in_single_quote && !in_double_quote && isspace(*text))
        {
            text++;
            continue;
        }

        // copy the text to the buffer
        *dst++ = *text++;
    }

    // add the null terminator
    *dst = '\0';
    // return the buffer
    return result;
}

// remove the quotes from the text
char* remove_quotes(char *text)
{
    // check if the text is ""
    if (text[0] == '"' && text[strlen(text) - 1] == '"')
    {
        // remove the first quote
        for (size_t i = 0; i < strlen(text); i++)
        {
            text[i] = text[i + 1];
        }
        // remove the last quote
        text[strlen(text) - 1] = '\0';
        // return the text
        return text;
    }

    // remove the first quote
    for (size_t i = 0; i < strlen(text); i++)
    {
        if (text[i] == '"')
        {
            for (size_t j = i; j < strlen(text); j++)
            {
                text[j] = text[j + 1];
            }
        }
        else if (text[i] == '\'')
        {
            for (size_t j = i; j < strlen(text); j++)
            {
                text[j] = text[j + 1];
            }
        }
    }
    // remove the last quote
    for (size_t i = strlen(text); i > 0; i--)
    {
        if (text[i] == '"')
        {
            for (size_t j = i; j < strlen(text); j++)
            {
                text[j] = text[j + 1];
            }
        }
        else if (text[i] == '\'')
        {
            for (size_t j = i; j < strlen(text); j++)
            {
                text[j] = text[j + 1];
            }
        }
    }

    // return the text
    return text;
}

// check if the path exists
bool check_if_path_exists(char *path)
{
    // check if the path exists
    if (access(path, F_OK) == -1)
    {
        // print an error message
        perror(path);
        return false;
    }
    return true;
}

// process the variable expansion
char* process_variable_expansion(char* text)
{
    char *temp = text;
    // remove the first character
    for (size_t i = 0; i < strlen(temp); i++)
    {
        temp[i] = temp[i + 1];
    }
    // check if the variable has a curly brace
    if (temp[0] == '{')
    {
        // remove the first character
        for (size_t i = 0; i < strlen(temp); i++)
        {
            temp[i] = temp[i + 1];
        }
        // remove the last character
        temp[strlen(temp) - 1] = '\0';
    }
    
    // find the environment variable
    char *env_var = getenv(temp);

    // check if the environment variable exists
    if (env_var == NULL)
    {
        // print an error message
        cerr << "Environment variable " << temp << " not found" << endl;
        return temp;
    }
    // return the environment variable
    else
    {
        return env_var;
    }
    // return the text
    return text;
}

// old doline code
void debug_cmd(struct command *pcmd)
{
    if (pcmd == NULL)
        return;
    
    // set up a counter for the number of pipes
    int pipes = 0;
    // print the command line header
    cout << "========== line " << lines << " ==========" << endl;
    // while there are more commands to print
    while(pcmd != NULL)
    {
        // print the command name
        cout << "Command name: '" << pcmd->command << "'" << endl;
        // print each of the arguments
        for (int i = 0; i < pcmd->argc; i++)
            cout << "    argv[" << i << "]: '" << pcmd->argv[i] <<"'" << endl;
        // print the input
        cout << "  stdin:  ";
        // if there is no input
        if (pcmd->infile == NULL)
        {
            cout << "UNDIRECTED" << endl;
        }
        // if there is an input
        else
        {
            // check if the input is a pipe
            if (pcmd->infile[0] == '|')
            {
                // print the pipe with the pipe number
                cout << "PIPE" << pipes << endl;
            }
            // if the input is not a pipe
            else
            {
                // print the input file
                cout <<  "'" << pcmd->infile <<  "'" << endl;
            }
        }

        // print the output
        cout << "  stdout: ";
        // if there is no output
        if (pcmd->outfile == NULL)
        {
            cout << "UNDIRECTED" << endl;
        }
        // if there is an output
        else
        {
            // check if the output is a pipe
            if (pcmd->outfile[0] == '|')
            {
                // increment the number of pipes
                pipes++;
                // print the pipe with the pipe number
                cout << "PIPE" << pipes << endl;
            }
            // if the output is not a pipe
            else
            {
                // print the output file
                cout << "'" << pcmd->outfile <<  "'";
                // check if the output is an append
                if (pcmd->output_append != false)
                {
                    cout << " (append)";
                }
                cout << endl;
            }
        }

        // print the error
        cout << "  stderr: ";
        // if there is no error file
        if (pcmd->errfile == NULL)
        {
            cout << "UNDIRECTED" << endl;
        }
        // if there is an error file
        else
        {
            // print the error file
            cout <<  "'" << pcmd->errfile <<  "'";
            // check if the error file is an append
            if (pcmd->error_append != false)
            {
                cout << " (append)";
            }
            cout << endl;
        }

        // go to the next command
        pcmd = pcmd->next;
    }
    // print a newline
    cout << endl;
}