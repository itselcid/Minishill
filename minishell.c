#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_ARGS 100
#define MAX_PATH 1024

typedef struct s_file
{
	char			*file;
	int				infile;
	int				outfile;
	int				herdoc;
	int				apend;
	struct s_file	*next;
}					t_file;

typedef struct s_data
{
	char			**cmd;
	t_file			*file;
	struct s_data	*next;
}					t_data;

extern char			**environ;

int					exit_number = 0;

void	handle_sigint(int sig)
{
	(void)sig;
	write(1, "\n", 1);
	rl_on_new_line();
	rl_replace_line("", 0);
	rl_redisplay();
}

void	setup_signals(void)
{
	signal(SIGINT, handle_sigint);
	signal(SIGQUIT, SIG_IGN);
}

void	add_redirection(t_data *data, char *file, int type)
{
	t_file	*new_file;
	t_file	*temp;

	new_file = malloc(sizeof(t_file));
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
		temp = data->file;
		while (temp->next)
			temp = temp->next;
		temp->next = new_file;
	}
}
int	ft_setenv(char *key, char *value, int overwrite)
{
    int		i;
    int		key_len;
    int		value_len;
    char	*new_var;

    i = 0;
    key_len = strlen(key);
    value_len = strlen(value);
    while (environ[i])
    {
        if (strncmp(environ[i], key, key_len) == 0)
        {
            if (overwrite)
            {
                new_var = malloc(key_len + value_len + 2);
                strcpy(new_var, key);
                strcat(new_var, "=");
                strcat(new_var, value);
                free(environ[i]);
                environ[i] = new_var;
            }
            return (0);
        }
        i++;
    }
	if(value=="+" || value==NULL)
	{
		new_var = malloc(key_len + 1);
		strcpy(new_var, key);
	}
	else
	{
		new_var = malloc(key_len + value_len + 2);
		strcpy(new_var, key);
		strcat(new_var, "=");
		strcat(new_var, value);
	}

    environ[i] = new_var;
    environ[i + 1] = NULL;
    return (0);
}
int is_valid_identifier(const char *name) 
{

	int i = 1;
    if (!isalpha(name[0]) && name[0] != '_')
        return 0;
    while(name[i]) 
	{
        if (!isalnum(name[i]) && name[i] != '_')
            return 0;
		i++;
    }
    return 1;
}
int	builtin_export(t_data *data)
{
      int		i;
    int		env_count;
    char	*name;
    char	*value;
    char    *key;
    char    *val;

    env_count = 0;
    if (!data->cmd[1])
    {
        i = 0;
        while (environ[i])
        {
            key = strdup(environ[i]);
            val = strchr(key, '=');
            if (val)
            {
                *val = '\0';
                val++;
                printf("declare -x %s=\"%s\"\n", key, val);
            }
            else
            {
                printf("declare -x %s\n", key);
            }
            free(key);
            i++;
        }
        return (0);
    }
    i = 1;
    while (data->cmd[i])
    {
        name = strdup(data->cmd[i]);
        value = strchr(name, '=');
        if (value)
        {
            *value = '\0';
            value++;
        }
        if (!is_valid_identifier(name))
        {
            printf("bash: export: `%s': not a valid identifier\n", data->cmd[i]);
            free(name);
            return (1);
        }
        if (value)
        {
            ft_setenv(name, value, 1);
        }
        else
        {
            ft_setenv(name, "+", 0);
        }
        free(name);
        i++;
    }
    return (0);
}
int	builtin_env(void)
{
    char	**env;

    env = environ;
    while (*env)
    {
        if (strchr(*env, '='))
        {
            write(1, *env, strlen(*env));
            write(1, "\n", 1);
        }
        env++;
    }
    return (0);
}

int	is_builtin(char *cmd)
{
    char	*builtins[10]={"echo", "cd", "pwd", "export", "unset", "env", "exit", NULL};
    int			i;

    i = 0;
    while (builtins[i])
    {
        if (strcmp(cmd, builtins[i]) == 0)
            return (1);
        i++;
    }
    return (0);
}

int	builtin_echo(t_data *data)
{
    int	i;
    int	n_flag;

    i = 1;
    n_flag = 0;
    while (data->cmd[i] && strncmp(data->cmd[i], "-n", 2) == 0)
    {
        int j = 2;
        while (data->cmd[i][j] == 'n')
            j++;
        if (data->cmd[i][j] == '\0')
        {
            n_flag = 1;
            i++;
        }
        else
            break;
    }
    while (data->cmd[i])
    {
        write(1, data->cmd[i], strlen(data->cmd[i]));
        if (data->cmd[i + 1])
            write(1, " ", 1);
        i++;
    }
    if (!n_flag)
        write(1, "\n", 1);
    return (0);
}

int	builtin_cd(t_data *data)
{
	char	*path;
	char	cwd[MAX_PATH];

	path = data->cmd[1];
	if (!path)
	{
		path = getenv("HOME");
		if (!path)
		{
			write(2, "cd: HOME not set\n", 17);
			return (1);
		}
	}
	if (chdir(path) != 0)
	{
		perror("cd");
		return (1);
	}
	if (getcwd(cwd, sizeof(cwd)) != NULL)
		setenv("PWD", cwd, 1);
	else
	{
		perror("getcwd");
		return (1);
	}
	return (0);
}

int	builtin_pwd(void)
{
	char	cwd[MAX_PATH];

	if (getcwd(cwd, sizeof(cwd)) != NULL)
	{
		write(1, cwd, strlen(cwd));
		write(1, "\n", 1);
		return (0);
	}
	else
	{
		perror("pwd");
		return (1);
	}
}



int	builtin_unset(t_data *data)
{
	int	i;

	i = 1;
	while (data->cmd[i])
	{
		unsetenv(data->cmd[i]);
		i++;
	}
	return (0);
}



int	builtin_exit(t_data *data)
{
	int	exit_code;

	exit_code = 0;
	if (data->cmd[1])
	{
		exit_code = atoi(data->cmd[1]);
	}
	printf("exit\n");
	return (-1);
}

int	execute_builtin(t_data *data)
{
	if (strcmp(data->cmd[0], "echo") == 0)
		return (builtin_echo(data));
	else if (strcmp(data->cmd[0], "cd") == 0)
		return (builtin_cd(data));
	else if (strcmp(data->cmd[0], "pwd") == 0)
		return (builtin_pwd());
	else if (strcmp(data->cmd[0], "export") == 0)
		return (builtin_export(data));
	else if (strcmp(data->cmd[0], "unset") == 0)
		return (builtin_unset(data));
	else if (strcmp(data->cmd[0], "env") == 0)
		return (builtin_env());
	else if (strcmp(data->cmd[0], "exit") == 0)
		return (builtin_exit(data));
	return (1);
}

void	handle_redirections(t_file *file)
{
	int	fd;

	while (file)
	{
		if (file->infile)
		{
			fd = open(file->file, O_RDONLY);
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
			fd = open(file->file,
					O_WRONLY | O_CREAT | (file->apend ? O_APPEND : O_TRUNC),
					0644);
			if (fd == -1)
			{
				perror("open");
				exit(1);
			}
			dup2(fd, 1);
			close(fd);
		}
		file = file->next;
	}
}

char	*find_command(char *cmd)
{
	char	*path;
	char	*dir;
	char	*full_path;
	char	*path_copy;

	if (strchr(cmd, '/'))
		return (strdup(cmd));
	path = getenv("PATH");
	if (!path)
		return (NULL);
	path_copy = strdup(path);
	dir = strtok(path_copy, ":");
	while (dir)
	{
		full_path = malloc(strlen(dir) + strlen(cmd) + 2);
		strcpy(full_path, dir);
		strcat(full_path, "/");
		strcat(full_path, cmd);
		if (access(full_path, X_OK) == 0)
		{
			free(path_copy);
			return (full_path);
		}
		free(full_path);
		dir = strtok(NULL, ":");
	}
	free(path_copy);
	return (NULL);
}

int	execute_command(t_data *data)
{
	pid_t	pid;
	int		status;
	char	*cmd_path;

	if (is_builtin(data->cmd[0]))
		return (execute_builtin(data));
	pid = fork();
	if (pid == -1)
	{
		perror("fork");
		return (1);
	}
	else if (pid == 0)
	{
		handle_redirections(data->file);
		cmd_path = find_command(data->cmd[0]);
		if (!cmd_path)
		{
			write(2, "Command not found: ", 19);
			write(2, data->cmd[0], strlen(data->cmd[0]));
			write(2, "\n", 1);
			exit(127);
		}
		execve(cmd_path, data->cmd, environ);
		perror("bash");
		free(cmd_path);
		exit(1);
	}
	else
	{
		waitpid(pid, &status, 0);
		return ((status & 0xff00) >> 8);
	}
}

int	execute_pipeline(t_data *data)
{
	int		pipefd[2];
	pid_t	pid;
	int		status;
	int		in_fd;
				char *cmd_path;

	in_fd = STDIN_FILENO;
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
				dup2(pipefd[1], 1);
				close(pipefd[1]);
			}
			handle_redirections(data->file);
			if (is_builtin(data->cmd[0]))
				exit(execute_builtin(data));
			else
			{
				cmd_path = find_command(data->cmd[0]);
				if (!cmd_path)
				{
					write(STDERR_FILENO, "Command not found: ", 19);
					write(STDERR_FILENO, data->cmd[0], strlen(data->cmd[0]));
					write(STDERR_FILENO, "\n", 1);
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
			return (1);
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
	return ((status & 0xff00) >> 8);
}

char	*expand_variables(char *arg)
{
	char	*result;
	char	*dst;
	char	*src;
	int		in_quotes;
				char var_name[256];
	int		i;
	char	*value;

	result = malloc(strlen(arg) * 2 + 1);
	dst = result;
	src = arg;
	in_quotes = 0;
	while (*src)
	{
		if (*src == '"')
		{
			in_quotes = !in_quotes;
			src++;
		}
		else if (*src == '$' && (!in_quotes || isalnum(src[1]) || src[1] == '_'
				|| src[1] == '?'))
		{
			src++;
			if (*src == '?')
			{
				dst += sprintf(dst, "%d", exit_number);
				src++;
			}
			else
			{
				i = 0;
				while (src[i] && (isalnum(src[i]) || src[i] == '_'))
				{
					var_name[i] = src[i];
					i++;
				}
				var_name[i] = '\0';
				value = getenv(var_name);
				if (value)
				{
					dst += sprintf(dst, "%s", value);
				}
				src += i;
			}
		}
		else
		{
			*dst++ = *src++;
		}
	}
	*dst = '\0';
	return (result);
}

t_data	*parse_input(char *input)
{
	t_data	*head;
	t_data	*current;
	char	*token;
	char	*saveptr;
	char	*cmd_token;
	char	*cmd_saveptr;
	int		i;
	t_data	*new_data;

	head = NULL;
	current = NULL;
	token = strtok_r(input, "|", &saveptr);
	while (token)
	{
		new_data = malloc(sizeof(t_data));
		new_data->cmd = malloc(sizeof(char *) * MAX_ARGS);
		new_data->file = NULL;
		new_data->next = NULL;
		cmd_token = strtok_r(token, " \t\n", &cmd_saveptr);
		i = 0;
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
				new_data->cmd[i++] = expand_variables(cmd_token);
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
	return (head);
}

void	free_data(t_data *data)
{
	t_data	*next;
	t_file	*file;
	t_file	*next_file;
	int		i;

	while (data)
	{
		next = data->next;
		i = 0;
		while (data->cmd[i])
		{
			free(data->cmd[i]);
			i++;
		}
		free(data->cmd);
		file = data->file;
		while (file)
		{
			next_file = file->next;
			free(file->file);
			free(file);
			file = next_file;
		}
		free(data);
		data = next;
	}
}
int	main(void)
{
	char	*input;
	t_data	*data;
	int		should_exit;
	int		exit_status;

	should_exit = 0;
	setup_signals();
	while (!should_exit)
	{
		input = readline("\033[1;33mminishell> \033[0m");
		if (!input)
		{
			printf("exit\n");
			break ;
		}
		if (*input)
		{
			add_history(input);
			data = parse_input(input);
			if (data)
			{
				if (data->next)
					exit_number = execute_pipeline(data);
				else
				{
					if (strcmp(data->cmd[0], "exit") == 0)
					{
						exit_status = builtin_exit(data);
						if (exit_status == -1)
						{
							should_exit = 1;
							if (data->cmd[1])
								exit_number = atoi(data->cmd[1]);
						}
					}
					else
						exit_number = execute_command(data);
				}
				free_data(data);
			}
		}
		free(input);
	}
	return (exit_number);
}
