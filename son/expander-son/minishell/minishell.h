#ifndef MINISHELL_H
# define MINISHELL_H

# define LESS 1
# define DLESS 2
# define PIPE 3
# define GREAT 4
# define DGREAT 5

typedef enum e_node_type {
	NODE_CMD,
	NODE_PIPE,
	NODE_REDIR_IN,
	NODE_REDIR_OUT,
	NODE_REDIR_APPEND,
	NODE_HEREDOC
}	t_node_type;

typedef struct s_ast {
	t_node_type		type;
	char			**args;     // sadece komut düğümlerinde
	char			*filename; // redirection varsa
	int 			is_expended;
	int 			was_quoted;
	struct s_ast	*left;
	struct s_ast	*right;
}	t_ast;

typedef struct s_lexer
{
    char *str;
    int token;
    int i;
	int is_expended;
	int was_quoted;
    struct s_lexer *next;
} t_lexer;

typedef struct s_env
{
    char *type;
    char *content;
    struct s_env *next;
} t_env;

typedef struct s_shell
{
    t_lexer *lex;
    t_env *env;
    t_ast  *ast;
} t_shell;

typedef struct s_garbage
{
	void *adress;
	struct s_garbage *next;
	int line;
} t_garbage;

t_ast *parse_tokens(t_lexer *tokens,t_garbage **garb);
void print_ast(t_ast *ast, int depth);
int	exec_cmd_node(t_ast *node, t_env *env,char **envir,t_garbage **garb);
int execute(t_ast *node, t_env *env, char **envir,t_garbage **garb);
int exec_pipe_node(t_ast *node, t_env *env, char **envir,t_garbage **garb);
int exec_redirect_node(t_ast *node, t_env *env,char **envir,t_garbage **garb);
int process_heredoc_priority(t_ast *node, t_env *env, t_garbage **garb);
char *get_env_type_for_cont(t_env *env, char *cont, int n);
char *mini_expend_dollar(t_env *env, char *src,t_garbage **garb);
char *get_env_cont_for_type(t_env *env, char *type, int n);
int builtin_export(char **cmd, t_env **env,t_garbage **garb);
int env_len(char *env, int t);
void env_cpy(char *type, char *content, const char *env);
void *g_collecter(t_garbage **garb, void *adress,int l_flag);
void g_free_l(t_garbage **garb);
void g_free(t_garbage **garb);

#endif