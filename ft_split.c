
 #include<stdio.h>


static int	count_words(const char *s, char c)
{
	int	words;
	int	i;

	words = 0;
	i = 0;
	while (s != NULL && s[i])
	{
		if (s[i] != c)
		{
			words++;
			while (s[i] && s[i] != c)
				i++;
		}
		while (s[i] && s[i] == c)
			i++;
	}
	return (words);
}

static int	word_len(const char *s, char c)
{
	int	len;

	len = 0;
	while (s[len] != c && s[len] != '\0')
	{
		len++;
	}
	return (len);
}

static char	**freemem(int i, char **result)
{
	while (i >= 0)
	{
		free(result[i]);
		i--;
	}
	free(result);
	return (NULL);
}

static char	*copy_word(const char *s, char c)
{
	int		len;
	char	*word;
	int		j;

	len = word_len(s, c);
	word = (char *)malloc((len + 1) * sizeof(char));
	if (word == NULL)
		return (NULL);
	j = 0;
	while (j < len)
	{
		word[j] = s[j];
		j++;
	}
	word[len] = '\0';
	return (word);
}

char	**ft_split(char const *s, char c)
{
	int		i;
	char	**result;

	result = (char **)malloc((count_words(s, c) + 1) * sizeof(char *));
	if (result == NULL)
		return (NULL);
	i = 0;
	while (s != NULL && *s)
	{
		if (*s != c)
		{
			result[i] = copy_word(s, c);
			if (result[i] == NULL)
				return (freemem(i, result));
			s += word_len(s, c);
			i++;
		}
		else
			s++;
	}
	result[i] = NULL;
	return (result);
}