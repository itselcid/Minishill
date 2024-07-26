NAME = minishell

CC = gcc
# CFLAGS = -Wall -Wextra -Werror
LIBS = -lreadline

SRCS = minishell.c
OBJS = $(SRCS:.c=.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CC)  $(OBJS) -o $(NAME) $(LIBS)


clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all


