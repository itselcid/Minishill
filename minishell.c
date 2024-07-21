#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>

#define MAX_TOKEN_LENGTH 1024

typedef enum {
    TOKEN_WORD,
    TOKEN_PIPE,
    TOKEN_REDIRECT_IN,
    TOKEN_REDIRECT_OUT,
    TOKEN_REDIRECT_APPEND,
    TOKEN_HEREDOC
} token_type;

typedef struct s_token {
    char *value;
    token_type type;
} t_token;

int is_special_char(char c) {
    return (c == '|' || c == '<' || c == '>');
}

t_token *create_token(char *value, token_type type) {
    t_token *token = malloc(sizeof(t_token));
    if (!token)
        return NULL;
    token->value = value;
    token->type = type;
    return token;
}

void handle_quotes(char **input, char *buffer, int *buffer_index) {
    char quote = **input;
    (*input)++;
    while (**input && **input != quote) {
        buffer[(*buffer_index)++] = **input;
        (*input)++;
    }
    if (**input == quote)
        (*input)++;
}

t_token **tokenize(char *input) {
    t_token **tokens = NULL;
    int token_count = 0;
    char *current = input;
    char buffer[MAX_TOKEN_LENGTH];
    int buffer_index = 0;

    while (*current) {
        if (*current == ' ' || *current == '\t') {
            if (buffer_index > 0) {
                buffer[buffer_index] = '\0';
                tokens = realloc(tokens, (token_count + 1) * sizeof(t_token *));
                tokens[token_count++] = create_token(strdup(buffer), TOKEN_WORD);
                buffer_index = 0;
            }
            current++;
        } else if (*current == '\'' || *current == '"') {
            handle_quotes(&current, buffer, &buffer_index);
        } else if (is_special_char(*current)) {
            if (buffer_index > 0) {
                buffer[buffer_index] = '\0';
                tokens = realloc(tokens, (token_count + 1) * sizeof(t_token *));
                tokens[token_count++] = create_token(strdup(buffer), TOKEN_WORD);
                buffer_index = 0;
            }
            if (*current == '|') {
                tokens = realloc(tokens, (token_count + 1) * sizeof(t_token *));
                tokens[token_count++] = create_token(strdup("|"), TOKEN_PIPE);
            } else if (*current == '<') {
                if (*(current + 1) == '<') {
                    tokens = realloc(tokens, (token_count + 1) * sizeof(t_token *));
                    tokens[token_count++] = create_token(strdup("<<"), TOKEN_HEREDOC);
                    current++;
                } else {
                    tokens = realloc(tokens, (token_count + 1) * sizeof(t_token *));
                    tokens[token_count++] = create_token(strdup("<"), TOKEN_REDIRECT_IN);
                }
            } else if (*current == '>') {
                if (*(current + 1) == '>') {
                    tokens = realloc(tokens, (token_count + 1) * sizeof(t_token *));
                    tokens[token_count++] = create_token(strdup(">>"), TOKEN_REDIRECT_APPEND);
                    current++;
                } else {
                    tokens = realloc(tokens, (token_count + 1) * sizeof(t_token *));
                    tokens[token_count++] = create_token(strdup(">"), TOKEN_REDIRECT_OUT);
                }
            }
            current++;
        } else {
            buffer[buffer_index++] = *current++;
        }
    }

    if (buffer_index > 0) {
        buffer[buffer_index] = '\0';
        tokens = realloc(tokens, (token_count + 1) * sizeof(t_token *));
        tokens[token_count++] = create_token(strdup(buffer), TOKEN_WORD);
    }

    tokens = realloc(tokens, (token_count + 1) * sizeof(t_token *));
    tokens[token_count] = NULL;

    return tokens;
}

void free_tokens(t_token **tokens) {
    for (int i = 0; tokens[i] != NULL; i++) {
        free(tokens[i]->value);
        free(tokens[i]);
    }
    free(tokens);
}

int main() {
    char *input;
    t_token **tokens;

    while (1) {
        input = readline("minishell> ");
        if (!input)
            break;

        if (*input)
            add_history(input);

        tokens = tokenize(input);

        // Print tokens for debugging
        for (int i = 0; tokens[i] != NULL; i++) {
            write(STDOUT_FILENO, "Token: ", 7);
            write(STDOUT_FILENO, tokens[i]->value, strlen(tokens[i]->value));
            write(STDOUT_FILENO, "\n", 1);
        }

        free_tokens(tokens);
        free(input);
    }

    rl_clear_history();
    return 0;
}


