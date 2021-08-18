#define _POSIX_SOURCE
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>

#ifdef THREAD
#include <pthread.h>
#endif

#define PIPE_BUF 1048576
#define LINE_MAX 1024
#define CHILD_MAX 32

struct mpipe
{
    int pipe_data[CHILD_MAX][2];
    int pipe_sorted[CHILD_MAX][2];
    int size;
    int current;
};
struct mpipe g_mpipe;

void sys_error(char *message);
char *my_strdup(char *source);
void create_pipes(int size);
void close_data_pipes(int io);
void close_sorted_pipes(int io);
void next_pipe();
void write_to_pipe(char *s);
void string_sanitize(char *s);
void string_tokenize(char *s);
void sys_sort(int fd_in, int fd_out);

#ifdef THREAD
void *child(void *args);
#else
void child(void *args);
#endif

void wait_child(pid_t pid_arr[], int size);
void read_input();
char **get_word_list(int fd);
char **merge(char **word_list1, char **word_list2);
void print_word_list(char **word_list);
int count_words(char **word_list);
void print_remove_duplicate(char **word_list, int print_count);

int main(int argc, char *argv[])
{
    int i, child_num, print_count = 0;
    if (argc < 2)
    {
        printf("./msort k [-c]\n");
        return EXIT_FAILURE;
    }
    if (argc == 3 && strcmp(argv[2], "-c") == 0)
    {
        print_count = 1;
    }

    child_num = atoi(argv[1]);

    if (child_num > CHILD_MAX)
    {
        child_num = CHILD_MAX;
    }
    create_pipes(child_num);
    read_input();

#ifdef THREAD
    //version threads
    pthread_t thread_arr[CHILD_MAX];
    for (i = 0; i < child_num; ++i)
    {
        int index = i;
        if (pthread_create(thread_arr + i, NULL, child, (void *)(&index)))
        {
            sys_error("pthread_create");
        }
        usleep(1);
    }
    for (i = 0; i < child_num; ++i)
    {
        pthread_join(thread_arr[i], NULL);
    }

#else
    //version processes
    int pid_arr[CHILD_MAX];
    for (i = 0; i < child_num; ++i)
    {
        pid_t pid = fork();
        if (pid == -1)
        {
            sys_error("fork");
        }
        if (pid == 0)
        {
            int index = i;
            child((void *)(&index));
            return EXIT_SUCCESS;
        }
        else
        {
            pid_arr[i] = pid;
        }
    }
    wait_child(pid_arr, child_num);

#endif

    close_sorted_pipes(1);

    char ***dic = (char ***)malloc(sizeof(char **) * child_num);
    for (i = 0; i < child_num; ++i)
    {
        dic[i] = get_word_list(g_mpipe.pipe_sorted[i][0]);
    }

    char **full_list = dic[0];
    for (i = 1; i < child_num; ++i)
    {
        full_list = merge(full_list, dic[i]);
    }
    print_remove_duplicate(full_list, print_count);

    return EXIT_SUCCESS;
}

void sys_error(char *message)
{
    perror(message);
    exit(EXIT_FAILURE);
}

char *my_strdup(char *source)
{
    char *res = (char *)malloc(strlen(source) + 1);
    strcpy(res, source);
    return res;
}

void create_pipes(int size)
{
    int i;
    g_mpipe.size = size;
    g_mpipe.current = 0;
    for (i = 0; i < size; ++i)
    {
        if (pipe(g_mpipe.pipe_data[i]) == -1 || pipe(g_mpipe.pipe_sorted[i]) == -1)
        {
            sys_error("pipe");
        }
        fcntl(g_mpipe.pipe_data[i][1], F_SETPIPE_SZ, PIPE_BUF);
        fcntl(g_mpipe.pipe_sorted[i][1], F_SETPIPE_SZ, PIPE_BUF);
    }
}

void close_data_pipes(int io)
{
    int i;
    for (i = 0; i < g_mpipe.size; ++i)
    {
        close(g_mpipe.pipe_data[i][io]);
    }
}

void close_sorted_pipes(int io)
{
    int i;
    for (i = 0; i < g_mpipe.size; ++i)
    {
        close(g_mpipe.pipe_sorted[i][io]);
    }
}

void next_pipe()
{
    g_mpipe.current++;
    if (g_mpipe.current == g_mpipe.size)
    {
        g_mpipe.current = 0;
    }
}

void write_to_pipe(char *s)
{
    char buf[64];
    int length = sprintf(buf, "%s\n", s);
    if (write(g_mpipe.pipe_data[g_mpipe.current][1], buf, length) == -1)
    {
        sys_error("write");
    }
    next_pipe();
}

void string_sanitize(char *s)
{
    char *ptr = s;
    while (*ptr != '\0')
    {
        *ptr = isalpha(*ptr) ? tolower(*ptr) : '\n';
        ptr++;
    }
}

void string_tokenize(char *s)
{
    char *begin = NULL;
    char *end = s;
    while (*end)
    {
        if (*end == '\n')
        {
            if (begin)
            {
                *end = '\0';
                write_to_pipe(begin);
                begin = NULL;
            }
        }
        else
        {
            if (begin == NULL)
            {
                begin = end;
            }
        }
        end++;
    }
    if (begin)
    {
        write_to_pipe(begin);
    }
}

void sys_sort(int fd_in, int fd_out)
{
    if (dup2(fd_in, STDIN_FILENO) == -1 || dup2(fd_out, STDOUT_FILENO) == -1)
    {
        sys_error("dup2");
    }
    close(fd_in);
    close(fd_out);
    execlp("sort", "sort", (char *)NULL);
    sys_error("execlp");
}

#ifdef THREAD
void *child(void *args)
#else
void child(void *args)
#endif
{
    int index = *((int *)args);
    int fd_in = g_mpipe.pipe_data[index][0];
    int fd_out = g_mpipe.pipe_sorted[index][1];

    int sort_pipe[2];
    if (pipe(sort_pipe) == -1)
    {
        sys_error("pipe");
    }
    fcntl(sort_pipe[1], F_SETPIPE_SZ, PIPE_BUF);

    pid_t pid = fork();
    if (pid == -1)
    {
        sys_error("fork");
    }
    else if (pid == 0)
    {
        close(sort_pipe[0]);
        sys_sort(fd_in, sort_pipe[1]);
    }
    else
    {
        close(fd_in);
        close(sort_pipe[1]);
        char line[1024];
        ssize_t nbytes;
        while ((nbytes = read(sort_pipe[0], line, 1024)) > 0)
        {
            if (write(fd_out, line, nbytes) == -1)
            {
                sys_error("write");
            }
        }
        close(sort_pipe[0]);
        wait(NULL);
        close(fd_out);
        close(fd_in);
    }

#ifdef THREAD
    return NULL;
#endif
}

void wait_child(pid_t pid_arr[], int size)
{
    int i, ret, finished = size;
    while (finished)
    {
        for (i = 0; i < size; ++i)
        {
            if (pid_arr[i] > 0)
            {
                ret = waitpid(pid_arr[i], NULL, WNOHANG);
                if (ret == -1)
                {
                    sys_error("waitpid");
                }

                if (ret > 0)
                {
                    pid_arr[i] = 0;
                    finished--;
                }
            }
        }
    }
}

void read_input()
{
    static char line[LINE_MAX];
    while (fgets(line, LINE_MAX, stdin) != NULL)
    {
        string_sanitize(line);
        string_tokenize(line);
    }
    close_data_pipes(1);
}

char **merge(char **word_list1, char **word_list2)
{
    int count1 = count_words(word_list1);
    int count2 = count_words(word_list2);

    char **result = (char **)malloc(sizeof(char *) * (count1 + count2 + 1));

    char **ptr1 = word_list1;
    char **ptr2 = word_list2;
    char **ptr_result = result;

    while (*ptr1 && *ptr2)
    {
        if (strcmp(*ptr1, *ptr2) < 0)
        {
            *ptr_result = *ptr1;
            ptr_result++, ptr1++;
        }
        else
        {
            *ptr_result = *ptr2;
            ptr_result++, ptr2++;
        }
    }
    while (*ptr1)
    {
        *ptr_result = *ptr1;
        ptr_result++, ptr1++;
    }
    while (*ptr2)
    {
        *ptr_result = *ptr2;
        ptr_result++, ptr2++;
    }
    *ptr_result = NULL;
    return result;
}

char **get_word_list(int fd)
{
    int size = 0;
    char **word_list = NULL;

    char word[64];
    FILE *fin = fdopen(fd, "r");

    while (fgets(word, 64, fin) != NULL)
    {
        word[strlen(word) - 1] = '\0';
        size++;
        word_list = (char **)realloc(word_list, sizeof(char *) * size);
        word_list[size - 1] = my_strdup(word);
    }

    size++;
    word_list = (char **)realloc(word_list, sizeof(char *) * size);
    word_list[size - 1] = NULL;
    return word_list;
}

void print_word_list(char **word_list)
{
    int i = 1;
    char **ptr = word_list;
    while (*ptr)
    {
        printf("%d: %s\n", i, *ptr);
        ptr++;
        i++;
    }
}

void print_remove_duplicate(char **word_list, int print_count)
{
    char **ptr = word_list;
    char *current = *ptr;
    int count = 0;

    while (*ptr)
    {
        if (strcmp(*ptr, current) == 0)
        {
            count++;
        }
        else
        {
            if (print_count)
            {
                printf("%7d %s\n", count, current);
            }
            else
            {
                printf("%s\n", current);
            }
            current = *ptr;
            count = 1;
        }
        ptr++;
    }

    if (print_count)
    {
        printf("%7d %s\n", count, current);
    }
    else
    {
        printf("%s\n", current);
    }
}

int count_words(char **word_list)
{
    int count = 0;
    char **ptr = word_list;
    while (*ptr)
    {
        count++;
        ptr++;
    }
    return count;
}
