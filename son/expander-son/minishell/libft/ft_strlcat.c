/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_strlcat.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fses <fses@student.42kocaeli.com.tr>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/23 03:12:12 by fses              #+#    #+#             */
/*   Updated: 2024/11/23 03:12:14 by fses             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "libft.h"

size_t	ft_strlcat(char *dst, const char *src, size_t dstsize)
{
	size_t	i;
	size_t	j;
	size_t	k;

	i = 0;
	j = ft_strlen(src);
	k = ft_strlen(dst);
	if (dstsize <= k)
		return (j + dstsize);
	while ((src[i] != '\0') && ((i + k) < (dstsize - 1)))
	{
		dst[k + i] = src[i];
		i++;
	}
	dst[k + i] = '\0';
	return (k + j);
}
