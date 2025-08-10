#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include "minishell.h"

void *g_collecter(t_garbage **garb, void *adress,int l_flag)
{
    t_garbage   *root;
    t_garbage   *iter;
    t_garbage   *new_node;
    root = (*garb);
    iter = root;
    new_node = malloc(sizeof(t_garbage));
    new_node->adress=adress;
    new_node->line = l_flag;
    new_node->next = NULL;
    if(!iter)
        iter = new_node;
    else
    {
        while(iter->next)
            iter = iter->next;
        iter->next = new_node;
    }
    return adress;
}

void g_free_l(t_garbage **garb)
{
    t_garbage *root;
    t_garbage *iter;
    t_garbage *tmp;

    root = (*garb);
    iter = root;
    while(iter)
    {
        if(iter->adress != NULL && iter->line)
        {
            free(iter->adress);
            iter->adress = NULL; // Double free'i önlemek için
        }
        tmp = iter;
        iter = iter->next;
        free(tmp);
    }
    *garb = NULL; // Root'u NULL yap
}

void g_free(t_garbage **garb)
{
    t_garbage *root;
    t_garbage *tmp;

    root = (*garb);
    while(root)
    {
        if(root->adress !=  NULL)
            free(root->adress);
        tmp = root;
        root = root->next;
        free(tmp);
    }
    *garb = NULL; // Root'u NULL yap
}
