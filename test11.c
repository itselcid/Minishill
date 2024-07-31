/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   test11.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: elcid <elcid@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/07/30 11:14:58 by elcid             #+#    #+#             */
/*   Updated: 2024/07/30 11:45:25 by elcid            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <stdio.h>
int	is_builtin(char *cmd)
{
	char	*builtins[10]={"echo", "cd", "pwd", "export", "unset", "env",
		"exit", NULL};
	int		i;

	*builtins = {"echo", "cd", "pwd", "export", "unset", "env",
		"exit", NULL};
	i = 0;
	while (builtins[i])
	{
		if (strcmp(cmd, builtins[i]) == 0)
			return (1);
		i++;
	}
	return (0);
}
