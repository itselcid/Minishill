#include <stdlib.h>
#include <stdio.h>

int main() {
   // Name of the environment variable (e.g., PATH)
   const char *name = "PATdcfH";
   // Get the value associated with the variable
   const char *env_p = getenv(name);
   if(env_p){
      printf("Your %s is %s\n", name, env_p);
   }
   else
    printf("doesnt exist \n");
   return 0;
}