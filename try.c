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
#include <dirent.h>
#include <errno.h>
#include <fnmatch.h>

#define MAX_ARGS 100
#define MAX_PATH 1024

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

// Global variable for exit status
int g_exit_status = 0;

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
    char *path = data->cmd[1];
    if (!path)
    {
        path = getenv("HOME");
        if (!path)
        {
            fprintf(stderr, "cd: HOME not set\n");
            return 1;
        }
    }

    if (chdir(path) != 0)
    {
        perror("cd");
        return 1;
    }

    char cwd[MAX_PATH];
    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
        setenv("PWD", cwd, 1);
    }
    else
    {
        perror("getcwd");
        return 1;
    }

    return 0;
}

int builtin_pwd(void)
{
    char cwd[MAX_PATH];
    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
        printf("%s\n", cwd);
        return 0;
    }
    else
    {
        perror("pwd");
        return 1;
    }
}

int builtin_export(t_data *data)
{
    if (!data->cmd[1])
    {
        // Print all environment variables
        for (char **env = environ; *env != 0; env++)
        {
            char *temp = strdup(*env);
            char *equals = strchr(temp, '=');
            if (equals)
                *equals = '\0';
            printf("declare -x %s", temp);
            if (equals)
                printf("=\"%s\"", equals + 1);
            printf("\n");
            free(temp);
        }
        return 0;
    }

    for (int i = 1; data->cmd[i]; i++)
    {
        char *equals = strchr(data->cmd[i], '=');
        if (equals)
        {
            *equals = '\0';
            setenv(data->cmd[i], equals + 1, 1);
            *equals = '=';
        }
        else
        {
            setenv(data->cmd[i], "", 1);
        }
    }

    return 0;
}

int builtin_unset(t_data *data)
{
    for (int i = 1; data->cmd[i]; i++)
    {
        unsetenv(data->cmd[i]);
    }
    return 0;
}

int builtin_env(void)
{
    for (char **env = environ; *env != 0; env++)
    {
        printf("%s\n", *env);
    }
    return 0;
}

int builtin_exit(t_data *data)
{
    int exit_code = 0;
    if (data->cmd[1])
    {
        exit_code = atoi(data->cmd[1]);
    }
    printf("exit\n");
    exit(exit_code);
}

int execute_builtin(t_data *data)
{
    if (strcmp(data->cmd[0], "echo") == 0)
        return builtin_echo(data);
    else if (strcmp(data->cmd[0], "cd") == 0)
        return builtin_cd(data);
    else if (strcmp(data->cmd[0], "pwd") == 0)
        return builtin_pwd();
    else if (strcmp(data->cmd[0], "export") == 0)
        return builtin_export(data);
    else if (strcmp(data->cmd[0], "unset") == 0)
        return builtin_unset(data);
    else if (strcmp(data->cmd[0], "env") == 0)
        return builtin_env();
    else if (strcmp(data->cmd[0], "exit") == 0)
        return builtin_exit(data);
    return 1;
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
        if (file->herdoc)
        {
            // Implement heredoc functionality
            // This is a simplified version and needs more work
            char *line = NULL;
            size_t len = 0;
            ssize_t read;
            while ((read = getline(&line, &len, stdin)) != -1)
            {
                if (strcmp(line, file->file) == 0)
                    break;
                printf("%s", line);
            }
            free(line);
        }
        file = file->next;
    }
}

char *find_command(char *cmd)
{
    if (strchr(cmd, '/'))
        return strdup(cmd);

    char *path = getenv("PATH");
    if (!path)
        return NULL;

    char *path_copy = strdup(path);
    char *dir = strtok(path_copy, ":");

    while (dir)
    {
        char full_path[MAX_PATH];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir, cmd);

        if (access(full_path, X_OK) == 0)
        {
            free(path_copy);
            return strdup(full_path);
        }

        dir = strtok(NULL, ":");
    }

    free(path_copy);
    return NULL;
}

int execute_command(t_data *data)
{
    if (is_builtin(data->cmd[0]))
        return execute_builtin(data);

    pid_t pid = fork();

    if (pid == -1)
    {
        perror("fork");
        return 1;
    }
    else if (pid == 0)
    {
        handle_redirections(data->file);

        char *cmd_path = find_command(data->cmd[0]);
        if (!cmd_path)
        {
            fprintf(stderr, "Command not found: %s\n", data->cmd[0]);
            exit(127);
        }

        execve(cmd_path, data->cmd, environ);
        perror("execve");
        free(cmd_path);
        exit(1);
    }
    else
    {
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status))
            return WEXITSTATUS(status);
        return 1;
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

            if (is_builtin(data->cmd[0]))
            {
                exit(execute_builtin(data));
            }
            else
            {
                char *cmd_path = find_command(data->cmd[0]);
                if (!cmd_path)
                {
                    fprintf(stderr, "Command not found: %s\n", data->cmd[0]);
                    exit(127);
                }
                execve(cmd_path, data->cmd, environ);
                perror("execve");
                free(cmd_path);
                exit(1);
            }
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

    if (WIFEXITED(status))
        return WEXITSTATUS(status);
    return 1;
}

void expand_wildcards(char ***args, int *arg_count)
{
    char **new_args = NULL;
    int new_count = 0;

    for (int i = 0; i < *arg_count; i++)
    {
        if (strchr((*args)[i], '*'))
        {
            DIR *dir;
            struct dirent *entry;
            char *pattern = (*args)[i];

            dir = opendir(".");
            if (dir == NULL)
            {
                perror("opendir");
                continue;
            }

            while ((entry = readdir(dir)) != NULL)
            {
                if (fnmatch(pattern, entry->d_name, 0) == 0)
                {
                    new_args = realloc(new_args, (new_count + 1) * sizeof(char *));
                    new_args[new_count++] = strdup(entry->d_name);
                }
            }

            closedir(dir);
        }
        else
        {
            new_args = realloc(new_args, (new_count + 1) * sizeof(char *));
            new_args[new_count++] = strdup((*args)[i]);
        }
    }

    // Free old args and update with new ones
    for (int i = 0; i < *arg_count; i++)
        free((*args)[i]);
    free(*args);

    *args = new_args;
    *arg_count = new_count;
}

char *expand_variables(char *arg)
{
    char *result = malloc(strlen(arg) * 2);  // Allocate more space for expansion
    char *dst = result;
    char *src = arg;

    while (*src)
    {
        if (*src == '$')
        {
            src++;
            if (*src == '?')
            {
                dst += sprintf(dst, "%d", g_exit_status);
                src++;
            }
            else if (*src == '{')
            {
                char var_name[256];
                char *end = strchr(src, '}');
                if (end)
                {
                    strncpy(var_name, src + 1, end - src - 1);
                    var_name[end - src - 1] = '\0';
                    char *value = getenv(var_name);
                    if (value)
                        dst += sprintf(dst, "%s", value);
                    src = end + 1;
                }
                else
                {
                    *dst++ = '$';
                    *dst++ = '{';
                    src++;
                }
            }
            else
            {
                char var_name[256];
                int i = 0;
                while (src[i] && (isalnum(src[i]) || src[i] == '_'))
                {
                    var_name[i] = src[i];
                    i++;
                }
                var_name[i] = '\0';
                char *value = getenv(var_name);
                if (value)
                    dst += sprintf(dst, "%s", value);
                src += i;
            }
        }
        else
        {
            *dst++ = *src++;
        }
    }
    *dst = '\0';

    return result;
}

t_data *parse_input(char *input)
{
    t_data *head = NULL;
    t_data *current = NULL;
    char *token;
    char *saveptr;

    token = strtok_r(input, "|", &saveptr);
    while (token)
    {
        t_data *new_data = malloc(sizeof(t_data));
        new_data->cmd = malloc(sizeof(char *) * MAX_ARGS);
        new_data->file = NULL;
        new_data->next = NULL;

        char *cmd_saveptr;
        char *cmd_token = strtok_r(token, " \t\n", &cmd_saveptr);
        int i = 0;
        while (cmd_token)
        {
            if (strcmp(cmd_token, "<") == 0)
            {
                cmd_token = strtok_r(NULL, " \t\n", &cmd_saveptr);
                if (cmd_token)
                    add_redirection(new_data, cmd_token, 0);
            }
            else if (strcmp(cmd_token, ">") == 0)
            {
                cmd_token = strtok_r(NULL, " \t\n", &cmd_saveptr);
                if (cmd_token)
                    add_redirection(new_data, cmd_token, 1);
            }
            else if (strcmp(cmd_token, ">>") == 0)
            {
                cmd_token = strtok_r(NULL, " \t\n", &cmd_saveptr);
                if (cmd_token)
                    add_redirection(new_data, cmd_token, 2);
            }
            else
            {
                new_data->cmd[i++] = strdup(cmd_token);
            }
            cmd_token = strtok_r(NULL, " \t\n", &cmd_saveptr);
        }
        new_data->cmd[i] = NULL;

        if (!head)
            head = new_data;
        else
            current->next = new_data;
        current = new_data;

        token = strtok_r(NULL, "|", &saveptr);
    }

    return head;
}
void add_redirection(t_data *data, char *file, int type)
{
    t_file *new_file = malloc(sizeof(t_file));
    new_file->file = strdup(file);
    new_file->infile = (type == 0);
    new_file->outfile = (type == 1 || type == 2);
    new_file->apend = (type == 2);
    new_file->herdoc = 0;
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
void free_data(t_data *data)
{
    while (data)
    {
        t_data *next = data->next;
        for (int i = 0; data->cmd[i]; i++)
            free(data->cmd[i]);
        free(data->cmd);
        
        t_file *file = data->file;
        while (file)
        {
            t_file *next_file = file->next;
            free(file->file);
            free(file);
            file = next_file;
        }
        
        free(data);
        data = next;
    }
}
int main()
{
    char *input;
    t_data *data;

    setup_signals();

    while (1)
    {
        input = readline("minishell> ");
        if (!input)
            break;

        if (*input)
        {
            add_history(input);
            data = parse_input(input);
            if (data)
            {
                if (data->next)
                    g_exit_status = execute_pipeline(data);
                else
                    g_exit_status = execute_command(data);
                free_data(data);
            }
        }

        free(input);
    }

    printf("exit\n");
    return 0;
}

