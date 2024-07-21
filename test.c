#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <readline/readline.h>
#include <readline/history.h>

typedef struct s_file
{
    char *file;
    int infile;
    int outfile;
    int herdoc;
    int apend;
    struct s_file *next;
} t_file;

typedef struct s_data
{
    char **cmd;
    t_file *file;
    struct s_data *next;
} t_data;

extern char **environ;

void handle_sigint(int sig)
{
    (void)sig;
    write(STDOUT_FILENO, "\n", 1);
    rl_on_new_line();
    rl_replace_line("", 0);
    rl_redisplay();
}

void setup_signals(void)
{
    signal(SIGINT, handle_sigint);
    signal(SIGQUIT, SIG_IGN);
}

int is_builtin(char *cmd)
{
    const char *builtins[] = {"echo", "cd", "pwd", "export", "unset", "env", "exit", NULL};
    for (int i = 0; builtins[i]; i++)
        if (strcmp(cmd, builtins[i]) == 0)
            return 1;
    return 0;
}

int builtin_echo(t_data *data)
{
    int i = 1;
    int n_flag = 0;

    if (data->cmd[1] && strcmp(data->cmd[1], "-n") == 0)
    {
        n_flag = 1;
        i++;
    }

    while (data->cmd[i])
    {
        printf("%s", data->cmd[i]);
        if (data->cmd[i + 1])
            printf(" ");
        i++;
    }

    if (!n_flag)
        printf("\n");

    return 0;
}

int builtin_cd(t_data *data)
{
    if (!data->cmd[1])
    {
        char *home = getenv("HOME");
        if (!home)
        {
            fprintf(stderr, "cd: HOME not set\n");
            return 1;
        }
        if (chdir(home) != 0)
        {
            perror("cd");
            return 1;
        }
    }
    else if (chdir(data->cmd[1]) != 0)
    {
        perror("cd");
        return 1;
    }
    return 0;
}

int execute_builtin(t_data *data)
{
    if (strcmp(data->cmd[0], "echo") == 0)
        return builtin_echo(data);
    else if (strcmp(data->cmd[0], "cd") == 0)
        return builtin_cd(data);
    // Implement other builtins here
    return 0;
}

void handle_redirections(t_file *file)
{
    while (file)
    {
        if (file->infile)
        {
            int fd = open(file->file, O_RDONLY);
            if (fd == -1)
            {
                perror("open");
                exit(1);
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
        }
        if (file->outfile)
        {
            int flags = O_WRONLY | O_CREAT | (file->apend ? O_APPEND : O_TRUNC);
            int fd = open(file->file, flags, 0644);
            if (fd == -1)
            {
                perror("open");
                exit(1);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }
        file = file->next;
    }
}

int execute_command(t_data *data)
{
    pid_t pid;
    int status;

    if (is_builtin(data->cmd[0]))
        return execute_builtin(data);

    pid = fork();

    if (pid == -1)
    {
        perror("fork");
        return 1;
    }
    else if (pid == 0)
    {
        handle_redirections(data->file);
        execve(data->cmd[0], data->cmd, environ);
        perror("execve");
        exit(1);
    }
    else
    {
        waitpid(pid, &status, 0);
        return WEXITSTATUS(status);
    }
}

int execute_pipeline(t_data *data)
{
    int pipefd[2];
    pid_t pid;
    int status;
    int in_fd = STDIN_FILENO;

    while (data)
    {
        if (data->next)
            pipe(pipefd);

        pid = fork();
        if (pid == 0)
        {
            if (in_fd != STDIN_FILENO)
            {
                dup2(in_fd, STDIN_FILENO);
                close(in_fd);
            }
            if (data->next)
            {
                close(pipefd[0]);
                dup2(pipefd[1], STDOUT_FILENO);
                close(pipefd[1]);
            }
            handle_redirections(data->file);
            execve(data->cmd[0], data->cmd, environ);
            perror("execve");
            exit(1);
        }
        else if (pid < 0)
        {
            perror("fork");
            return 1;
        }

        if (in_fd != STDIN_FILENO)
            close(in_fd);
        if (data->next)
        {
            close(pipefd[1]);
            in_fd = pipefd[0];
        }
        data = data->next;
    }

    while (wait(&status) > 0)
        ;

    return WEXITSTATUS(status);
}

t_data *create_test_command(const char *command, char **args, int arg_count)
{
    t_data *data = malloc(sizeof(t_data));
    if (!data)
        return NULL;

    data->cmd = malloc(sizeof(char *) * (arg_count + 2));
    if (!data->cmd)
    {
        free(data);
        return NULL;
    }

    data->cmd[0] = strdup(command);
    for (int i = 0; i < arg_count; i++)
    {
        data->cmd[i + 1] = strdup(args[i]);
    }
    data->cmd[arg_count + 1] = NULL;

    data->file = NULL;
    data->next = NULL;

    return data;
}

void add_redirection(t_data *data, const char *filename, int type)
{
    t_file *new_file = malloc(sizeof(t_file));
    if (!new_file)
        return;

    new_file->file = strdup(filename);
    new_file->infile = (type == 0);
    new_file->outfile = (type == 1);
    new_file->herdoc = 0;
    new_file->apend = (type == 2);
    new_file->next = NULL;

    if (!data->file)
        data->file = new_file;
    else
    {
        t_file *temp = data->file;
        while (temp->next)
            temp = temp->next;
        temp->next = new_file;
    }
}

void free_command(t_data *command)
{
    if (!command)
        return;

    for (int i = 0; command->cmd[i]; i++)
        free(command->cmd[i]);
    free(command->cmd);

    t_file *file = command->file;
    while (file)
    {
        t_file *next = file->next;
        free(file->file);
        free(file);
        file = next;
    }

    free(command);
}

int main()
{
    setup_signals();

    while (1)
    {
        char *input = readline("minishell> ");
        if (!input)
            break;  // EOF (Ctrl+D)

        add_history(input);

        // For testing purposes, we'll create a simple command
        char *args[] = {"-l", "-a"};
        t_data *command = create_test_command("ls", args, 2);

        if (!command)
        {
            fprintf(stderr, "Failed to create test command\n");
            free(input);
            continue;
        }

        // Uncomment to test redirection
        // add_redirection(command, "output.txt", 1);

        int status;
        if (command->next)
            status = execute_pipeline(command);
        else
            status = execute_command(command);

        printf("Command executed with status: %d\n", status);

        free_command(command);
        free(input);
    }

    printf("exit\n");
    return 0;
}
