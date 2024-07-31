#include <stdlib.h>
#include <stdio.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern char **environ;

int ft_setenv(const char *name, const char *value, int overwrite)
{
    int i;
    size_t name_len = strlen(name);
    size_t value_len = strlen(value);
    char *new_var;

    // Check if the environment variable already exists
    for (i = 0; environ[i] != NULL; i++)
    {
        if (strncmp(environ[i], name, name_len) == 0 && environ[i][name_len] == '=')
        {
            if (overwrite)
            {
                // Allocate memory for the new variable
                new_var = malloc(name_len + value_len + 2);
                if (!new_var)
                {
                    perror("malloc");
                    return -1;
                }
            // Construct the new variable string
                strcpy(new_var, name);
                strcat(new_var, "=");
                strcat(new_var, value);
                // Replace the old variable
                environ[i] = new_var;
            }
            return 0;
        }
    }

    // Allocate memory for the new variable
    new_var = malloc(name_len + value_len + 2);
    if (!new_var)
    {
        perror("malloc");
        return -1;
    }
    // Construct the new variable string
    strcpy(new_var, name);
    strcat(new_var, "=");
    strcat(new_var, value);

    // Add the new variable to the environ array
    for (i = 0; environ[i] != NULL; i++);
    environ[i] = new_var;
    environ[i + 1] = NULL;

    return 0;
}
int main() {
    // Set the environment variable "MY_VAR" to "Hello"
    ft_setenv("MY_VAR", "Hello0", 1);

    // Retrieve and print the value of "MY_VAR"
    char *value = getenv("MY_VAR");
    if (value) {
        printf("MY_VAR = %s\n", value);
    } else {
        printf("MY_VAR is not set\n");
    }

    // Attempt to set "MY_VAR" to "World" without overwriting
    ft_setenv("MY_VAR", "World", 0);

    // Retrieve and print the value of "MY_VAR" again
    value = getenv("MY_VAR");
    if (value) {
        printf("MY_VAR = %s\n", value);
    } else {
        printf("MY_VAR is not set\n");
    }

    // Set "MY_VAR" to "World" with overwriting
    ft_setenv("MY_VAR", "World", -7);

    // Retrieve and print the value of "MY_VAR" again
    value = getenv("MY_VAR");
    if (value) {
        printf("MY_VAR = %s\n", value);
    } else {
        printf("MY_VAR is not set\n");
    }

    return 0;
}