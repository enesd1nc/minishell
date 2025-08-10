#include "minishell.h"
#include <stdlib.h>
#include <stdio.h>
#include "libft/libft.h"

// Debug fonksiyonu: AST ağacını yazdırır
void print_ast(t_ast *ast, int depth)
{
    if (!ast)
        return;
    
    // Girinti oluştur
    for (int i = 0; i < depth; i++)
        printf("  ");
    
    // Düğüm tipini yazdır
    switch (ast->type)
    {
        case NODE_CMD:
            printf("[CMD");
            if (ast->args)
            {
                for (int i = 0; ast->args[i]; i++)
                    printf(" '%s'", ast->args[i]);
            }
            printf("]\n");
            break;
        case NODE_PIPE:
            printf("[PIPE]\n");
            break;
        case NODE_REDIR_IN:
            printf("[REDIR_IN '%s']\n", ast->filename);
            break;
        case NODE_REDIR_OUT:
            printf("[REDIR_OUT '%s']\n", ast->filename);
            break;
        case NODE_REDIR_APPEND:
            printf("[REDIR_APPEND '%s']\n", ast->filename);
            break;
        case NODE_HEREDOC:
            printf("[HEREDOC '%s']\n", ast->filename);
            break;
    }
    
    // Alt düğümleri yazdır
    print_ast(ast->left, depth + 1);
    print_ast(ast->right, depth + 1);
}

// Yeni bir AST düğümü oluşturur
t_ast *create_ast_node(t_node_type type,t_garbage **garb)
{
    t_ast *node = (t_ast *)malloc(sizeof(t_ast));
    g_collecter(garb,node,1);
    if (!node)
        return NULL;
    
    node->type = type;
    node->args = NULL;
    node->filename = NULL;
    node->left = NULL;
    node->right = NULL;
    node->is_expended =0;
    node->was_quoted = 0;
    
    return node;
}

// Bir komut düğümü oluşturur ve argümanlarını ekler
t_ast *create_cmd_node(t_lexer **tokens,t_garbage **garb)
{
    t_ast *node;
    t_lexer *current;
    int arg_count = 0;
    int i;
    
    // Argüman sayısını say
    current = *tokens;
    while (current && current->token == 0) // TOKEN_WORD
    {
        arg_count++;
        current = current->next;
    }
    
    if (arg_count == 0)
        return NULL;
    
    // Komut düğümü oluştur
    node = create_ast_node(NODE_CMD,garb);
    if (!node)
        return NULL;
    
    // Argümanlar için bellek ayır
    node->args = (char **)malloc(sizeof(char *) * (arg_count + 1));
    g_collecter(garb,node->args,1);
    if (!node->args)
    {
        free(node);
        return NULL;
    }
    
    // Argümanları kopyala
    i = 0;
    current = *tokens;
    while (i < arg_count)
    {
        node->args[i] = ft_strdup(current->str);
        g_collecter(garb,node->args[i],1);
        current = current->next;
        i++;
    }
    node->args[i] = NULL; // Son argüman NULL ile sonlandır
    
    // Token listesinde ilerleme
    *tokens = current;
    
    return node;
}

// Bir yönlendirme düğümü oluşturur
t_ast *create_redir_node(t_node_type type, t_lexer **tokens,t_garbage **garb)
{
    t_ast *node;
    
    // Dosya adı için token kontrolü
    if (!(*tokens) || !(*tokens)->next || (*tokens)->next->token != 0) // TOKEN_WORD değilse
    {
        printf("Parser error: expected filename after redirection\n");
        // Hata durumunda token'ı ilerlet (sonsuz döngüyü önlemek için)
        if (*tokens)
            *tokens = (*tokens)->next;
        return NULL;
    }
    
    // Yönlendirme düğümü oluştur
    node = create_ast_node(type,garb);
    if (!node)
        return NULL;
    
    // Yönlendirme token'ını atla
    *tokens = (*tokens)->next;
    
    // Dosya adını kopyala
    node->filename = ft_strdup((*tokens)->str);
    g_collecter(garb,node->filename,1);
    node->is_expended = (*tokens)->is_expended;
    node->was_quoted = (*tokens)->was_quoted;
    // Dosya adı token'ını atla
    *tokens = (*tokens)->next;
    
    return node;
}

// Bir komut bloğu parse eder (komut ve ilişkili yönlendirmeler)
t_ast *parse_command(t_lexer **tokens,t_garbage **garb)
{
    t_ast *cmd_node = NULL;
    t_ast *redir_node = NULL;
    t_ast *current = NULL;
    
    // Eğer token yoksa NULL döndür
    if (!(*tokens))
        return NULL;
    
    // İlk olarak komut düğümünü oluştur
    if ((*tokens)->token == 0) // TOKEN_WORD
    {
        cmd_node = create_cmd_node(tokens,garb);
        if (!cmd_node)
            return NULL;
        
        current = cmd_node;
    }
    
    // Komuttan sonraki yönlendirmeleri işle
    while (*tokens && (*tokens)->token != PIPE)
    {
        switch ((*tokens)->token)
        {
            case LESS: // <
                redir_node = create_redir_node(NODE_REDIR_IN, tokens,garb);
                break;
            case GREAT: // >
                redir_node = create_redir_node(NODE_REDIR_OUT, tokens,garb);
                break;
            case DGREAT: // >>
                redir_node = create_redir_node(NODE_REDIR_APPEND, tokens,garb);
                break;
            case DLESS: // <<
                redir_node = create_redir_node(NODE_HEREDOC, tokens,garb);
                break;
            case 0: // TOKEN_WORD - komutun devamı
                if (!cmd_node)
                {
                    cmd_node = create_cmd_node(tokens,garb);
                    current = cmd_node;
                }
                else
                {
                    // Komut zaten var, bu bir hata olabilir
                    printf("Parser error: unexpected word token\n");
                    return NULL;
                }
                break;
            default:
                printf("Parser error: unexpected token\n");
                return NULL;
        }
        
        // Yönlendirme düğümü oluşturulduysa
        if (redir_node)
        {
            if (cmd_node)
            {
                // Komut düğümü sol alt düğüm olur
                redir_node->left = cmd_node;
                cmd_node = redir_node;
            }
            else
            {
                cmd_node = redir_node;
            }
            redir_node = NULL;
        }
    }
    
    return cmd_node;
}

// Recursive olarak tam bir AST oluşturur
t_ast *parse_pipeline(t_lexer **tokens,t_garbage **garb)
{
	t_ast *left;
    t_ast *pipe_node;
    
    // Sol tarafı parse et (ilk komut veya komut bloğu)
    left = parse_command(tokens,garb);
    if (!left)
        return NULL;
    
    // Pipe token'ı yoksa tek komut döndür
    if (!(*tokens) || (*tokens)->token != PIPE)
        return left;
    
    // Pipe token'ını atla
    *tokens = (*tokens)->next;
    
    // Pipe düğümü oluştur
    pipe_node = create_ast_node(NODE_PIPE,garb);
    if (!pipe_node)
        return NULL;
    
    // Sol tarafı ayarla
    pipe_node->left = left;
    
    // Sağ tarafı recursive olarak parse et
    pipe_node->right = parse_pipeline(tokens,garb);
    if (!pipe_node->right)
    {
        printf("Parser error: expected command after pipe\n");
        return NULL;
    }
    
    return pipe_node;
}

// Ana parse fonksiyonu
t_ast *parse_tokens(t_lexer *tokens,t_garbage **garb)
{
    if (!tokens)
        return NULL;
    
    return parse_pipeline(&tokens,garb);
}

// AST'yi temizle (free)
void free_ast(t_ast *ast)
{
    if (!ast)
        return;
    
    // Alt düğümleri temizle
    free_ast(ast->left);
    free_ast(ast->right);
    
    // Argümanları temizle
    if (ast->args)
    {
        for (int i = 0; ast->args[i]; i++)
            free(ast->args[i]);
        free(ast->args);
    }
    
    // Dosya adını temizle
    if (ast->filename)
        free(ast->filename);
    
    // Düğümü temizle
    free(ast);
}