
#include <stdio.h>



int main()
{
    int id = fork();
    int n ;

    if(id == 0){
        n = 1;
       
    }

    else{
          n = 6;
         wait();
    }
      
    
    int i = n;
    while(i < n + 5)
    {
        printf("%d \n", i);
    i++;
    }
     
}