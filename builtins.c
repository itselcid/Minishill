#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct s_data
{
    char **cmd; // cmd[0] = command and cmd[>0] == command options
    t_file *file;
    struct s_data *next;
} t_data;

typedef struct s_file
{
    char *file;
    int infile;
    int outfile;
    int herdoc;
    int apend;
    struct s_file *next;
} t_file;


// Function prototypes for builtin commands
int builtin_echo(t_data *cmd, char **env);
int builtin_cd(t_data *cmd, char **env);
int builtin_pwd(t_data *cmd, char **env);
int builtin_export(t_data *cmd, char **env);
int builtin_unset(t_data *cmd, char **env);
int builtin_env(t_data *cmd, char **env);
int builtin_exit(t_data *cmd, char **env);

// Helper function for variable expansion
char *expand_variables(char *arg, char **env)
{
    // Implementation of expand_variables function
    // (You need to implement this function)
    return strdup(arg);  // Placeholder implementation
}

// Implementation of builtin commands
int builtin_echo(t_data *cmd, char **env)
{
    int i = 1;  // Start from the first argument after "echo"
    int newline = 1;  // By default, print newline at the end

    // Check for -n option
    if (cmd->args[i] && strcmp(cmd->args[i], "-n") == 0) {
        newline = 0;
        i++;
    }

    // Print arguments
    while (cmd->args[i])
    {
        char *arg = expand_variables(cmd->args[i], env);

        int in_quotes = 0;
        for (int j = 0; arg[j]; j++)
        {
            if (arg[j] == '"' && (j == 0 || arg[j-1] != '\\'))
            {
                in_quotes = !in_quotes;
                continue;
            }
            if (arg[j] == '\\' && arg[j+1] && !in_quotes)
            {
                j++;
                switch (arg[j])
                {
                    case 'n': putchar('\n'); break;
                    case 't': putchar('\t'); break;
                    default: putchar(arg[j]);
                }
            }
            else
            {
                putchar(arg[j]);
            }
        }

        free(arg);

        if (cmd->args[i + 1])
            printf(" "); 
        i++;
    }

    if (newline)
        printf("\n");

    return 0;
}

int builtin_cd(t_data *cmd, char **env)
{
    if (cmd->args[1] == NULL) {
        fprintf(stderr, "cd: missing argument\n");
        return 1;
    }
    if (chdir(cmd->args[1]) != 0) {
        perror("cd");
        return 1;
    }
    return 0;
}

int builtin_pwd(t_data *cmd, char **env)
{
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
        return 0;
    } else {
        perror("pwd");
        return 1;
    }
}

int execute_builtin(t_data *cmd, char **env)
{
    if (strcmp(cmd->args[0], "echo") == 0)
        return builtin_echo(cmd, env);
    else if (strcmp(cmd->args[0], "cd") == 0)
        return builtin_cd(cmd, env);
    else if (strcmp(cmd->args[0], "pwd") == 0)
        return builtin_pwd(cmd, env);
    else {
        fprintf(stderr, "Command not found: %s\n", cmd->args[0]);
        return 1;
    }
}

int main(int argc, char **argv, char **env)
{
    t_data cmd;
    cmd.args = argv + 1;
    cmd.file = NULL;
    cmd.next = NULL;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <builtin_command> [args...]\n", argv[0]);
        return 1;
    }

    return execute_builtin(&cmd, env);
}
