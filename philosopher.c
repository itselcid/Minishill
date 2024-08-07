
#include <stdlib.h>
# include <stdio.h>
# include <unistd.h>
# include <pthread.h>
# include <sys/time.h>
# include <string.h>

#define FORK 1
#define EAT 2
#define SLEEP 3
#define THINK 4
#define DEAD 5

unsigned long long int current_time_ms(void);

struct philosopher
{
    int						id;
    struct philosopher		*next;
    pthread_mutex_t			mutex_fork;
    pthread_t				thread_id;
    unsigned long long int	current_state;
    unsigned long long int	max_time_to_die;
    unsigned long long int	time_to_eat;
    unsigned long long int	time_to_sleep;
    unsigned long long int	last_time_ate;
    unsigned long long int	meals_eaten;
    unsigned long long int	required_meals;
};

void	print_status(int id, int action)
{
    static pthread_mutex_t	print_mutex;
    static pthread_mutex_t	*mutex_ptr = NULL;

    if (mutex_ptr == NULL)
    {
        mutex_ptr = &print_mutex;
        pthread_mutex_init(mutex_ptr, NULL);
    }
    pthread_mutex_lock(mutex_ptr);
    if (action == FORK)
        printf("%llu %d has taken a fork\n", current_time_ms(), id);
    else if (action == EAT)
        printf("%llu %d is eating\n", current_time_ms(), id);
    else if (action == SLEEP)
        printf("%llu %d is sleeping\n", current_time_ms(), id);
    else if (action == THINK)
        printf("%llu %d is thinking\n", current_time_ms(), id);
    else
    {
        printf("%llu %d died\n", current_time_ms(), id);
        return ;
    }
    pthread_mutex_unlock(mutex_ptr);
}

unsigned long long int	current_time_ms(void)
{
    unsigned long long int			ms;
    static unsigned long long int	start_time = 0;
    static struct timeval			current_time;

    gettimeofday(&current_time, NULL);
    ms = current_time.tv_sec * 1000;
    ms += current_time.tv_usec / 1000;
    if (!start_time)
        start_time = ms;
    return (ms - start_time);
}

void free_philosophers(struct philosopher *head) {
    if (head == NULL) 
        return; // If the list is empty, exit the function

    struct philosopher *start = head;
    struct philosopher *temp;

    // Loop until we've circled back to the start
    while (head->next != start) 
    {
        temp = head;
        head = head->next;
        free(temp);
    }
    // Free the last philosopher manually, as the loop condition prevents freeing the start node
    free(head);
}
int	all_philosophers_satisfied(struct philosopher *head)
{
    struct philosopher	*current;

    current = head;
    while (1)
    {
        if (current->required_meals > 0)
            return (0);
        current = current->next;
        if (current == head)
            break ;
        usleep(111);
    }
    return (1);
}

void	*monitor_health(void *arg)
{
    struct philosopher	*philo;

    philo = (struct philosopher *) arg;
    while (1)
    {
        if (philo->required_meals == 0 && all_philosophers_satisfied(philo))
            return (NULL);
        if (philo->last_time_ate + philo->max_time_to_die < current_time_ms())
        {
            print_status(philo->id, DEAD);
            return (NULL);
        }
        philo = philo->next;
        usleep(111);
    }
}

int	check_arguments(int argc, char **argv)
{
    int	i;

    i = 1;
    if (argc > 6)
        return (write(STDERR_FILENO, "Error:\nToo many args\n", 21));
    if (argc < 5)
        return (write(STDERR_FILENO, "Error:\nArguments needed\n", 24));
    while (i < argc)
        if (atoi(argv[i++]) <= 0)
            return (write(STDERR_FILENO, "Error:\nA non-valid argument\n", 28));
    return (0);
}

int	ft_atoi(char *str)
{
    int	i;
    int	number;
    int	prev_number;

    i = 0;
    number = 0;
    prev_number = 0;
    if (str[i] == '+')
        i++;
    while (str[i] >= '0' && str[i] <= '9')
    {
        number = (str[i] - '0') + (number * 10);
        i++;
        if (number < prev_number)
            return (0);
        prev_number = number;
    }
    if (str[i] != '\0')
        return (0);
    return (number);
}

struct philosopher	*initialize_philosophers(int argc, char **argv) {
    struct philosopher *head = NULL;
    struct philosopher *current = NULL;
    int num_philosophers = ft_atoi(argv[1]);
    int i;
    
    i = 1;
    while (i <= num_philosophers) {
        struct philosopher *new_philo = (struct philosopher *)malloc(sizeof(struct philosopher));
        if (new_philo == NULL) 
        {
            free_philosophers(head); 
            return NULL;
        }
        new_philo->id = i;
        new_philo->max_time_to_die = ft_atoi(argv[2]);
        new_philo->time_to_eat = ft_atoi(argv[3]);
        new_philo->time_to_sleep = ft_atoi(argv[4]);
        if (argc == 6) {
            new_philo->required_meals = ft_atoi(argv[5]);
        } else {
            new_philo->required_meals = -1;
        }
        new_philo->last_time_ate = 0;
        new_philo->next = NULL;

        if (head == NULL)
         {
            head = new_philo;
            current = new_philo;
         }
         else 
         {
            current->next = new_philo;
            current = new_philo;
        }
        i++;
    }

    if (current != NULL) 
    {
        current->next = head; 
    }

    return head;
}

void	*philosopher_routine(void *arg)
{
    struct philosopher	*philo;

    philo = (struct philosopher *) arg;
    while (1)
    {
        if (current_time_ms() <= 10 && philo->id % 2 == 0)
            usleep(philo->time_to_eat * 1000);
        pthread_mutex_lock(&philo->mutex_fork);
        print_status(philo->id, FORK);
        pthread_mutex_lock(&philo->next->mutex_fork);
        philo->current_state = EAT;
        print_status(philo->id, FORK);
        philo->last_time_ate = current_time_ms();
        print_status(philo->id, EAT);
        usleep(philo->time_to_eat * 1000);
        if (philo->required_meals > 0)
            philo->required_meals--;
        print_status(philo->id, SLEEP);
        pthread_mutex_unlock(&philo->mutex_fork);
        pthread_mutex_unlock(&philo->next->mutex_fork);
        usleep(philo->time_to_sleep * 1000);
        print_status(philo->id, THINK);
        philo->current_state = THINK;
    }
    return (NULL);
}

int	main(int argc, char **argv)
{
    struct philosopher	*head;
    int					num_philosophers;
    pthread_t			health_thread;

    if (check_arguments(argc, argv))
        return (1);
    head = initialize_philosophers(argc, argv);
    
    if (head == NULL)
        return (1);
    num_philosophers = ft_atoi(argv[1]);
    while (num_philosophers--)
    {
        pthread_mutex_init(&head->mutex_fork, NULL);
        pthread_create(&head->thread_id, NULL, &philosopher_routine, head);
        head = head->next;
    }
    pthread_create(&health_thread, NULL, &monitor_health, head);
    pthread_join(health_thread, NULL);
    num_philosophers = ft_atoi(argv[1]);
    while (num_philosophers--)
    {
        pthread_detach(head->thread_id);
        head = head->next;
    }
    free_philosophers(head);
}