#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>


int PIDS[256] = {0};
int COUNTER = 0;


#include "main.h"


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


    if(*fields_count == -1){
        *fields_count = countfields(line);
    }



    array_entry->length = strlen(line) + 1;
    char* temp = (char*) malloc(array_entry->length);
    strncpy(temp,line,array_entry->length);
    array_entry->fields = (char**) malloc((*fields_count) * sizeof(char*));

    while ((field = strsep(&temp, ",")) != NULL && i < 28) {

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


    return 0;

}

//


// build the array of entries through opening a file in memory and counting the number of entries needed to build the array
entry** load_array(int* entries_count, int* fields_count, char* filename){

    //TODO check validity of CSV file

    int i = 0;

    FILE* fptr = fopen(filename , "r");

    if(fptr == NULL){
        printf("%s\n", filename);
        perror(fptr);
        return NULL;
    }


    *entries_count = countlines(fptr);

    rewind(fptr);

    entry** buffer = (entry **) malloc(sizeof(entry*) * (*entries_count));


    char line_buffer[2048] ={0};

    while (fgets(line_buffer, sizeof(line_buffer), fptr) != NULL) {

        buffer[i] = (entry*) malloc(sizeof(entry));

        add_fields(buffer[i], fields_count,line_buffer);


        i++;


    }



    fclose(fptr);



    return buffer;
}

//

void sorter(const char* filename, const char* column, const char* output_directoy){



    int sorting_index = -1;


// call the function that builds the array
    int entries_count = -1;
    int fields_count = -1;
    int i = 0;
    int j = 0;

    entry** entries = load_array(&entries_count, &fields_count, filename);
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

    //TODO output filename and file directory here

// print the sorted array
    for(i = 0; i < entries_count - 1; i++) {

        for(j= 0; j < fields_count ; j++){

            printf("%s", entries[i]->fields[j]);
            if(j == fields_count - 1){
                break;
            }
            printf(",");
        }

        printf("\n");

    }
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


            sprintf(pathname,"%s%s/",input_directory, ep->d_name);


            dp2 = opendir(pathname);

            puts(ep->d_name);


            if (dp2 != NULL) {

                closedir(dp2);


                char * status = recursive(pathname, column, input_directory);


                if(strcmp(status, "child") == 0){
                    return "child";
                }

            }else{

                pathname[strlen(pathname) - 1 ] = '\0';

                int pid = fork();

                if(pid == 0){
//                    placeholder();
                    sorter(pathname, column, output_directory);
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

int main(int argc, char* argv[]) {


    char* input_directory = "./";
    char* output_directory = NULL;


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