 #include<stdio.h>



char	*ft_strtok(char *str, char sepa)
{
	static char	*stock = NULL;
	char		*ptr;
	int		i;

	if(str!=NULL)
		stock=strdup(str);
	if(stock == NULL)
		return NULL;
	
	if(*stock==sepa)
		stock++;

	if(*stock=='\0')
		return NULL;

	ptr = stock;

	while(*stock != '\0' && *stock != sepa)
			stock++;
	if(*stock==sepa)
	{
		*stock='\0';
		stock++;
	}
	return (ptr);
}


int main()
{
     f();
printf("%s\n",environ[0]);
}