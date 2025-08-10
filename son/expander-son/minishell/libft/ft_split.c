/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_split.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fses <fses@student.42kocaeli.com.tr>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/23 03:12:40 by fses              #+#    #+#             */
/*   Updated: 2024/11/23 03:15:26 by fses             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <stdlib.h>

static size_t	get_word_length(const char *str, char delim)
{
	size_t	len;

	len = 0;
	while (str[len] && str[len] != delim)
		len++;
	return (len);
}

static size_t	count_words(const char *str, char delim)
{
	int		in_word;
	size_t	count;

	count = 0;
	in_word = 0;
	while (*str)
	{
		if (*str != delim && !in_word)
		{
			in_word = 1;
			count++;
		}
		else if (*str == delim)
			in_word = 0;
		str++;
	}
	return (count);
}

static int	free_split(char **result, size_t i)
{
	while (i > 0)
		free(result[--i]);
	free(result);
	return (0);
}

static char	**split_into(char **result, char delim, const char *str)
{
	size_t	i;
	size_t	j;
	size_t	word_len;

	i = 0;
	while (*str)
	{
		if (*str != delim)
		{
			word_len = get_word_length(str, delim);
			result[i] = (char *)malloc((word_len + 1) * sizeof(char));
			if (!result[i])
				return (free_split(result, i), NULL);
			j = 0;
			while (j < word_len)
				result[i][j++] = *str++;
			result[i][j] = '\0';
			i++;
		}
		else
			str++;
	}
	result[i] = NULL;
	return (result);
}

char	**ft_split(const char *str, char delim)
{
	size_t	word_count;
	char	**result;

	word_count = count_words(str, delim);
	result = (char **)malloc((word_count + 1) * sizeof(char *));
	if (!result)
		return (NULL);
	if (!split_into(result, delim, str))
		return (NULL);
	return (result);
}
