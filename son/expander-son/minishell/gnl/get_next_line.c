/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   get_next_line.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fses <fses@student.42kocaeli.com.tr>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/11 10:55:24 by fses              #+#    #+#             */
/*   Updated: 2025/07/26 16:33:41 by fses             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "get_next_line.h"
#include <stdlib.h>
#include <unistd.h>

static char	*last_part(char *extra)
{
	int		i;
	int		j;
	char	*tmp;

	i = 0;
	j = 0;
	while (extra[i] != '\0' && extra[i] != '\n')
		i++;
	if (!extra[i])
	{
		free(extra);
		return (NULL);
	}
	tmp = (char *)malloc(ft_strlen_ps(extra) - i + 1);
	if (!tmp)
		return (NULL);
	i++;
	while (extra[i])
		tmp[j++] = extra[i++];
	tmp[j] = '\0';
	free(extra);
	return (tmp);
}

static char	*first_line(char *extra)
{
	int		i;
	char	*line;

	i = 0;
	if (!extra[i])
		return (NULL);
	while (extra[i] != '\n' && extra[i] != '\0')
		i++;
	if (ft_strchr_ps(extra))
		line = (char *)malloc(i + 2);
	else
		line = (char *)malloc(i + 1);
	if (!line)
		return (NULL);
	i = 0;
	while (extra[i] != '\0' && extra[i] != '\n')
	{
		line[i] = extra[i];
		i++;
	}
	if (extra[i] == '\n')
		line[i++] = '\n';
	line[i] = '\0';
	return (line);
}

static char	*read_myfd(int fd, char *extra)
{
	char	*buffer;
	int		readed;

	readed = 1;
	buffer = (char *)malloc(BUFFER_SIZE + 1);
	if (!buffer)
		return (NULL);
	while (!ft_strchr_ps(extra) && readed != 0)
	{
		readed = read(fd, buffer, BUFFER_SIZE);
		if (readed == -1)
		{
			free(extra);
			free(buffer);
			return (NULL);
		}
		buffer[readed] = '\0';
		extra = ft_strjoin_ps(extra, buffer);
	}
	free(buffer);
	return (extra);
}

char	*get_next_line(int fd)
{
	static char	*extra = NULL;
	char		*line;

	if (fd < 0 || BUFFER_SIZE <= 0)
	{
		free(extra);
		return (NULL);
	}
	extra = read_myfd(fd, extra);
	if (!extra)
		return (NULL);
	line = first_line(extra);
	extra = last_part(extra);
	return (line);
}
