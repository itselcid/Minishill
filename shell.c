#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>


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