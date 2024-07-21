#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>


typedef struct t_data
{
    char *cmd;
    t_file *file;
    struct t_data *next;
}t_data;

typedef struct t_file
{
    char *file;
    int infile;
    int outfile;
    int herdoc;
    int apend;
    struct t_file *next;
}t_file;


int ft_execution(t_data *data, char **env)
{
    int status = 0;
    t_data *current = data;

    while (current)
    {
        status = execute_command(current, env);
        current = current->next;
    }
    return status;
}

int execute_command(t_data *cmd, char **env)
{
    if (is_builtin(cmd->cmd))
        return execute_builtin(cmd, env);
    else
        return execute_external(cmd, env);
}

int is_builtin(const char *cmd)
{
    static const char *builtins[] = {"echo", "cd", "pwd", "export", "unset", "env", "exit", NULL};
    for (int i = 0; builtins[i]; i++)
    {
        if (strncmp(cmd, builtins[i], strlen(builtins[i])) == 0)
            return 1;
    }
    return 0;
}

int execute_builtin(t_data *cmd, char **env)
{
    if (strncmp(cmd->cmd, "echo", 4) == 0)
        return builtin_echo(cmd);
    else if (strncmp(cmd->cmd, "cd", 2) == 0)
        return builtin_cd(cmd);
    else if (strncmp(cmd->cmd, "pwd", 3) == 0)
        return builtin_pwd();
    else if (strncmp(cmd->cmd, "export", 6) == 0)
        return builtin_export(cmd, env);
    else if (strncmp(cmd->cmd, "unset", 5) == 0)
        return builtin_unset(cmd, env);
    else if (strncmp(cmd->cmd, "env", 3) == 0)
        return builtin_env(env);
    else if (strncmp(cmd->cmd, "exit", 4) == 0)
        return builtin_exit(cmd);
    return 1;
}

int execute_external(t_data *cmd, char **env)
{
    pid_t pid = fork();

    if (pid == 0)
    {
        // Child process
        setup_redirections(cmd->file);

        char **args = split_command(cmd->cmd);
        execve(args[0], args, env);
        perror("execve");
        exit(EXIT_FAILURE);
    }
    else if (pid < 0)
    {
        perror("fork");
        return -1;
    }
    else
    {
        // Parent process
        int status;
        waitpid(pid, &status, 0);
        return WEXITSTATUS(status);
    }
}

void setup_redirections(t_file *files)
{
    while (files)
    {
        int fd;
        if (files->infile)
        {
            fd = open(files->file, O_RDONLY);
            dup2(fd, STDIN_FILENO);
            close(fd);
        }
        else if (files->outfile)
        {
            fd = open(files->file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }
        else if (files->apend)
        {
            fd = open(files->file, O_WRONLY | O_CREAT | O_APPEND, 0644);
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }
        else if (files->herdoc)
        {
            // Implement heredoc
        }
        files = files->next;
    }
}

char **split_command(char *cmd)
{
    // Implement command splitting
    // Return an array of strings suitable for execve
}

// Implement builtin functions
int builtin_echo(t_data *cmd) { /* ... */ }
int builtin_cd(t_data *cmd) { /* ... */ }
int builtin_pwd() { /* ... */ }
int builtin_export(t_data *cmd, char **env) { /* ... */ }
int builtin_unset(t_data *cmd, char **env) { /* ... */ }
int builtin_env(char **env) { /* ... */ }
int builtin_exit(t_data *cmd) { /* ... */ }
