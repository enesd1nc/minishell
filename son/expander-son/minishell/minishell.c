#include "minishell.h"
#include <stdlib.h>
#include <stdio.h>
#include "libft/libft.h"
#include <readline/readline.h>
#include <readline/history.h>

// Bir karakterin boşluk karakteri (whitespace) olup olmadığını kontrol eder
// Boşluk karakteri ise 1, değilse 0 döndürür
int is_ws(char c)
{
    return (c == ' ' || (c <= '\r' && c >= '\t'));
}

// Komutta tırnak işaretlerinin doğru bir şekilde kapatılıp kapatılmadığını kontrol eder
// Tüm tırnaklar doğru şekilde kapatılmışsa 1, aksi halde 0 döndürür
int quote_check(const char *cmd)
{
    int i;
    char quote;

    i = 0;
    quote = 0;
    while (cmd[i])
    {
        if (cmd[i] == '\\' && quote != '\'')
        {
            i++;
            if (cmd[i])
                i++;
            continue;
        }
        else if (!quote && (cmd[i] == '\'' || cmd[i] == '"'))
            quote = cmd[i];
        else if (cmd[i] == quote)
            quote = 0;

        i++;
    }
    return (quote == 0);
}

// Çevre değişkenlerinden belirli bir türün içeriğini getirir
// env: Çevre değişkenleri listesi
// type: Aranacak değişken adı
// n: Karşılaştırılacak karakter sayısı
char *get_env_cont_for_type(t_env *env, char *type, int n)
{
    t_env *iter;

    iter = env;
    while (iter)
    {
        if (ft_strncmp(iter->type, type, n) == 0 && iter->type[n] == 0)
            return (iter->content);
        iter = iter->next;
    }
    return NULL; // Değişken bulunamazsa NULL döndürür
}

char *get_env_type_for_cont(t_env *env, char *cont, int n)
{
    t_env *iter;

    iter = env;
    while (iter)
    {
        if (ft_strncmp(iter->content, cont, n) == 0 && iter->content[n] == 0)
            return (iter->type);
        iter = iter->next;
    }
    return NULL; // Değişken bulunamazsa NULL döndürür
}

// Tilda (~) karakteri için gerekli uzunluğu hesaplar (HOME dizini)
void tilda_len(char *str, int *l, int *i, t_env *env)
{
    *l += ft_strlen(get_env_cont_for_type(env, "HOME", 4));
}

// Çevre değişkeni için gerekli uzunluğu hesaplar ($VAR gibi ifadeler için)
void envir_len(char *str, int *i, int *l, t_env *env)
{
    int k;

    k = 0;
    // Değişken adının uzunluğunu bul (alfanümerik karakterler ve alt çizgi)
    while (str[*i + k + 1] && (ft_isalnum(str[*i + k + 1]) || str[*i + k + 1] == '_'))
        k++;
    // Değişken varsa, değerinin uzunluğunu ekle
    if (ft_strlen(get_env_cont_for_type(env, str + *i + 1, k)))
        *l += ft_strlen(get_env_cont_for_type(env, str + *i + 1, k));
    else
        *l += k + 1; // Değişken yoksa, değişken adının uzunluğunu ekle
    *i += k;         // İndeksi ilerlet
}

int quote_count(char *str)
{
    int count = 0;
    char quote = 0; // Aktif tırnak (0: yok, ' veya ")
    
    while (*str)
    {
        if ((*str == '\'' || *str == '"'))
        {
            if (quote == 0) // Yeni tırnak açılıyor
            {
                quote = *str;
            }
            else if (quote == *str) // Aynı tırnak kapanıyor
            {
                count++;
                quote = 0;
            }
        }
        str++;
    }
    return count;
}

// Bir komut parçasının (token) genişletilmiş halinin uzunluğunu hesaplar
// Tilda (~) ve çevre değişkenlerini ($VAR) dikkate alır
int get_len(char *str, int i, t_env *env,int old_token)
{
    int l;  // Uzunluk
    int q;  // Tırnak durumu
    int quote_count_local = 0;  // Bu token için tırnak sayısı

    l = 0;
    q = 0;
    while(!(is_ws(str[i])) && str[i])
    {
        // Kaçış karakteri (\) kontrolü
        if (str[i] == '\\' && str[i + 1])
        {
            l++; // Kaçırılan karakter için uzunluk ekle
            i += 2; // Hem \ hem de sonraki karakteri atla
            continue;
        }
        // Tırnak işareti kontrolü
        if(!q && str[i] == '\'')
        {
            q = str[i];  // Tırnak başlangıcı
            l++; // Count the opening quote
            i++;
            while (str[i] && str[i] != q)
            {
                l++;
                i++;
            }
            if (str[i] == q) // Count the closing quote
            {
                l++;
                i++;
                quote_count_local++; // Tamamlanan tırnak çifti
            }
            q = 0; // Reset quote state
            continue;
        }
            
        // Tilda (~) işareti kontrolü - HOME dizini ile değiştirilecek
        if(str[i] == '~' && (str[i + 1] == '/' || is_ws(str[i + 1])) && (i == 0 || is_ws(str[i - 1])) && !q && old_token != 2)
            tilda_len(str, &l, &i, env);
        // Çevre değişkeni ($VAR) kontrolü
        else if(str[i] == '$' && (!is_ws(str[i+1]) && q != '\'') && (ft_isalnum(str[i+1]) || str[i+1] == '_') && old_token != 2)
            envir_len(str, &i, &l, env);
        // Normal karakter
        else
            l++;
        i++;
    }
    return l - quote_count_local * 2;  // Toplam uzunluğu döndür
}

// Tilda (~) işaretini HOME dizini ile değiştirir
void expend_tilda(char *dst, int *l, int *i, t_env *env, int *is_expended)
{
    char *src;
    int j;

    j = 0;
    src = get_env_cont_for_type(env, "HOME", 4);
    if (src)
    {
        *is_expended = 1;
        while (src[j])
        {
            dst[*l] = src[j++];
            *l += 1;
        }
    }
}

// Çevre değişkeni ($VAR) işaretini değişkenin değeri ile değiştirir
void expend_dollar(char *dst, int *l, int *i, t_env *env, char *src, int *is_expended)
{
    int k = 0;
    int j;
    char *str;

    // Değişken adının uzunluğunu bul
    while (src[*i + k + 1] && (ft_isalnum(src[*i + k + 1]) || src[*i + k + 1] == '_'))
        k++;

    // Değişkenin değerini al
    str = get_env_cont_for_type(env, src + *i + 1, k);

    if (str)
    {
        *is_expended = 1;
        j = -1;
        while (str[++j])
        {
            dst[(*l)++] = str[j];
        }
    }
    else
    {
        *is_expended = 0;
        dst[(*l)++] = '$';
        for (int n = 0; n < k; n++)
            dst[(*l)++] = src[*i + 1 + n];
    }

    *i += k; // $ ve değişken adı kadar ilerlet
}

int mini_expend_dollar_len(t_env *env, char *src)
{
    int i;

    i = 0;
    int k;
    k = 0;
    int l;
    l = 0;
    while (src[i] && src[i] != '$')
        i++;
    while (src[i + k + 1] && (ft_isalnum(src[i + k + 1]) || src[i + k + 1] == '_'))
        k++;
    l += ft_strlen(get_env_cont_for_type(env, src + i + 1, k));
    while (src[i + k + 2])
        i++;
    return (i + l);
}

char *mini_expend_dollar(t_env *env, char *src,t_garbage **garb)
{
    int i;
    char *str;

    i = -1;
    int k;
    int l;
    char *cnt;
    str = malloc(mini_expend_dollar_len(env, src) + 1);
    g_collecter(garb,str,1);
    while (src[++i] && src[i] != '$')
        str[i] = src[i];
    k = 0;
    while (src[i + k + 1] && (ft_isalnum(src[i + k + 1]) || src[i + k + 1] == '_'))
        k++;
    cnt = get_env_cont_for_type(env, src + i + 1, k);
    l = -1;
    if (cnt)
        while (cnt[++l])
            str[i + l] = cnt[l];
    while (src[++i + k])
        str[l + i] = src[i + 1 + k];
    str[l + i] = 0;
    return str;
}

// Bir tokeni türüne göre sınıflandırır
// 0: normal komut/argüman, 1: <, 2: <<, 3: |, 4: >, 5: >>
int token(char *str)
{
    if (str[0] == '<' && str[1] == 0)
        return 1; // Giriş yönlendirme (<)
    else if (str[0] == '<' && str[1] == '<' && str[2] == 0)
        return 2; // Here-doc (<<)
    else if (str[0] == '|' && str[1] == 0)
        return 3; // Pipe (|)
    else if (str[0] == '>' && str[1] == 0)
        return 4; // Çıkış yönlendirme (>)
    else if (str[0] == '>' && str[1] == '>' && str[2] == 0)
        return 5; // Çıkış yönlendirme, ekleme modu (>>)
    else
        return 0; // Normal komut veya argüman
}

// Bir token'ı kaynak diziden hedef diziye kopyalar
// Tilda ve çevre değişkenlerini genişleterek kopyalar
// was_quoted_ptr parametresi, token içinde tırnak olup olmadığını döndürür
int lex_cpy(char *dst, char *src, int i, t_env *env, int *was_quoted_ptr, int *is_expended, int old_token)
{
    int l; // Hedef indeksi
    int q; // Tırnak durumu
    int start_i;

    *was_quoted_ptr = 0; // Token içinde tırnak olup olmadığını takip eder
    *is_expended = 0;
    start_i = i;
    l = 0;
    q = 0;
    while (!(is_ws(src[i])) && src[i])
    {
        // Kaçış karakteri (\) kontrolü (tırnak dışında)
        if (src[i] == '\\' && src[i + 1] && !q)
        {
            i++;                 // backslash'i atla
            dst[l++] = src[i++]; // sonraki karakteri kopyala
            continue;
        }
        // Tırnak işareti kontrolü
        if (!q && (src[i] == '"' || src[i] == '\''))
        {
            if (src[i] == '"')
                *was_quoted_ptr = 6; // Token içinde tırnak var
            else if (src[i] == '\'')
                *was_quoted_ptr = 7; // Token içinde tırnak var
            q = src[i];
            i++; // Sadece başlangıç tırnağını atla, kopyalama

            while (src[i] && src[i] != q)
            {
                // Kaçış karakteri işleme (sadece çift tırnak içinde)
                if (q == '"' && src[i] == '\\' && (src[i + 1] == '"' || src[i + 1] == '\\' || src[i + 1] == '$'))
                {
                    i++;                 // Kaçış karakterini atla
                    dst[l++] = src[i++]; // Kaçırılan karakteri kopyala
                }
                else if (q == '"' && src[i] == '$' && (ft_isalnum(src[i + 1]) || src[i + 1] == '_') && old_token != 2)
                {
                    expend_dollar(dst, &l, &i, env, src, is_expended);
                    i++;
                }
                else
                {
                    dst[l++] = src[i++]; // Normal karakteri kopyala
                }
            }

            if (src[i] == q)
                i++; // Sadece bitiş tırnağını atla, kopyalama

            q = 0; // Tırnak durumunu sıfırla
            continue;
        }

        // Tilda (~) işareti kontrolü - HOME dizini ile değiştirilecek
        if (src[i] == '~' && (i == 0 || is_ws(src[i - 1])) && (src[i + 1] == '/' || is_ws(src[i + 1]) || src[i + 1] == 0) && !q && old_token != 2)
        {
            expend_tilda(dst, &l, &i, env, is_expended);
            i++;
        }
        // Çevre değişkeni ($VAR) kontrolü
        else if (src[i] == '$' && (ft_isalnum(src[i + 1]) || src[i + 1] == '_') && !q && old_token != 2)
        {
            expend_dollar(dst, &l, &i, env, src, is_expended);
            i++;
        }
        else
        {
            // Normal karakteri kopyala
            dst[l++] = src[i++];
        }
    }
    dst[l] = 0; // Dizinin sonuna NULL karakteri ekle

    // Tüketilen karakter sayısını döndür
    return (i - start_i);
}

// Boşluk karakterlerini geçer ve atlanan karakter sayısını döndürür
// t=1 ise önce boşluk olmayan karakterleri geçer, sonra boşlukları geçer
// t=0 ise sadece boşlukları geçer
int skip_ws(const char *str, int i, int t)
{
    int j;

    j = 0;
    if (t)
    {
        while (!(str[i] == ' ' || (str[i] <= '\r' && str[i] >= '\t')) && str[i])
        {
            i++;
            j++;
        }
    }
    while ((str[i] == ' ' || (str[i] <= '\r' && str[i] >= '\t')) && str[i])
    {
        i++;
        j++;
    }
    return j;
}

// Komut dizesini token'lara ayırır (lexical analysis)
t_lexer *lexer(char *cmd, t_env *envir,t_garbage **garb)
{
    t_lexer *root;
    t_lexer *iter;
    int i;
    int j;
    int token_len;
    int consumed_len;
    int was_quoted;
    int is_expended;
    int old_token;

    j = 0;
    i = 0;
    old_token = 0;
    root = malloc(sizeof(t_lexer));
    g_collecter(garb,root,1);
    iter = root;
    j += skip_ws(cmd, j, 0); // Baştaki boşlukları atla
    while (cmd[j])
    {
        // Get token length
        token_len = get_len(cmd, j, envir, old_token);
        // Token için bellek ayır ve kopyala
        iter->str = malloc(token_len + 1);
        g_collecter(garb,iter->str,1);
        consumed_len = lex_cpy(iter->str, cmd, j, envir, &was_quoted, &is_expended, old_token);

        // Eğer tırnak içindeyse her zaman token = 0 (normal metin), değilse token() fonksiyonuyla belirle
        if (!was_quoted)
            iter->token = token(iter->str);
        old_token = iter->token;
        iter->was_quoted = was_quoted;
        iter->is_expended = is_expended;
        iter->i = i++;
        j += consumed_len;
        // Skip whitespace to get to the next token
        j += skip_ws(cmd, j, 0);
        // Daha fazla token varsa yeni düğüm oluştur
        if (cmd[j])
        {
            iter->next = malloc(sizeof(t_lexer));
            g_collecter(garb,iter->next,1);
            iter = iter->next;
        }
        else
        {
            iter->next = NULL; // Ensure the last node points to NULL
        }
    }
    return root; // Token listesinin başını döndür
}

// Çevre değişkeni dizesinin uzunluğunu hesaplar
// t=0 ise değişken adının uzunluğunu, t=1 ise değerinin uzunluğunu döndürür
int env_len(char *env, int t)
{
    int l;
    int i;

    i = 0;
    l = 0;
    while (env[i] != '=')
        i++;
    if (t)
    {
        while (env[i + l])
            l++;
        return l;
    }
    return i + 1;
}

// Çevre değişkenini tür ve içerik olarak ayırır
void env_cpy(char *type, char *content, const char *env)
{
    int i;
    int l;

    l = 0;
    i = -1;
    // Değişken adını kopyala (= işaretine kadar)
    while (env[++i] != '=')
        type[i] = env[i];
    type[i] = 0;
    i++;
    // Değişken değerini kopyala (= işaretinden sonrası)
    while (env[i + l])
        content[l++] = env[i + l];
    content[l] = 0;
}

// Sistem çevre değişkenlerini parse eder ve bir bağlı liste oluşturur
t_env *split_env(char **env,t_garbage **garb)
{
    int i;
    t_env *root;
    t_env *iter;

    root = malloc(sizeof(t_env));
    g_collecter(garb,root,0);
    iter = root;
    i = 0;
    while (env[i])
    {
        // Her çevre değişkeni için bellek ayır
        iter->type = malloc(env_len(env[i], 0));
        g_collecter(garb,iter->type,0);
        iter->content = malloc(env_len(env[i], 1));
        g_collecter(garb,iter->content,0);
        // Değişken adı ve değerini kopyala
        env_cpy(iter->type, iter->content, env[i]);

        if (env[++i])
        {
            iter->next = malloc(sizeof(t_env));
            g_collecter(garb,iter->next,0);
            iter = iter->next;
        }
    }
    return root; // Çevre değişkenleri listesinin başını döndür
}

int more_space_len(char *cmd)
{
    int i = -1;
    int l = 0;
    int q = 0; // Tırnak kontrolü

    // NULL pointer kontrolü
    if (!cmd)
        return 0;

    while (cmd[++i])
    {
        // Tırnak açma/kapama takibi
        if (!q && (cmd[i] == '\'' || cmd[i] == '"'))
            q = cmd[i];
        else if (q && cmd[i] == q)
            q = 0;

        if ((cmd[i] == '<' || cmd[i] == '|' || cmd[i] == '>') && !q)
        {
            l++; // boşluk ihtiyacı
            while ((cmd[i] == '<' || cmd[i] == '>' || cmd[i] == '|') && cmd[i])
                i++;
            l++; // sağındaki boşluk
            i--; // döngü artışına telafi
        }
    }
    return (i + 1 + 2 * l); // i + 1 çünkü döngü sonunda i cmd[i] == '\0' olduğu zaman çıkıyor
}


char *more_space(char *cmd,t_garbage **garb)
{
    int i = -1, l = 0;
    int q = 0;
    
    // NULL pointer kontrolü
    if (!cmd)
        return NULL;
        
    char *str = malloc(more_space_len(cmd)); // Genişletilmiş uzunluk tahmini
    g_collecter(garb,str,1);
    if (!str)
        return NULL;

    while (cmd[++i])
    {
        // Tırnak açma/kapama takibi
        if (!q && (cmd[i] == '\'' || cmd[i] == '"'))
            q = cmd[i];
        else if (q && cmd[i] == q)
            q = 0;

        if ((cmd[i] == '<' || cmd[i] == '>' || cmd[i] == '|') && !q)
        {
            str[l++] = ' ';
            while ((cmd[i] == '<' || cmd[i] == '>' || cmd[i] == '|') && cmd[i])
                str[l++] = cmd[i++];
            str[l++] = ' ';
            i--; // döngü artışında bir kez daha artacağı için geri al
        }
        else
        {
            str[l++] = cmd[i];
        }
    }
    str[l] = '\0';
    return str;
}


void free_lex_node(t_lexer *node)
{
    free(node->str);
    free(node);
}

void lex_clean(t_lexer **root)
{
    t_lexer *curr = *root;
    t_lexer *prev = NULL;
    t_lexer *next;

    while (curr)
    {
        next = curr->next;

        if (curr->str && curr->str[0] == '$' && !curr->is_expended && !curr->was_quoted)
        {
            if (prev)
                prev->next = curr->next;
            else
                *root = curr->next;

            free_lex_node(curr);
            // prev değişmiyor çünkü silinen düğüm prev değil
        }
        else
        {
            prev = curr;
        }

        curr = next;
    }
}

void repair_expand(t_lexer **root, t_env *env, t_garbage **garb)
{
    t_lexer *iter = *root;
    t_lexer *prev = NULL;
    t_lexer *new_list, *tail;
    t_lexer *tmp;
    int i = 0;

    while (iter)
    {
        if (iter->is_expended && !iter->was_quoted)
        {
            new_list = lexer(iter->str, env,garb); // Yeni liste oluştur
            tail = new_list;
            while (tail->next)
                tail = tail->next;
            if (prev)
                prev->next = new_list;
            else
                *root = new_list; // İlk node değiştirilmişse root'u güncelle

            tail->next = iter->next;
            tmp = iter;
            iter = tail->next;
            free(tmp); // iter yerine tmp'yi free et
        }
        else
        {
            prev = iter;
            iter = iter->next;
        }
    }
    iter = *root;
    while (iter)
    {
        iter->i = i++;
        iter = iter->next;
    }
}

int dollar_fix_len(char *cmd, t_env *env)
{
    int i = 0;
    int new_len = 0;
    int q = 0;

    // NULL pointer kontrolü
    if (!cmd)
        return 0;

    while (cmd[i])
    {
        if (cmd[i] == '\'' || cmd[i] == '"')
        {
            if (!q)
                q = cmd[i];
            else if (q == cmd[i])
                q = 0;
            new_len++;
            i++;
        }
        else if (cmd[i] == '$' && !q)
        {
            int var_len = 0;
            int j = i + 1;
            while (cmd[j] && (ft_isalnum(cmd[j]) || cmd[j] == '_'))
            {
                var_len++;
                j++;
            }

            if (var_len > 0)
            {
                char *value = get_env_cont_for_type(env, cmd + i + 1, var_len);
                if (value)
                    new_len += (var_len +  1); // Bulunan değerin uzunluğunu ekle
                // Bulunamayan değişken: hiçbir şey eklenmez
                i += var_len + 1; // '$' + variable name kadar atla
            }
            else
            {
                // '$' tek başına kalırsa normal karakter gibi say
                new_len++;
                i++;
            }
        }
        else
        {
            new_len++;
            i++;
        }
    }
    return new_len;
}


char *dollar_fix(char *cmd, t_env *env,t_garbage **garb)
{
    int i = 0;
    int j = 0;
    int q = 0;
    int var_len;
    char *result;
    char *value;

    // NULL pointer kontrolü
    if (!cmd)
        return NULL;

    int new_len = dollar_fix_len(cmd, env);
    result = malloc(sizeof(char) * (new_len + 1));
    g_collecter(garb,result,1);
    if (!result)
        return NULL;

    while (cmd[i])
    {
        if ((cmd[i] == '\'' || cmd[i] == '"'))
        {
            if (!q)
                q = cmd[i];
            else if (cmd[i] == q)
                q = 0;

            result[j++] = cmd[i++];
        }
        else if (cmd[i] == '$' && !q)
        {
            int start = i + 1;
            var_len = 0;

            while (cmd[start + var_len] && (ft_isalnum(cmd[start + var_len]) || cmd[start + var_len] == '_'))
                var_len++;

            if (var_len > 0)
            {
                value = get_env_cont_for_type(env, cmd + start, var_len);
                if (value)
                {
                    int k = 0;
                    while (k <= var_len)
                        result[j++] = cmd[i + k ++];
                }
                i += var_len + 1; // '$' + variable
            }
            else
            {
                // '$' tek başına
                result[j++] = cmd[i++];
            }
        }
        else
        {
            result[j++] = cmd[i++];
        }
    }

    result[j] = '\0';
    return result;
}
// ... diğer header dosyalarını da burada dahil etmelisin (split_env, lexer, etc.)

int main(int ac, char **av, char **env)
{
    t_lexer *lexer_root;
    t_lexer *iter;
    t_env *envir;
    t_ast *ast_root;
    t_garbage *garb;
    char *s;
    // Çevre değişkenlerini parse et
    garb = NULL;
    envir = split_env(env,&garb);

    while (1)
    {
        // readline ile kullanıcıdan komut al
        s = readline("minishell> ");
        // Kullanıcı Ctrl+D (EOF) ile çıkmak isterse
        if (!s)
        {
            printf("exit\n");
            break;
        }

        // Boş komutları atla
        if (*s == '\0')
        {
            free(s);
            continue;
        }

        // Komutu geçmişe (history) ekle
        add_history(s);

        // Dollar fix işlemi
        s = dollar_fix(s, envir, &garb);
        
        // Gerekirse ekstra boşlukları düzenle
        s = more_space(s, &garb);
        if(strcmp("exit",s) == 0)
        {
            g_free_l(&garb);
            break;
        }
        // Tırnak kontrolü
        if (quote_check(s))
        {
            lexer_root = lexer(s, envir,&garb);
            repair_expand(&lexer_root,envir,&garb);
        
            printf("\n--- Lexer tokens WITHOUT CLEAN ---\n");
            iter = lexer_root;
            while (iter)
            {
                printf("'%s' (token: %d, i: %d, exp: %d, quoted: %d)\n",
                iter->str, iter->token, iter->i,
                iter->is_expended, iter->was_quoted);
                iter = iter->next;
            }
            ast_root = parse_tokens(lexer_root,&garb);
            
            printf("\n--- Parser AST ---\n");
            if (ast_root)
            {
                print_ast(ast_root, 0);
            }
            else
            {
                printf("Parse error: Could not build AST\n");
            }
           //ast_root = parse_tokens(lexer_root,&garb);
           printf("%d\n",execute(ast_root, envir, env,&garb));
           g_free_l(&garb);
        }
        else
        {
            printf("Syntax error: Unclosed quotes\n");
        }

        // free(s) kaldırıldı çünkü garbage collector ile yönetiliyor
    }
    g_free(&garb);
    return 0;
}
