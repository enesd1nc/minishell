#include "minishell.h"
#include "libft/libft.h"
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>  // open
#include <stdio.h>  // perror
#include <stdlib.h> // exit
#include "gnl/get_next_line.h"
#include <readline/readline.h>
#include <readline/history.h>

int is_n_arg(char *arg)
{
    int i;

    i = 1;
    if (ft_strncmp("-n", arg, 2) != 0)
        return 0;
    while (arg[++i])
        if (arg[i] != 'n')
            return 0;
    return 1;
}

int builtin_echo(char **cmd)
{
    int i;
    int c;

    c = 0;
    i = 1;
    if(cmd[1] == NULL)
    {
        ft_putchar_fd('\n', 1);
        return 0;
    }
    while (is_n_arg(cmd[i]))
    {

        i++;
        c++;
    }
    while (cmd[i])
    {
        ft_putstr_fd(cmd[i], 1);
        if (cmd[++i])
            ft_putchar_fd(' ', 1);
    }
    if(!c)
        ft_putchar_fd('\n', 1);
    return 0;
}

int update_env(char *typ, char *new_cont, t_env **env,t_garbage **garb)
{
    t_env *tmp = *env;
    while (tmp)
    {
        if (ft_strncmp(tmp->type, typ, ft_strlen(typ)) == 0)
        {
            free(tmp->content);
            tmp->content = ft_strdup(new_cont);
            g_collecter(garb,tmp->content,0);
            if (!tmp->content)
                return 1;
            return 0;
        }
        tmp = tmp->next;
    }
    return 0;
}

int builtin_cd(char **cmd, t_env **env,t_garbage **garb)
{
    char *target = NULL;
    char *cwd = getcwd(NULL, 0);
    g_collecter(garb,cwd,1);
    static char *oldpwd = NULL;

    if (!cwd)
    {
        perror("cd: getcwd");
        return 1;
    }

    update_env("OLDPWD", cwd, env,garb); // Eski PWD güncellenir

    if (!cmd[1]) // cd → HOME
    {
        target = get_env_cont_for_type(*env, "HOME", 4);
        if (!target)
        {
            write(2, "cd: HOME not set\n", 17);
            free(cwd);
            return 1;
        }
    }
    else if (strcmp(cmd[1], "-") == 0) // cd -
    {
        if (!oldpwd)
        {
            write(2, "cd: OLDPWD not set\n", 20);
            free(cwd);
            return 1;
        }
        target = oldpwd;
        ft_putendl_fd(target, 1); // cd - çıktısı
    }
    else
        target = cmd[1]; // cd path

    if (chdir(target) != 0)
    {
        perror("cd");
        free(cwd);
        return 1;
    }

    // Yeni konumu al
    char *new_pwd = getcwd(NULL, 0);
    g_collecter(garb,new_pwd,1);
    if (new_pwd)
    {
        update_env("PWD", new_pwd, env,garb);
        free(new_pwd);
    }

    free(oldpwd);
    oldpwd = cwd; // eski cwd artık oldpwd olacak

    return 0;
}

int builtin_pwd(t_garbage **garb)
{
    char *cwd;

    cwd = getcwd(NULL, 0);
    g_collecter(garb,cwd,1);
    if (!cwd)
    {
        perror("pwd");
        return 1;
    }

    printf("%s\n", cwd);
    free(cwd);
    return 0;
}

int builtin_unset(char **cmd, t_env **env)
{
    t_env *iter = *env;
    t_env *prev = NULL;

    while (iter && strcmp(cmd[1], iter->type) != 0)
    {
        prev = iter;
        iter = iter->next;
    }

    if (!iter)
        return 0;
    if (prev)
        prev->next = iter->next; // Ortada ya da sonda
    else
        *env = iter->next; // İlk eleman

    free(iter->type);
    free(iter->content);
    free(iter);
    return 0;
}

int builtin_env(char **cmd, t_env *env)
{

    t_env *iter;

    if (cmd[1])
    {
        ft_putstr_fd("env argument error", 2);
        return 1;
    }
    iter = env;

    while (iter)
    {
        if (iter->content[0])
            printf("%s=%s\n", iter->type, iter->content);
        iter = iter->next;
    }
    return 0;
}

char **env_list_to_array(t_env *env,t_garbage **garb)
{
    int count = 0;
    t_env *tmp = env;

    while (tmp)
    {
        count++;
        tmp = tmp->next;
    }

    char **arr = malloc(sizeof(char *) * (count + 1));
    g_collecter(garb,(arr),1);
    if (!arr)
        return NULL;

    tmp = env;
    int i = 0;
    while (tmp)
    {
        int len;
        if (tmp->content[0] == '\0') // içerik boşsa sadece declare -x VAR
            len = strlen("declare -x ") + strlen(tmp->type) + 1;
        else // içerik varsa declare -x VAR="value"
            len = strlen("declare -x ") + strlen(tmp->type) + 2 + strlen(tmp->content) + 2;

        arr[i] = malloc(len);
        g_collecter(garb,arr[i],1);
        if (!arr[i])
            return NULL;

        arr[i][0] = '\0';
        strcat(arr[i], "declare -x ");
        strcat(arr[i], tmp->type);

        if (tmp->content[0] != '\0')
        {
            strcat(arr[i], "=\"");
            strcat(arr[i], tmp->content);
            strcat(arr[i], "\"");
        }

        i++;
        tmp = tmp->next;
    }

    arr[i] = NULL;
    return arr;
}

void sort_str_array(char **arr)
{
    int i, j;
    char *tmp;
    for (i = 0; arr[i]; i++)
    {
        for (j = i + 1; arr[j]; j++)
        {
            if (strcmp(arr[i], arr[j]) > 0)
            {
                tmp = arr[i];
                arr[i] = arr[j];
                arr[j] = tmp;
            }
        }
    }
}

int export_out(t_env *env,t_garbage **garb)
{
    char **arr = env_list_to_array(env,garb);
    if (!arr)
        return 1;

    sort_str_array(arr);

    for (int i = 0; arr[i]; i++)
    {
        if (ft_strncmp("declare -x _", arr[i], 12) != 0)
        {
            write(1, arr[i], strlen(arr[i]));
            write(1, "\n", 1);
        }
        free(arr[i]);
    }
    free(arr);

    return 0;
}

int env_remove(char *type, t_env **env)
{
    t_env *tmp = *env;
    t_env *prev = NULL;

    while (tmp)
    {
        if (strcmp(tmp->type, type) == 0)
        {
            if (prev)
                prev->next = tmp->next;
            else
                *env = tmp->next;

            free(tmp->type);
            free(tmp->content);
            free(tmp);
            return 0;
        }
        prev = tmp;
        tmp = tmp->next;
    }
    return 1; // bulunamadıysa
}

int env_add(char *type, char *content, t_env **env,t_garbage **garb)
{
    t_env *new = malloc(sizeof(t_env));
    g_collecter(garb,new,0);
    if (!new)
        return 1;

    new->type = type;
    new->content = content;
    new->next = NULL;

    if (!*env)
        *env = new;
    else
    {
        t_env *tmp = *env;
        while (tmp->next)
            tmp = tmp->next;
        tmp->next = new;
    }
    return 0;
}

char *get_env_value(t_env *env, char *key)
{
    while (env)
    {
        if (strcmp(env->type, key) == 0)
            return env->content;
        env = env->next;
    }
    return NULL;
}

int is_valid_export(char *str)
{
    int i;
    int l = -1;
    i = 0;

    // Eğer '=' varsa, sadece sol tarafı (değişken adı) kontrol et
    while (str[i] && str[i] != '=')
    {
        if (i == 0 && !(ft_isalpha(str[i]) || str[i] == '_'))
            return 0;
        else if (i > 0 && !(ft_isalnum(str[i]) || str[i] == '_'))
            return 0;
        i++;
    }

    // '=' baştaysa veya hiç karakter yoksa (örneğin `=val`) → geçersiz
    if (i == 0 && str[i] == '=')
        return 0;

    return 1;
}

int builtin_export(char **cmd, t_env **env,t_garbage **garb)
{
    int i = 1;
    int ret = 0;
    ;

    if (!cmd[1])
        return export_out(*env,garb);
    while (cmd[i])
    {
        if (!is_valid_export(cmd[i]))
        {
            ft_putstr_fd("export: \\=': not a valid identifier\n", 2);
            ret = 1;
        }
        else
        {
            char *equal = ft_strchr(cmd[i], '=');
            char *type;
            char *content;

            // export a  →  içerik yok
            if (!equal)
            {
                // varsa ekleme
                if (get_env_value(*env, cmd[i]) != NULL)
                {
                    i++;
                    continue;
                }

                type = strdup(cmd[i]);
                content = strdup(""); // boş içerik
                g_collecter(garb,type,1);
                g_collecter(garb,content,1);
            }
            else
            {
                type = malloc(env_len(cmd[i], 0));
                content = malloc(env_len(cmd[i], 1));
                g_collecter(garb,type,1);
                g_collecter(garb,content,1);
                if (!type || !content)
                    return 1;
                env_cpy(type, content, cmd[i]);
                env_remove(type, env); // varsa önce sil
            }

            env_add(type, content, env,garb);
        }

        i++;
    }

    return ret;
}

int exec_builtin(char **cmd, t_env *env,t_garbage **garb)
{
    if (strcmp(cmd[0], "echo") == 0)
        return (builtin_echo(cmd));
    else if (strcmp(cmd[0], "cd") == 0)
        return (builtin_cd(cmd, &env,garb));
    else if (strcmp(cmd[0], "pwd") == 0)
        return (builtin_pwd(garb));
    else if (strcmp(cmd[0], "export") == 0)
        return (builtin_export(cmd, &env,garb));
    else if (strcmp(cmd[0], "unset") == 0)
        return (builtin_unset(cmd, &env));
    else if (strcmp(cmd[0], "env") == 0)
        return (builtin_env(cmd, env));
    return (0);
}

int is_builtin(char *cmd)
{
    if (!cmd)
        return (0);

    if (strcmp(cmd, "echo") == 0 ||
        strcmp(cmd, "cd") == 0 ||
        strcmp(cmd, "pwd") == 0 ||
        strcmp(cmd, "export") == 0 ||
        strcmp(cmd, "unset") == 0 ||
        strcmp(cmd, "env") == 0 ||
        strcmp(cmd, "exit") == 0)
        return (1);

    return (0);
}

char *get_env_path(char **envp)
{
    for (int i = 0; envp[i]; i++)
    {
        if (strncmp(envp[i], "PATH=", 5) == 0)
            return envp[i] + 5;
    }
    return NULL;
}

char *find_command_path(char *cmd, char **envp,t_garbage **garb)
{
    if (strchr(cmd, '/'))
    {
        if (access(cmd, X_OK) == 0)
            g_collecter(garb,strdup(cmd),1);
        else
            return NULL;
    }

    char *path = get_env_path(envp);
    if (!path)
        return NULL;

    char *path_copy = strdup(path);
    g_collecter(garb,path_copy,1);
    char *dir = strtok(path_copy, ":");

    while (dir)
    {
        size_t len = strlen(dir) + strlen(cmd) + 2; // '/' + '\0'
        char *fullpath = malloc(len);
        g_collecter(garb,fullpath,1);
        if (!fullpath)
        {
            free(path_copy);
            return NULL;
        }

        // Elle birleştir: fullpath = dir + "/" + cmd
        strcpy(fullpath, dir);
        strcat(fullpath, "/");
        strcat(fullpath, cmd);

        if (access(fullpath, X_OK) == 0)
        {
            free(path_copy);
            return fullpath; // bulundu
        }

        free(fullpath);
        dir = strtok(NULL, ":");
    }

    free(path_copy);
    return NULL; // bulunamadı
}

int exec_cmd_node(t_ast *node, t_env *env, char **envir,t_garbage **garb)
{
    if (!node || !node->args)
        return (0);
    if (is_builtin(node->args[0]))
        return (exec_builtin(node->args, env,garb));

    pid_t pid = fork();
    if (pid == -1)
    {
        perror("fork");
        return 0;
    }

    if (pid == 0)
    {
        // Çocuk süreç: execve çağırılır
        execve(find_command_path(node->args[0], envir,garb), node->args, envir);
        perror("execve"); // exec başarısızsa burası çalışır
        exit(1);
    }
    else
    {
        // Ebeveyn süreç: Çocuğu bekle
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status))
            return WEXITSTATUS(status);
        else if (WIFSIGNALED(status))
            return 128 + WTERMSIG(status); // sinyalle öldüyse
        return 1;                          // diğer durumlar (çok nadir)
    }
}

// Heredoc işlemini öncelikli olarak yapan yardımcı fonksiyon
int process_heredoc_priority(t_ast *node, t_env *env, t_garbage **garb)
{
    if (!node)
        return 0;
    
    // Eğer bu bir heredoc düğümüyse, işle
    if (node->type == NODE_HEREDOC)
    {
        int pipefd[2];
        char *txt;

        if (pipe(pipefd) == -1)
        {
            perror("pipe");
            return 1;
        }

        while (1)
        {
            txt = readline("> ");
            if (!txt)
                break;
            if (ft_strcmp(txt, node->filename) == 0)
            {
                free(txt);
                break;
            }
            if (node->was_quoted != 6 && node->was_quoted != 7) // tırnaklı değilse expand
            {
                char *expanded = mini_expend_dollar(env, txt, garb);
                free(txt);
                txt = expanded;
            }
            txt = ft_strdup(txt);
            g_collecter(garb, txt, 1);
            write(pipefd[1], txt, ft_strlen(txt));
            write(pipefd[1], "\n", 1); // satır sonu ekle
            free(txt);
        }

        close(pipefd[1]);
        
        // Heredoc verilerini geçici dosyaya yaz
        int temp_fd = open("/tmp/heredoc_temp", O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (temp_fd == -1)
        {
            perror("open temp file");
            close(pipefd[0]);
            return 1;
        }
        
        char buffer[1024];
        ssize_t bytes_read;
        while ((bytes_read = read(pipefd[0], buffer, sizeof(buffer))) > 0)
        {
            write(temp_fd, buffer, bytes_read);
        }
        
        close(pipefd[0]);
        close(temp_fd);
        
        return 0;
    }
    
    // Alt düğümlerde heredoc ara
    int left_result = process_heredoc_priority(node->left, env, garb);
    int right_result = process_heredoc_priority(node->right, env, garb);
    
    return left_result || right_result;
}

int exec_pipe_node(t_ast *node, t_env *env, char **envir,t_garbage **garb)
{
    // Önce heredoc işlemlerini yap
    process_heredoc_priority(node, env, garb);
    
    int pipefd[2];
    pid_t pid1;
    pid_t pid2;
    int status;

    if (pipe(pipefd) == -1)
    {
        perror("pipe");
        return (1);
    }

    pid1 = fork();
    if (pid1 < 0)
    {
        perror("fork (left)");
        close(pipefd[0]);
        close(pipefd[1]);
        return (1);
    }
    if (pid1 == 0)
    {
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);
        exit(execute(node->left, env, envir,garb));
    }

    pid2 = fork();
    if (pid2 < 0)
    {
        perror("fork (right)");
        close(pipefd[0]);
        close(pipefd[1]);
        return (1);
    }
    if (pid2 == 0)
    {
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);
        exit(execute(node->right, env, envir,garb));
    }
    close(pipefd[0]);
    close(pipefd[1]);

    waitpid(pid1, NULL, 0);
    waitpid(pid2, &status, 0);
    if (WIFEXITED(status))
        return WEXITSTATUS(status);
    else if (WIFSIGNALED(status))
        return 128 + WTERMSIG(status);
    return (1);
}

int exec_redirect_out(t_ast *node, t_env *env, char **envir, int append,t_garbage **garb)
{
    int fd;
    int flags;

    if (!node || !node->filename || !node->left)
        return (1);

    // Eğer sol alt düğüm de bir redirection ise, önce tüm dosyaları oluştur
    t_ast *current = node;
    while (current && (current->type == NODE_REDIR_OUT || current->type == NODE_REDIR_APPEND))
    {
        int temp_flags = O_CREAT | O_WRONLY;
        if (current->type == NODE_REDIR_APPEND)
            temp_flags |= O_APPEND;
        else
            temp_flags |= O_TRUNC;
        
        int temp_fd = open(current->filename, temp_flags, 0644);
        if (temp_fd >= 0)
            close(temp_fd);
        
        current = current->left;
    }

    // Komut düğümünü bul
    t_ast *cmd_node = node->left;
    while (cmd_node && (cmd_node->type == NODE_REDIR_OUT || cmd_node->type == NODE_REDIR_APPEND))
    {
        cmd_node = cmd_node->left;
    }

    if (!cmd_node)
        return 1;

    flags = O_CREAT | O_WRONLY;
    if (append)
        flags |= O_APPEND;
    else
        flags |= O_TRUNC;

    fd = open(node->filename, flags, 0644);
    if (fd < 0)
    {
        perror("open (redirect out)");
        return (0);
    }

    pid_t pid = fork();
    if (pid < 0)
    {
        perror("fork");
        close(fd);
        return (1);
    }

    if (pid == 0)
    {
        dup2(fd, STDOUT_FILENO);
        close(fd);
        exit(execute(cmd_node, env, envir,garb));
    }

    close(fd);
    int status;
    waitpid(pid, &status, 0);
    if (WIFEXITED(status))
        return WEXITSTATUS(status);
    else if (WIFSIGNALED(status))
        return 128 + WTERMSIG(status); // sinyalle öldüyse
    return 1;                          // diğer durumlar (çok nadir)
}

int exec_redirect_in(t_ast *node, t_env *env, char **envir,t_garbage **garb)
{
    int fd;

    if (!node || !node->filename || !node->left)
        return (1); // geçersiz AST

    // Dosyayı sadece okumak için aç
    fd = open(node->filename, O_RDONLY);
    if (fd < 0)
    {
        write(2, "minishell: no such file or directory: ", 39);
        write(2, node->filename, ft_strlen(node->filename));
        write(2, "\n", 1);
        return (1);
    }

    pid_t pid = fork();
    if (pid < 0)
    {
        perror("fork");
        close(fd);
        return (1);
    }

    if (pid == 0)
    {
        // Çocuk süreç: stdin'i dosyaya yönlendir
        dup2(fd, STDIN_FILENO);
        close(fd);
        exit(execute(node->left, env, envir,garb));
    }

    // Ebeveyn süreç
    close(fd);
    int status;
    waitpid(pid, &status, 0);
    if (WIFEXITED(status))
        return WEXITSTATUS(status);
    else if (WIFSIGNALED(status))
        return 128 + WTERMSIG(status); // sinyalle öldüyse
    return 1;                          // diğer durumlar (çok nadir)
    return 1;
}

int exec_redirect_node(t_ast *node, t_env *env, char **envir,t_garbage **garb)
{
    if (node->type == NODE_REDIR_OUT)
        return exec_redirect_out(node, env, envir, 0,garb);
    else if (node->type == NODE_REDIR_IN)
        return exec_redirect_in(node, env, envir,garb);
    else if (node->type == NODE_REDIR_APPEND)
        return exec_redirect_out(node, env, envir, 1,garb);

    return 0;
}

int exec_heredoc_node(t_ast *node, t_env *env, char **envir,t_garbage **garb)
{
    if (!node || !node->filename || !node->left)
        return (1);

    // Geçici dosyadan oku
    int temp_fd = open("/tmp/heredoc_temp", O_RDONLY);
    if (temp_fd == -1)
    {
        perror("open temp file for reading");
        return 1;
    }

    pid_t pid = fork();
    if (pid == 0)
    {
        dup2(temp_fd, STDIN_FILENO);
        close(temp_fd);
        exit(execute(node->left, env, envir,garb));
    }

    close(temp_fd);
    int status;
    waitpid(pid, &status, 0);
    if (WIFEXITED(status))
        return WEXITSTATUS(status);
    else if (WIFSIGNALED(status))
        return 128 + WTERMSIG(status); // sinyalle öldüyse
    return 1;                          // diğer durumlar (çok nadir)
}

int execute(t_ast *node, t_env *env, char **envir,t_garbage **garb)
{
    if (!node)
        return (0);
    
    // Eğer heredoc varsa önce işle
    if (node->type == NODE_HEREDOC)
    {
        process_heredoc_priority(node, env, garb);
        return exec_heredoc_node(node, env, envir, garb);
    }
    
    if (node->type == NODE_CMD)
        return exec_cmd_node(node, env, envir,garb);
    else if (node->type == NODE_PIPE)
        return exec_pipe_node(node, env, envir,garb);
    else if (node->type == NODE_REDIR_IN || node->type == NODE_REDIR_OUT)
        return exec_redirect_node(node, env, envir,garb);
    else if (node->type == NODE_REDIR_APPEND)
        return exec_redirect_node(node, env, envir,garb);
    return (0);
}
