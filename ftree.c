#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <ctype.h>

#define BLU   "\x1B[34m"
#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define RESET "\x1B[0m"

#define MAX_PATH_LEN         512
#define MAX_PATH_LEN_DWN     511            /* MAX_PATH_LEN - 1*/
#define MAX_SRCH_LEN         32
#define MAX_SRCH_LEN_DWN     31
#define MAX_BASE_LEN         256
#define MAX_BASE_LEN_DWN     255
#define MAX_SRCH_COUNT       500
#define MAX_SRCH_COUNT_DWN   499
#define ERROR_OCCURED        999


typedef struct {    
          
        unsigned short int rows;   
        unsigned short int base_path_slash; 
        unsigned short int recursion_layer_limit;
        unsigned short int current_rec_layer;
        char searched_thing[MAX_SRCH_LEN];      

        enum {NO_tree = 0, YES_tree}                       flag_t;       
        enum {CASE_sens = 0, CASE_insens}                  flag_c;
        enum {EXACT_srch = 0, SUBSTRING_srch}              flag_s; 
        enum {NO_srch_for_empty = 0, YES_srch_for_empty}   flag_e;
        enum {NO_limit = 0, YES_limit}                     flag_l;
        enum {NO_srch = 0, DIR_srch, FILE_srch}            search_type;

} PARAMS_t;


#define print_usage() do{                                                                                                   \
    printf(                                                                                                                 \
        GRN "\t\tftree - search and list files/folders recursively\n\n" RESET                                               \
        "usage:  "                                                                                                          \
        "%s -h\n"                                                                                                           \
        "\t%s [path] -e [-l recursion_limit]\n"                                                                             \
        "\t%s [path] -t [-l recursion_limit]\n"                                                                             \
        "\t%s [path] {-d | -f} \"searched_thing\" [-t] [-c] [-s] [-l recursion_limit]\n\n"                                  \
        "-h: explain options\n"                                                                                             \
        "-t: list everything from given path\n"                                                                             \
        "-d: search for folders\n"                                                                                          \
        "-f: search for files\n"                                                                                            \
        "-c: search case insensitive.\n"                                                                                    \
        "-s: search for exact given name\n"                                                                                 \
        "-e: search only for empty dirs\n"                                                                                  \
        "-l: maxdepth i.e. levels of dirs below starting dir. value must be positive integer or 0\n"                        \
        "    0 means only scan in starting dir. without this limit program will circulate all dirs under starting dir\n"    \
        "path: from where to start searching   default: current dir\n\n"                                                    \
        "EXAMPLES:\n"                                                                                                       \
        "%s path -f picture           -> will search for *picture*.    will not show *Picture*\n"                           \
        "%s path -f picture -c        -> will search for *picture* case insensitive. will show *PicTurE* \n"                \
        "%s path -f picture.png -s    -> will search exact picture.png name. will not show Picture.png or *picture.png \n"  \
        "%s path -f picture.png -c -s -> will search for picture.png case insensitive. will show PicTurE.png but not *picture.png \n" \
        ,argv[0], argv[0], argv[0], argv[0], argv[0], argv[0], argv[0], argv[0] );                                          \
                                                                                                                            \
    exit(1); } while(0)

/*
    check_and_store

    Are we searching for something?
      If we search for exact name, use strcmp.
      If we search for paths that contain searched thing as substring. Then use strstr
        If we search case sensitive use d_name
        otherwise use lowercase format of d_name and searched_thing (searched_thing was lowered in main() ) 

*/
#define check_and_store(type) do{                                                                                           \
                                                                                                                            \
    if(handler->search_type == type &&                                                                                      \
        (handler->flag_s == SUBSTRING_srch ?                                                                                \
          (strstr((handler->flag_c == CASE_sens ? entity->d_name : base_name), handler->searched_thing) != NULL)            \
          : (strcmp((handler->flag_c == CASE_sens ? entity->d_name : base_name), handler->searched_thing) == 0))) {         \
                                                                                                                            \
        if(handler->rows < MAX_SRCH_COUNT_DWN) {                                                                            \
            list[handler->rows] = calloc(strlen(path)+1, 1);    /* sizeof(char) */                                          \
            strncpy(list[handler->rows++], path, strlen(path));                                                             \
        }                                                                                                                   \
        else if(handler->rows == MAX_SRCH_COUNT_DWN) {                                                                      \
            char warning[] = "count of searched thing is limited to MAX_SRCH_COUNT";                                        \
            list[handler->rows] = calloc(strlen(warning)+1, 1);                                                             \
            strncpy(list[handler->rows++], warning, strlen(warning));                                                       \
        }                                                                                                                   \
    }                                                                                                                       \
} while(0)



void ScanFiles(const char* dirname, char** list, PARAMS_t* handler) {
    static int i = 0, ii = 0;           /* we don't need to create counters for each function call  */
    handler->current_rec_layer++;

    DIR* dir = opendir(dirname);
    if(dir == NULL) {           /* there is a problem in combined new path */
        perror(RED "ftree-error" RESET);
        handler->flag_l = YES_limit;            
        handler->current_rec_layer = ERROR_OCCURED;   
        closedir(dir);
        return;
    }

    struct dirent* entity;
    entity = readdir(dir);

    while(entity != NULL) {
        /* don't show the . and .. (parent and current DIR) */
        if(strcmp(entity->d_name, ".") == 0 || strcmp(entity->d_name, "..") == 0) {
            entity = readdir(dir);
            continue;
        }
           
        i = 0; ii = 0;
        
        if(handler->flag_t == YES_tree) {           /* for adding tree appearance with spaces*/
            while(dirname[i++]) {
                if(dirname[i] == '/' &&  ++ii > handler->base_path_slash) printf("    ");
            }           /* usage of short circuit evaluation to increase ii */
        }
                                                        
        char path[MAX_PATH_LEN] = {0};  
        /* 
           all ScanFiles function calls have their unique path 

           so cannot make path variable static because we need those paths when the 
           program returns from recursion to its starting point DIR.
        */
        static char base_name[MAX_BASE_LEN] = {0};
        
        if(handler->flag_c == CASE_insens) {            /* copy d_name to base_name in lowered format */ 
            memset(&base_name[0], 0, sizeof(base_name));
            strncpy(base_name, entity->d_name, MAX_BASE_LEN_DWN);   

            i = 0;
            while(base_name[i]) {
                base_name[i] = tolower(base_name[i]);
                i++;
            }
          
        } 

        strncat(path, dirname, MAX_PATH_LEN_DWN - strlen(path));    
        strncat(path, "/", MAX_PATH_LEN_DWN - strlen(path));
        strncat(path, entity->d_name, MAX_PATH_LEN_DWN - strlen(path));     /* combine the new path */
        
        if(entity->d_type == DT_DIR) {    

            if(handler->flag_t == YES_tree) {
                printf(BLU "%s\n" RESET, entity->d_name);
            }
            check_and_store(DIR_srch);        

            if(handler->flag_l == YES_limit){
                if(handler->current_rec_layer <= handler->recursion_layer_limit){
                    ScanFiles(path, list, handler);
                }
            } else {
                ScanFiles(path, list, handler);         /* if d_type type is DIR, then recurse */
            }
            

        } else {      

            if(handler->flag_t == YES_tree) {
                printf("%s\n",entity->d_name);
            }
            check_and_store(FILE_srch);
        }
        
        entity = readdir(dir);

    }//while(entity != NULL)
    
    handler->current_rec_layer--;
    closedir(dir);
}


void ScanForEmpty(const char* dirname, PARAMS_t* handler) {
    handler->current_rec_layer++;

    DIR* dir = opendir(dirname);
    if(dir == NULL) {           /* there is a problem in combined new path*/
        perror(RED "ftree-error" RESET);
        handler->flag_l = YES_limit;
        handler->current_rec_layer = ERROR_OCCURED;
        closedir(dir);
        return;
    }

    struct dirent* entity;
    entity = readdir(dir);

    static int folder_is_empty = 1;  
    folder_is_empty = 1;
    
    while(entity != NULL) {
        /* don't show the . and .. (parent and current DIR) */
        if(strcmp(entity->d_name, ".") == 0 || strcmp(entity->d_name, "..") == 0) {
            entity = readdir(dir);
            continue;
        }

        folder_is_empty = 0;            /* program's entering here means folder is not empty  */

        if(entity->d_type == DT_DIR) {     
            if(handler->flag_l == YES_limit){
                if(handler->current_rec_layer <= handler->recursion_layer_limit){
                    char path[MAX_PATH_LEN] = {0};
                    strncat(path, dirname, MAX_PATH_LEN_DWN - strlen(path));
                    strncat(path, "/", MAX_PATH_LEN_DWN - strlen(path));
                    strncat(path, entity->d_name, MAX_PATH_LEN_DWN - strlen(path));                                       
                    ScanForEmpty(path, handler);        

                }

            } else {
                char path[MAX_PATH_LEN] = {0};
                strncat(path, dirname, MAX_PATH_LEN_DWN - strlen(path));
                strncat(path, "/", MAX_PATH_LEN_DWN - strlen(path));
                strncat(path, entity->d_name, MAX_PATH_LEN_DWN - strlen(path));                                         
                ScanForEmpty(path, handler);         /* combine the new path and recurse */
            }

        }

        entity = readdir(dir);
    }//while(entity != NULL)  

    if(folder_is_empty == 1){
        printf("%s\n", dirname);
    }
    
    handler->current_rec_layer--; 
    closedir(dir);

}


int main(int argc, char* argv[]) {

    //system("clear");          /* Clear The Shell */

    PARAMS_t init_params = { 
        .rows                   = 0u, 
        .base_path_slash        = 0u, 
        .recursion_layer_limit  = 0u,
        .current_rec_layer      = 0u,
        .searched_thing         = {0},
        .flag_t                 = NO_tree, 
        .flag_c                 = CASE_sens, 
        .flag_s                 = SUBSTRING_srch, 
        .flag_e                 = NO_srch_for_empty,
        .flag_l                 = NO_limit,
        .search_type            = NO_srch,
    };

    PARAMS_t* handler;
    handler = &init_params;
    
    int opt;    
    while((opt = getopt(argc, argv, ":d:f:l:tcseh")) != -1) {       
        switch(opt) { 
            case 'h':
                print_usage();
                break;

            case 'e':
                handler->flag_e = YES_srch_for_empty;
                break;

            case 't':
                handler->flag_t = YES_tree;
                break;

            case 'c': 
                handler->flag_c = CASE_insens;
                break; 

            case 's':
                handler->flag_s = EXACT_srch;
                break;
            
            case 'l': 
                if(handler->flag_l == YES_limit){   /* block multiple -l usage */
                    printf(RED "ftree-error: multiple -l usage detected\n" RESET);
                    print_usage();                  
                }
                handler->flag_l = YES_limit;

                int number = atoi(optarg);          
                if(number < 0){
                    printf(RED "ftree-error: -l value is not appropriate\n" RESET);
                    print_usage();
                }

                char str_number[32] = {0}; //<!
                sprintf(str_number, "%d", number);
                if(strlen(optarg) == strlen(str_number)){   /* error handling. -l value must be positive number without any letter */
                    if(number > 100) number = 100;
                    handler->recursion_layer_limit = number;
                } else {
                    printf(RED "ftree-error: -l value is not appropriate\n" RESET);
                    print_usage();
                }
                break;
                
            case 'd':
                if(handler->search_type == NO_srch) {           /* to block both -d and -f usage */
                    handler->search_type = DIR_srch;
                    strncpy(handler->searched_thing, optarg, MAX_SRCH_LEN_DWN);
                } else {
                    printf(RED "ftree-error: multiple -f or -d usage detected\n" RESET);
                    print_usage();
                } 
                break;

            case 'f': 
                if(handler->search_type == NO_srch) {
                    handler->search_type = FILE_srch;
                    strncpy(handler->searched_thing, optarg, MAX_SRCH_LEN_DWN);
                } else {
                    printf(RED "ftree-error: multiple -f or -d usage detected\n" RESET);
                    print_usage();
                } 
                break;

            default:
                printf(RED "ftree-error: unknown option was used or option value not entered \n" RESET);
                print_usage();
                break;
        } 
    } 

    /* optind is for the extra arguments which are not parsed
    if the starting path is not given then use current dir */
    if(optind >= argc) {     
        argv[optind] = ".";  
        argc++;
    }  

    char **list = calloc(MAX_SRCH_COUNT, sizeof(char *)); 
    /*
        there is no need to cast from void*.
        calloc is better than malloc + memset.

        **list for creating 2D array that will store the paths of searched things.
        500 rows are probably enough for that. If count is more than 500, code will give warning to user.
        length of rows will depend on the path length of searched thing. 
        instead of giving constant 500 value, I have tried to make rows dynamic too by reallocing **list.
        but when realloc moves the block, our row pointers will be useless thus core will dump.
    */

    if(handler->flag_e == YES_srch_for_empty) {         /* if -e option used, just search for empty DIRS and exit */
        ScanForEmpty(argv[optind], handler);
        free(list);
        return 0;
    }
    
    if(handler->flag_t == NO_tree && handler->search_type == NO_srch) {
        printf(RED "ftree-error: at least one of the -t and {-f | -d} must be used\n" RESET);
        free(list);
        print_usage();          
    }

    if((handler->flag_s == EXACT_srch || handler->flag_c == CASE_insens ) && 
        handler->search_type == NO_srch) {
        printf(RED "ftree-error: -c and -s needs {-f | -d}\n" RESET);
        free(list);
        print_usage();         
    }
                       
    int i = 0;
    if(handler->flag_c == CASE_insens) {      /* lowercase the searched thing for case insensitive searching */
        while(handler->searched_thing[i]){
            handler->searched_thing[i] = tolower(handler->searched_thing[i]);
            i++;
        }
    }
    
    i = 0;
    if(handler->flag_t == YES_tree) {
        while(argv[optind][i++]) {
            if(argv[optind][i] == '/') handler->base_path_slash++;  /* count the slashes in given path */ 
        }
    }
    
    printf("\n"); 

    DIR* dir = opendir(argv[optind]); /* test if starting point exists */
    if(dir == NULL) {
        perror(RED "ftree-error" RESET);
        printf(RED "\ncheck your starting point dir");
        free(list);
        closedir(dir);
        return 1;
    }

	ScanFiles(argv[optind], list, handler);         /* let's start */

    if(handler->current_rec_layer > 900){       /* checking for ERROR_OCCURED value*/
        printf(RED "\nprogram exited. paths are corrupted probably because of bufferoverflow" RESET);
        printf("\nincrease MAX_PATH_LEN and MAX_PATH_LEN_DWN macro values accordingly\n");

        for(i = 0; i < handler->rows;  i++){ 
            printf("%d> %s\n", i, list[i]); 
            free(list[i]); 
        } 
    }
    else if(handler->search_type != NO_srch) {           /* print results */

        printf(
            RED "\n%s you are searching for:'%s' (%s)\nRESULTS:\n" RESET,
            handler->search_type == DIR_srch ? "DIR" : "FILE", 
            handler->searched_thing, 
            handler->flag_c == CASE_sens ? "case-sensitive" : "case-insensitive"); 

        for(i = 0; i < handler->rows;  i++){ 
            printf("%d> %s\n", i, list[i]); 
            free(list[i]); 
        } 
    }

    free(list);
    return 0;
}