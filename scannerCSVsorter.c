#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>


int PIDS[256] = {0};
int COUNTER = 0;


#include "scannerCSVsorter.h"


int merge_numeric = 1;


// sorting function that calls merging_int or merging_string depending on data type
void sort(entry** entries, entry** internal_buffer,int sorting_index ,int low, int high) {

    int mid;

    if (low < high) {
        mid = (low + high) / 2;
        sort(entries, internal_buffer, sorting_index,low, mid);
        sort(entries, internal_buffer, sorting_index,mid + 1, high);
        if(merge_numeric == 1){
            merging_int(entries, internal_buffer, sorting_index, low, mid, high);
        }
        else {
            merging_string(entries, internal_buffer, sorting_index, low, mid, high);
        }
    } else {
        return;
    }
}
//


// function to count the number of fields needed
int countfields(char* line){
    int count = 0;
    int i =0;

    for(i = 0; line[i] != '\0'; ++i)
    {
        if(line[i] == ',')
            ++count;
    }

    return count + 1;

}
//

// function to count the number of lines in the opened file in memory
int countlines (FILE *fin)
{
    int  nlines  = 0;
    char line[BUFFSIZE];

    while(fgets(line, BUFFSIZE, fin) != NULL) {
        nlines++;
    }

    return nlines;
}
//

// add fields from a line in the opened file to an entry element in a the entry array
int add_fields(entry* array_entry,int* fields_count,  char* line){

    int i = 0;
    char* field = NULL;

    int linecounter = 0;


    if(*fields_count == -1){
        *fields_count = countfields(line);
    }





    array_entry->length = strlen(line) + 1;
    char* temp = (char*) malloc(array_entry->length);
    strncpy(temp,line,array_entry->length);
    array_entry->fields = (char**) malloc((*fields_count) * sizeof(char*));

    while ((field = strsep(&temp, ",")) != NULL) {

        linecounter += 1;

        sanitize_content(field);

        if(strcmp(field, "") == 0){
            array_entry->fields[i] = malloc(sizeof("") + 1);
            strncpy(array_entry->fields[i] ,"\0", sizeof("") + 1);
        }
        else {

            array_entry->fields[i] = (char *) malloc((strlen(field) + 1) * sizeof(char));
            strncpy(array_entry->fields[i], field, strlen(field) + 1);
            if(array_entry->fields[i][0] == '"'){

                size_t index = strlen(field);
                size_t size = strlen(field) + 1;

                while(field[strlen(field)-1] != '"') {

                    field = strsep(&temp, ",");

                    size += strlen(field) + 1;
                    array_entry->fields[i] = realloc(array_entry->fields[i], size);
                    sprintf(array_entry->fields[i] + index, ",%s", field);
                    index += strlen(field) + 1;

                }



            }
        }

        i++;



    }


    free(temp);

    if (*fields_count != linecounter){

        fprintf(stderr, "FILE HAS WRONG FORMAT\n");
        return -1;
    }


    return 0;

}

//


// build the array of entries through opening a file in memory and counting the number of entries needed to build the array
entry** load_array(int* entries_count, int* fields_count, char* filename){



    int i = 0;
    int status = 0;

    if(strstr(filename, "-sorted-")!= NULL){
        return NULL;
    }

    FILE* fptr = fopen(filename , "r");

    if(fptr == NULL){
        return NULL;
    }


    *entries_count = countlines(fptr);

    rewind(fptr);

    entry** buffer = (entry **) malloc(sizeof(entry*) * (*entries_count));


    char line_buffer[4096] ={0};

    while (fgets(line_buffer, sizeof(line_buffer), fptr) != NULL) {

        buffer[i] = (entry*) malloc(sizeof(entry));

        status= add_fields(buffer[i], fields_count,line_buffer);

        if(status == -1){
            fclose(fptr);
            free(buffer);
            return NULL;

        }



        i++;


    }



    fclose(fptr);



    return buffer;
}

//

void sorter(const char* pathname, const char* column, const char* output_directoy, char* rawfilename){



    int sorting_index = -1;


// call the function that builds the array
    int entries_count = -1;
    int fields_count = -1;
    int i = 0;
    int j = 0;

    entry** entries = load_array(&entries_count, &fields_count, pathname);

    if(entries == NULL){
        return;
    }
//

// determine if column to be sorted exists in fields
    for(i = 0; i < fields_count; i++){
        if(strcmp(entries[0]->fields[i], column) == 0){
            sorting_index = i;
            break;
        }
    }


    if(sorting_index == -1){
        fprintf(stderr, "INVALID COLUMN NAME\n");
        goto NAME_NOT_FOUND;
    }
//

//determine the type of entries to be sorted (numeric vs strings)
    int k ;

    for(k=1; k < entries_count -1; k++){

        if(strcmp(entries[k]->fields[sorting_index], "") == 0){
            continue;
        }


        int entry_length =  (int) strlen(entries[k]->fields[sorting_index]);

        for(i = 0; i < entry_length; i++){


            if(!(isdigit(entries[k]->fields[sorting_index][i]))){
                merge_numeric = 0;
                break;
            }
        }
    }

    i = 0;


//

// load the internal buffer needed by mergesort
    entry** internal_buffer = (entry**) malloc(sizeof(entry*) * entries_count);

    while(i < entries_count){

        internal_buffer[i] = (entry*) malloc(sizeof(entry));
        i++;
    }
//

// call sort on the array
    sort(entries, internal_buffer, sorting_index, 1, entries_count - 2);
//
    char ouptutilfe_path[512] = {0};

    rawfilename[strlen(rawfilename) - 4] = '\0';

    sprintf(ouptutilfe_path, "%s/%s-sorted-%s.csv",output_directoy,rawfilename,column);


    FILE * output = fopen(ouptutilfe_path, "w");

// print the sorted array
    for(i = 0; i < entries_count - 1; i++) {

        for(j= 0; j < fields_count ; j++){

            fprintf(output, "%s", entries[i]->fields[j]);
            if(j == fields_count - 1){
                break;
            }
            fprintf(output, ",");
        }

        fprintf(output, "\n");

    }
    fclose(output);
//

// free memory
    for(i = 0; i < entries_count -1; i++){
        free(internal_buffer[i]);
    }

    free(internal_buffer);


    NAME_NOT_FOUND:   for(i = 0; i < entries_count - 2; i++){

    for(j= 0; j < fields_count - 1; j++){

        free(entries[i]->fields[j]);
    }

    free(entries[i]->fields);

    free(entries[i]);
}

    free(entries);

//

}



char* recursive(const char* input_directory, const char* column, const char* output_directory){

    DIR *dp;
    DIR *dp2;
    struct dirent *ep;
    char pathname[2048] = {0};
    dp = opendir (input_directory);

    if (dp != NULL)
    {
        while (ep = readdir (dp)) {

            if (strcmp(ep->d_name, ".") == 0 || strcmp(ep->d_name, "..") == 0) {
                continue;
            }


            sprintf(pathname,"%s/%s/",input_directory, ep->d_name);


            dp2 = opendir(pathname);


            if (dp2 != NULL) {

                closedir(dp2);

                int pid = fork();

                if(pid == 0){

                    recursive(pathname, column, output_directory);

                    return "child";

                }else{

                    PIDS[COUNTER] = pid;
                    COUNTER++;
                    continue;
                }

//                char * status = recursive(pathname, column, output_directory);
//
//
//                if(strcmp(status, "child") == 0){
//                    return "child";
//                }

            }else{

                int offset = (int) strlen(ep->d_name) -4;

                // TODO call fork before checking file extension





                    int pid = fork();


                    if (pid == 0) {

                        //calling the sorter function in the child

                        if(strcmp(ep->d_name+offset,".csv") == 0 ||
                           strcmp(ep->d_name + offset,".CSV") == 0 ) {

                            pathname[strlen(pathname) - 1] = '\0';


                            if (output_directory == NULL) {
                                sorter(pathname, column, input_directory, ep->d_name);
                            }else{
                                sorter(pathname, column, output_directory, ep->d_name);
                            }
                        }

                        return "child";

                    } else {

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

int main(int argc, char* argv[]) {


    char* input_directory = "./";
    char* output_directory = NULL;

    //TODO make the flags independent

    // check for the number of arguments
    if( argc < 3  || argc == 4 || argc == 6 ){
        fprintf(stderr, "INVALID NUMBER OF INPUTS\n");
        return 0;
    }
//

// check for flag -c in input
    if(strcmp(argv[1], "-c") != 0){
        fprintf(stderr, "INVALID COMMAND\n");
        return 0;
    }
//

    if( argc >= 5){

        if(strcmp(argv[3], "-d") != 0){
            fprintf(stderr, "INVALID COMMAND\n");
            return 0;
        }

        input_directory = argv[4];

        if (argc > 5){

            if(strcmp(argv[5], "-o") != 0){
                fprintf(stderr, "INVALID COMMAND\n");
                return 0;
            }

            output_directory = argv[6];

        }

    }


    int i = 0;
    int status = 0;
    char* state = recursive(input_directory, argv[2], output_directory);


    while (PIDS[i] != 0) {
        waitpid(PIDS[i], &status, NULL);
        i++;
    }


    if(strcmp(state, "parent") == 0) {

//
        // ftok to generate unique key
        key_t key = ftok("shmfile",65);

        // shmget returns an identifier in shmid
        int shmid = shmget(key,1024,0666|IPC_CREAT);

        // shmat to attach to shared memory
        char *str = (char*) shmat(shmid,(void*)0,0);

        char* number = NULL;

        while ((number = strsep(&str, ",")) != NULL) {

            if (strcmp(number, "") != 0) {

                int j = 0;
                int num = atoi(number);
                int bool = 1;

                while(PIDS[j] != 0){

                    if(num == PIDS[j]){
                        bool = 0;
                        break;
                    }

                    j++;

                }

                if(bool == 1) {

                    PIDS[COUNTER] = atoi(number);
                    COUNTER++;
                }

            }

        }

        //detach from shared memory
        shmdt(str);

        // destroy the shared memory
        shmctl(shmid,IPC_RMID,NULL);


        i = 0;

        printf("Initial PID: %d\n", getpid());

        printf("PIDS of all child processes: ");

        while (PIDS[i] != 0) {

            printf(" %d", PIDS[i]);

            if(PIDS[i+1] != 0){
                printf(",");
            }

            i++;

        }

        printf("\n");

        printf("Total number of processes: %d\n", COUNTER);


    }else if(strcmp(state, "child") == 0){

        i = 0;

        // ftok to generate unique key
        key_t key = ftok("shmfile",65);

        // shmget returns an identifier in shmid
        int shmid = shmget(key,1024,0666|IPC_CREAT);

        // shmat to attach to shared memory
        char *str = (char*) shmat(shmid,(void*)0,0);

        char str2[1024] = {0};

        char str3[1024] = {0};


        while(PIDS[i] != 0) {


            sprintf(str2, "%d",PIDS[i]);

            if(PIDS[i+1] == 0){
                strcat(str2, ",");
            }

            i++;

        }

        strcpy(str3,str);

        strcat(str3, str2);

        strcpy(str, str3);

        shmdt(str);

    }

    return 0;
}