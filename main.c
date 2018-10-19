#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>


int PIDS[256] = {0};
int COUNTER = 0;


void placeholder(void){
    printf("I am a place holder\n");
}



char* recursive(const char* path){

    DIR *dp;
    DIR *dp2;
    struct dirent *ep;
    char pathname[2048] = {0};
    dp = opendir (path);

    if (dp != NULL)
    {
        while (ep = readdir (dp)) {

            if (strcmp(ep->d_name, ".") == 0 || strcmp(ep->d_name, "..") == 0) {
                continue;
            }


            sprintf(pathname,"%s%s/",path, ep->d_name);


            dp2 = opendir(pathname);

            puts(ep->d_name);


            if (dp2 != NULL) {

                closedir(dp2);


                char * status = recursive(pathname);


                if(strcmp(status, "child") == 0){
                    return "child";
                }

            }else{

                int pid = fork();

                if(pid == 0){
                    placeholder();
                    return "child";
                }

                else{

                    PIDS[COUNTER] = pid;
                    COUNTER++;
                    continue;

                }

            }

        }

        closedir(dp);

    }
    else {
        perror("Couldn't open the directory");
    }

    return "parent";

}

int main() {

    int i = 0;
    int status = 0;
    char* state = recursive("./");

    if(strcmp(state, "parent") == 0) {
        while (PIDS[i] != 0) {
            printf("%d\n", PIDS[i]);
            waitpid(PIDS[i], &status, NULL);
            i++;
        }


        printf("number of process: %d\n", COUNTER);


    }




    return 0;
}