/*********************************************************************************************************************************************************
This program implements a basic interpreter for the 'Path_maker' language.

Description of the 'Path_maker' language:

    Data Types: Path is the only data type. Path constants are relative directory path expressions written in the form:

    <dir1/dir2/dir3> where dir1, dir2 and dir3 are directory names.
    • No file names are of any concern (just directories)
    • Directory names start with a letter (upper or lower case) and are made of any combination of letters, digits and underscore characters (only). (Punctuation characters are not allowed. Blank characters are not allowed either.) Directory names are not case sensitive so <AA> and <aa> are basically the same. (since that is the policy of most operating systems)
    • Operator “*” can be used instead of a directory name, and it indicates parent directory. It can be used multiple times before any other directory name.
    Example: <'*'/ * /mydirectory> indicates that one should move up (to parent) twice and then choose mydirectory.
    • Operator * can only be used at the beginning of path expressions. <hi/'*'/there> is not allowed.
    • Operator / cannot be used at the beginning or the end of any path. So </hi/there> is not allowed. Neither is <hi/there/> allowed.
    • Blanks in a path expression are ignored (unless they exist in a directory name (which is not allowed)) so < * / * / mydirectory> is OK.

    Variables: There are no variables in the language.

    Basic Commands: The only two basic commands are “make” and “go”. Make has the form:
    make <myDirectoryPath>;
    It simply creates the directories in the myDirectoryPath. If the path already exists it does nothing (but gives a warning message). If the path partially exists, it completes the path.
    Example: make <'*'/project1/data> goes up once and then creates a directory called project1 and then creates another one called “data” inside it.
    “make” does not change the current (working) directory. (This is what we do with go command)
    “go” simply changes the current directory

    Syntax: go <myPathExpression>;
    If the path does not exist, go does nothing. (Gives an error message but does not exit the execution) It does not partially follow a path. Partial existence of any path is considered as nonexistence.

    Control Structures: There is an “if” clause and a similar “ifnot” clause
    if <path_expression> command
    is the basic form of this clause where command can be a basic command or a block. “if” clause executes the command if from the current directory the path <path_expression> exists.
    “if” does not change the current directory.
    “ifnot” clause has the exact same structure
    ifnot <path_expression> command
    but operates if the path <path_expression> does not exist.

    Blocks: A command can be a basic command (“make” or “go”) but it can also be a block. A block is a list of lines of code enclosed in { } set brackets. Blocks may also be nested in one another.

    End of line character: Only “make” and “go” commands require an end of line character and it is ‘;’ (semi-colon)

    Keywords: Keywords are case sensitive and all are lowercase. They are:
    make, go, if, ifnot

    Symbols: < , > , { , } , / , *, ;
***************************************************************************************************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include <strings.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>


// Function prototypes
char* checkIfKeyWord(char *str);
char* isbracket(char c);
bool checkIfAlphaString(char *str);
char* findTokenType(char *str);
bool check_path(char* holder3, FILE* fptr3);
void go(char holder2[], char* cwd, FILE* fptr3);
void make (char holder2[], char* cwd, FILE* fptr3);
void ifPath_maker (char holder2[], char* cwd, FILE* fptr3);
void ifnot (char holder2[], char* cwd, FILE* fptr3);
void translate(char holder2[], FILE* fptr3, char* cwd);


// Max size of string to be read from 'code.lex'
// Means that length of a line can't be greater than it
#define BUFF_SIZE 255


// Main program logic
int main(void)
{


    /*
        Take in file name for the source code file and open
        it if it exists.
    */

    // Create character string to hold input
    // PATH_MAX is OS specific max path length
    char input[PATH_MAX + 1];
    printf("Enter file name (without the .pmk extension): ");
    // Take in input
    scanf("%s", input);
    // Concatenate file name with the .pmk extension
    strcat(input, ".pmk");
    // Open file in read mode
    FILE* fptr1 = fopen(input, "r");
    // Check for errors in opening file
    if (fptr1 == NULL)
    {
        printf("The source code file could not be found/read.\nExiting...\n");

        return 1;
    }


    /*********************************************************************
     * This subprogram reads in source code for the path_maker language *
     * and generates tokens to be used by the parser.                   *
     ********************************************************************/
    // Create file pointer to access code.lex, the object file to contain the tokens generated
    FILE* fptr2 = fopen("code.lex", "w");
    if(fptr2 == NULL)
    {
        printf("Error opening code.lex.\nExiting...\n");
        return 1;
    }

    // Holds incoming alphanumeric character string from file
    char holder[PATH_MAX + 1];
    for(int i = 0; i < PATH_MAX + 1; i++)
    {
        holder[i] = '\0';
    }

    // variables for holding current index values
    int index = 0;

    // Identifies lexeme(s) in the source code
    for(char c = fgetc(fptr1); c != EOF; c = fgetc(fptr1))
    {

        if(c == ';' || c == '*' || c == '/' || isbracket(c) || isspace(c))
        {
            // If character string is token, find token type and print it to code.lex file
            // print form: 'tokenType(lexeme)'
            if(*holder)
            {
                char *tokenType = findTokenType(holder);
                if(!strcasecmp(tokenType, "t_DirectoryName"))
                {
                    for (int i = 0; holder[i] != '\0'; i++)
                    {
                        fputc(tolower(holder[i]), fptr2);
                    }
                    fputc('\n', fptr2);
                }
                else if (!strcasecmp(tokenType, "t_go")
                         || !strcasecmp(tokenType, "t_make")|| !strcasecmp(tokenType, "t_if")
                         || !strcasecmp(tokenType, "t_ifnot"))
                {
                    fputs(tokenType, fptr2);
                    fputc('\n', fptr2);
                }
                else if(findTokenType(holder) == NULL)
                {
                    printf("Error. Unrecognized character: \"%s\" in source file.\nExiting...\n", holder);
                    return 1;
                }
            }

            // Reset index to zero; flush 'holder' array so that it can be used again anew
            index = 0;
            for(int i = 0; i < PATH_MAX + 1; i++)
            {
                holder[i] = '\0';
            }
        }
        // Check if character is an EOL character
        if(c == ';')
        {
            fputs("t_EndOfLine\n", fptr2);
            continue;
        }
        // Check if character is forward slash
        if (c == '/')
        {
            fputs("t_ForwardSlash\n", fptr2);
            continue;
        }

        // Check if character is an astrix
        if (c == '*')
        {
            fputs("t_Astrix\n", fptr2);
            continue;
        }
        // Check if character is a bracket
        if (isbracket(c) != NULL)
        {
            fputs(isbracket(c), fptr2);
            continue;

        }
        // If character string length is greater than MAX_LEN print error and exit
        if(index > PATH_MAX - 1)
        {
            printf("Error. Identifier length cannot be greater than %d characters long.\nExiting...\n", PATH_MAX);
            return 1;
        }
        else if(!isspace(c))
        {
            holder[index] = c;
            index++;
            continue;
        }
    }

    fclose(fptr1);
    fclose(fptr2);


    /************************************************
     * This subprogram is the parser for path_maker *
     ************************************************/

    // Create a variable that holds the current
    // (location of this program at execution) directory address
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) printf("Current directory: %s\n", cwd);
    else
    {
        printf("Error getting current directory.\nExiting...\n");
        return 1;
    }

    // Read text file containing the lexemes (code.lex)
    FILE* fptr3 = fopen("code.lex", "r");
    if (fptr3 == NULL)
    {
        printf("Error. 'Code.lex' file could not be read.\nExiting...\n");
        return 1;
    }

    // Create a variable to hold incoming strings
    char holder2[BUFF_SIZE + 1];
    for(int i = 0; i < BUFF_SIZE + 1; i++)
    {
        holder2[i] = '\0';
    }

    // Check to see if path is legal
    while(fgets(holder2, BUFF_SIZE, fptr3) != NULL)
    {
        for(int i = 0; i < BUFF_SIZE + 1; i++)
        {
            if (holder2[i] == '\n')
            {
                holder2[i] = '\0';
            }
        }
        if(feof(fptr3))
        {
            break;
        }

        if (!strcasecmp(holder2, "t_LessThanSign"))
        {
            if (check_path(holder2, fptr3))
            {
                if (!strcasecmp(holder2, "t_GreaterThanSign"));
                else
                {
                    printf("Error. Missing greater than sign after path name: <INVALID_PATH_NAME\n");
                    return 1;
                }
            }
            else
            {
                printf("Error. Less than sign was not followed by a valid path name: <INVALID_PATH_NAME\n");
            }
        }
    }
    // Take pointer to beginning of file
    rewind(fptr3);
    // Translate path_maker commands to C commands and execute
    translate(holder2, fptr3, cwd);
    // Close fptr3 to code.lex
    fclose(fptr3);
    return 0;
}


// Find token type: path or keyword
char* findTokenType(char *str)
{
    if (checkIfKeyWord(str)!= NULL)
    {
        return checkIfKeyWord(str);
    }
    bool alphaString_Check = checkIfAlphaString(str);
    if(alphaString_Check == true)
    {
        return "t_DirectoryName";
    }
    return NULL;
}


// Check if character string is made up of only ASCII alphabet characters
bool checkIfAlphaString(char *str)
{
    int len = strlen(str);
    if(isalpha(str[0]))
    {
        for(int i = 1; i < len; i++)
        {
            if(!isalnum(str[i]) && !(str[i] == '_'))
            {
                return false;
            }
        }
        return true;
    }
    return false;
}


// Check to see if character is a bracket as defined by path_maker definition
char* isbracket(char c)
{
    if(c == 123)
    {
        return "t_LeftCurlyBrace\n";
    }
    else if(c == 125)
    {
        return "t_RightCurlyBrace\n";
    }
    else if(c == 60)
    {
        return "t_LessThanSign\n";
    }
    else if(c == 62)
    {
        return "t_GreaterThanSign\n";
    }
    return NULL;
}


// Check if character string is a key word as defined in path_maker
char* checkIfKeyWord(char *str)
{
    if(!strcmp(str, "go"))
    {
        return "t_go";
    }
    else if (!strcmp(str, "make"))
    {
        return "t_make";
    }
    else if (!strcmp(str, "if"))
    {
        return "t_if";
    }
    else if (!strcmp(str, "ifnot"))
    {
        return "t_ifnot";
    }
    return NULL;
}


// Check a given path's syntactic validity
bool check_path(char* holder3, FILE* fptr3)
{
    fgets(holder3, BUFF_SIZE, fptr3);
    for(int i = 0; i < BUFF_SIZE + 1; i++)
    {
        if (holder3[i] == '\n')
        {
            holder3[i] = '\0';
        }
    }
    if(feof(fptr3))
    {
        return false;
    }

    LOOP: if (checkIfAlphaString(holder3) && (strcasecmp(holder3, "t_ForwardSlash") &&
                                              strcasecmp(holder3, "t_Astrix") &&
                                              strcasecmp(holder3, "t_LeftCurlyBrace") &&
                                              strcasecmp(holder3, "t_EndOfLine") &&
                                              strcasecmp(holder3, "t_if") &&
                                              strcasecmp(holder3, "t_ifnot") &&
                                              strcasecmp(holder3, "t_make") &&
                                              strcasecmp(holder3, "t_go") &&
                                              strcasecmp(holder3, "t_RightCurlyBrace") &&
                                              strcasecmp(holder3, "t_LessThanSign") &&
                                              strcasecmp(holder3, "t_GreaterThanSign")
                                              ))
    {
        fgets(holder3, BUFF_SIZE, fptr3);
        for(int i = 0; i < BUFF_SIZE + 1; i++)
        {
            if (holder3[i] == '\n')
            {
                holder3[i] = '\0';
            }
        }
        if(feof(fptr3))
        {
            return false;
        }
        if (!strcasecmp(holder3, "t_ForwardSlash"))
        {
            fgets(holder3, BUFF_SIZE, fptr3);
            for(int i = 0; i < BUFF_SIZE + 1; i++)
            {
                if (holder3[i] == '\n')
                {
                    holder3[i] = '\0';
                }
            }
            if (!strcasecmp(holder3, "t_Astrix"))
            {
                return false;
            }
            goto LOOP;
        }
        return true;
    }

    if (!strcasecmp(holder3, "t_Astrix"))
    {
        fgets(holder3, BUFF_SIZE, fptr3);
        for(int i = 0; i < BUFF_SIZE + 1; i++)
        {
            if (holder3[i] == '\n')
            {
                holder3[i] = '\0';
            }
        }
        if(feof(fptr3))
        {
            return false;
        }
        if (!strcasecmp(holder3, "t_ForwardSlash"))
        {
            check_path(holder3, fptr3);
        }
        return true;
    }
    return false;
}


// Translate path_maker commands to C command
void translate(char holder2[], FILE* fptr3, char* cwd)
{
    while(fgets(holder2, BUFF_SIZE, fptr3) != NULL)
    {
        for(int i = 0; i < BUFF_SIZE + 1; i++)
        {
            if (holder2[i] == '\n')
            {
                holder2[i] = '\0';
            }
        }
        if (feof(fptr3)) break;
        if (!strcasecmp(holder2, "t_go"))
        {
            go(holder2, cwd, fptr3);
        }
        if (!strcasecmp(holder2, "t_make"))
        {
            make(holder2, cwd, fptr3);
        }
        if (!strcasecmp(holder2, "t_if"))
        {
            ifPath_maker(holder2, cwd, fptr3);
        }
        if (!strcasecmp(holder2, "t_ifnot"))
        {
            ifnot(holder2, cwd, fptr3);
        }
    }


}


// Translate path_maker go command to C command
void go(char holder2[], char* cwd, FILE* fptr3)
{

    char cwd2[PATH_MAX];
    for(int i = 0; i < PATH_MAX; i++)
    {
        cwd2[i] = '\0';
    }
    strcpy(cwd2, cwd);
    fgets(holder2, BUFF_SIZE, fptr3);
    for(int i = 0; i < BUFF_SIZE + 1; i++)
    {
        if (holder2[i] == '\n')
        {
            holder2[i] = '\0';
        }
    }
    if (!strcasecmp(holder2, "t_LessThanSign"))
    {
        fgets(holder2, BUFF_SIZE, fptr3);
        for(int i = 0; i < BUFF_SIZE + 1; i++)
        {
            if (holder2[i] == '\n')
            {
                holder2[i] = '\0';
            }
        }
        char folder[PATH_MAX];
        for(int i = 0; i < PATH_MAX; i++)
        {
            folder[i] = '\0';
        }
        while (strcasecmp(holder2, "t_GreaterThanSign"))
        {
            if (!strcasecmp(holder2, "t_Astrix"))
            {
                for(int i = PATH_MAX - 1; i >= 0; i--)
                {
                    if (cwd[i] != '\\')
                    {
                         cwd[i] = '\0';
                    }
                    else
                    {
                        cwd[i] = '\0';
                        break;
                    }
                }
                strcpy(folder, cwd);
                fgets(holder2, BUFF_SIZE, fptr3);
                for(int i = 0; i < BUFF_SIZE + 1; i++)
                {
                    if (holder2[i] == '\n')
                    {
                        holder2[i] = '\0';
                    }
                }
                if (!strcasecmp(holder2, "t_ForwardSlash"))
                {
                     fgets(holder2, BUFF_SIZE, fptr3);
                    for(int i = 0; i < BUFF_SIZE + 1; i++)
                    {
                        if (holder2[i] == '\n')
                        {
                            holder2[i] = '\0';
                        }
                    }
                }
            }
            else if (strcasecmp(holder2, "t_GreaterThanSign"))
            {

                if (!strcasecmp(holder2, "t_ForwardSlash"))
                {
                    fgets(holder2, BUFF_SIZE, fptr3);
                    for(int i = 0; i < BUFF_SIZE + 1; i++)
                    {
                        if (holder2[i] == '\n')
                        {
                            holder2[i] = '\0';
                        }
                    }
                    if (!strcasecmp(holder2, "t_GreaterThanSign"))
                    {
                        break;
                    }
                }
                strcpy(folder, cwd);
                while (true)
                {
                    if(strcmp(holder2, ""))
                    {
                        strcat(folder, "\\");
                    }
                    strcat(folder, holder2);
                    fgets(holder2, BUFF_SIZE, fptr3);
                    for(int i = 0; i < BUFF_SIZE + 1; i++)
                    {
                        if (holder2[i] == '\n')
                        {
                            holder2[i] = '\0';
                        }
                    }
                    if (!strcasecmp(holder2, "t_ForwardSlash")) strcpy(holder2, "");
                    if (!strcasecmp(holder2, "t_GreaterThanSign"))
                    {
                        break;
                    }
                }
            }
        }
        fgets(holder2, BUFF_SIZE, fptr3);
        for(int i = 0; i < BUFF_SIZE + 1; i++)
        {
            if (holder2[i] == '\n')
            {
                holder2[i] = '\0';
            }
        }
        if (strcasecmp(holder2, "t_EndOfLine"))
        {
            printf("Error. 'go' statement was not followed by a semicolon. Exiting...\n");
            exit(0);
        }
        struct stat sb;
        if (stat(folder, &sb) == 0 && S_ISDIR(sb.st_mode)) {
            printf("Path exists. Go statement executed.\n");
            strcpy(cwd, folder);
            printf("Current directory is now changed to: %s\n", cwd);
        } else {
            printf("Path: %s does not exist. Go statement cannot be executed\n", folder);
            strcpy(cwd, cwd2);
        }
    }
    else
    {
        printf("Error. 'go' statement should be followed by a path name: '<PATH_NAME>'.\n");
        exit(0);
    }
}


// Translate path_maker make command to C command
void make(char holder2[], char* cwd, FILE* fptr3)
{
    fgets(holder2, BUFF_SIZE, fptr3);
    for(int i = 0; i < BUFF_SIZE + 1; i++)
    {
        if (holder2[i] == '\n')
        {
            holder2[i] = '\0';
        }
    }
    if (!strcasecmp(holder2, "t_LessThanSign"))
    {
        fgets(holder2, BUFF_SIZE, fptr3);
        for(int i = 0; i < BUFF_SIZE + 1; i++)
        {
            if (holder2[i] == '\n')
            {
                holder2[i] = '\0';
            }
        }
        char folder[PATH_MAX];
        for(int i = 0; i < PATH_MAX; i++)
        {
            folder[i] = '\0';
        }
        while (strcasecmp(holder2, "t_GreaterThanSign"))
        {
            if (!strcasecmp(holder2, "t_Astrix"))
            {
                for(int i = PATH_MAX - 1; i >= 0; i--)
                {
                    if (cwd[i] != '\\')
                    {
                         cwd[i] = '\0';
                    }
                    else
                    {
                        cwd[i] = '\0';
                        break;
                    }
                }
                strcpy(folder, cwd);
                fgets(holder2, BUFF_SIZE, fptr3);
                for(int i = 0; i < BUFF_SIZE + 1; i++)
                {
                    if (holder2[i] == '\n')
                    {
                        holder2[i] = '\0';
                    }
                }
                if (!strcasecmp(holder2, "t_ForwardSlash"))
                {
                     fgets(holder2, BUFF_SIZE, fptr3);
                    for(int i = 0; i < BUFF_SIZE + 1; i++)
                    {
                        if (holder2[i] == '\n')
                        {
                            holder2[i] = '\0';
                        }
                    }
                }
            }
            else if (strcasecmp(holder2, "t_GreaterThanSign"))
            {

                if (!strcasecmp(holder2, "t_ForwardSlash"))
                {
                    fgets(holder2, BUFF_SIZE, fptr3);
                    for(int i = 0; i < BUFF_SIZE + 1; i++)
                    {
                        if (holder2[i] == '\n')
                        {
                            holder2[i] = '\0';
                        }
                    }
                    if (!strcasecmp(holder2, "t_GreaterThanSign"))
                    {
                        break;
                    }
                }
                strcpy(folder, cwd);
                while (true)
                {
                    if(strcmp(holder2, ""))
                    {
                        strcat(folder, "\\");
                    }
                    strcat(folder, holder2);
                    fgets(holder2, BUFF_SIZE, fptr3);
                    for(int i = 0; i < BUFF_SIZE + 1; i++)
                    {
                        if (holder2[i] == '\n')
                        {
                            holder2[i] = '\0';
                        }
                    }
                    if (!strcasecmp(holder2, "t_ForwardSlash")) strcpy(holder2, "");
                    if (!strcasecmp(holder2, "t_GreaterThanSign"))
                    {
                        break;
                    }
                }
            }
        }
        fgets(holder2, BUFF_SIZE, fptr3);
        for(int i = 0; i < BUFF_SIZE + 1; i++)
        {
            if (holder2[i] == '\n')
            {
                holder2[i] = '\0';
            }
        }
        if (strcasecmp(holder2, "t_EndOfLine"))
        {
            printf("Error. 'make' statement was not followed by a semicolon. Exiting...\n");
            exit(0);
        }
        struct stat sb;
        if (stat(folder, &sb) == 0 && S_ISDIR(sb.st_mode)) {
            printf("Path already exists. Make statement will not be executed.\n");
        } else {
            char* mkcmd[PATH_MAX];
            for(int i = 0; i < BUFF_SIZE + 1; i++)
            {
               mkcmd[i] = '\0';
            }
            sprintf(mkcmd, "mkdir -p %s", folder);
            system(mkcmd);
            printf("Success. Path: \'%s\' created with make command.\n", folder);
        }
    }
    else
    {
        printf("Error. 'make' statement should be followed by a path name: '<PATH_NAME>'.\n");
        exit(0);
    }
}


// Translate path_maker if to C if
void ifPath_maker(char holder2[], char* cwd, FILE* fptr3)
{
    char cwd2[PATH_MAX];
    for(int i = 0; i < PATH_MAX; i++)
    {
        cwd2[i] = '\0';
    }
    strcpy(cwd2, cwd);
    fgets(holder2, BUFF_SIZE, fptr3);
    for(int i = 0; i < BUFF_SIZE + 1; i++)
    {
        if (holder2[i] == '\n')
        {
            holder2[i] = '\0';
        }
    }
    if (!strcasecmp(holder2, "t_LessThanSign"))
    {
        fgets(holder2, BUFF_SIZE, fptr3);
        for(int i = 0; i < BUFF_SIZE + 1; i++)
        {
            if (holder2[i] == '\n')
            {
                holder2[i] = '\0';
            }
        }
        char folder[PATH_MAX];
        for(int i = 0; i < PATH_MAX; i++)
        {
            folder[i] = '\0';
        }
        while (strcasecmp(holder2, "t_GreaterThanSign"))
        {
            if (!strcasecmp(holder2, "t_Astrix"))
            {
                for(int i = PATH_MAX - 1; i >= 0; i--)
                {
                    if (cwd[i] != '\\')
                    {
                         cwd[i] = '\0';
                    }
                    else
                    {
                        cwd[i] = '\0';
                        break;
                    }
                }
                strcpy(folder, cwd);
                fgets(holder2, BUFF_SIZE, fptr3);
                for(int i = 0; i < BUFF_SIZE + 1; i++)
                {
                    if (holder2[i] == '\n')
                    {
                        holder2[i] = '\0';
                    }
                }
                if (!strcasecmp(holder2, "t_ForwardSlash"))
                {
                     fgets(holder2, BUFF_SIZE, fptr3);
                    for(int i = 0; i < BUFF_SIZE + 1; i++)
                    {
                        if (holder2[i] == '\n')
                        {
                            holder2[i] = '\0';
                        }
                    }
                }
            }
            else if (strcasecmp(holder2, "t_GreaterThanSign"))
            {

                if (!strcasecmp(holder2, "t_ForwardSlash"))
                {
                    fgets(holder2, BUFF_SIZE, fptr3);
                    for(int i = 0; i < BUFF_SIZE + 1; i++)
                    {
                        if (holder2[i] == '\n')
                        {
                            holder2[i] = '\0';
                        }
                    }
                    if (!strcasecmp(holder2, "t_GreaterThanSign"))
                    {
                        break;
                    }
                }
                strcpy(folder, cwd);
                while (true)
                {
                    if(strcmp(holder2, ""))
                    {
                        strcat(folder, "\\");
                    }
                    strcat(folder, holder2);
                    fgets(holder2, BUFF_SIZE, fptr3);
                    for(int i = 0; i < BUFF_SIZE + 1; i++)
                    {
                        if (holder2[i] == '\n')
                        {
                            holder2[i] = '\0';
                        }
                    }
                    if (!strcasecmp(holder2, "t_ForwardSlash")) strcpy(holder2, "");
                    if (!strcasecmp(holder2, "t_GreaterThanSign"))
                    {
                        break;
                    }
                }
            }
        }
        struct stat sb;
        if (stat(folder, &sb) == 0 && S_ISDIR(sb.st_mode)) {
            printf("Path exists. If statement will be executed.\n");
            fgets(holder2, BUFF_SIZE, fptr3);
            for(int i = 0; i < BUFF_SIZE + 1; i++)
            {
                if (holder2[i] == '\n')
                {
                    holder2[i] = '\0';
                }
            }
            if (feof(fptr3)) {
                printf("Error. End of file reached without a command completing.\n");
                exit(0);}
            if (!strcasecmp(holder2, "t_go"))
            {
                go(holder2, cwd, fptr3);
            }
            else if (!strcasecmp(holder2, "t_make"))
            {
                make(holder2, cwd, fptr3);
            }
            if (!strcasecmp(holder2, "t_LeftCurlyBrace"))
            {
                translate(holder2, fptr3, cwd);
                if (!(strcasecmp(holder2, "t_RightCurlyBrace")));
                else printf("Error. Left curly brace not closed with a right curly brace.\n");
            }


        } else {
            printf("Path: %s does not exist. Command following if clause will not be executed.\n", folder);
            while (fgets(holder2, BUFF_SIZE, fptr3) != NULL){
            for(int i = 0; i < BUFF_SIZE + 1; i++){
                if (holder2[i] == '\n')
                {
                    holder2[i] = '\0';
                }
            }
            if (feof(fptr3)) {
                printf("Error. End of file reached without a command completing.\n");
                exit(0);}
            if (!(strcasecmp(holder2, "t_RightCurlyBrace"))) break;
        }
        }
    }
    else
    {
        printf("Error. 'if' statement should be followed by a path name: '<PATH_NAME>'.\n");
        exit(0);
    }
}


// Translate path_maker ifnot to C if(!expression)
void ifnot(char holder2[], char* cwd, FILE* fptr3)
{
    char cwd2[PATH_MAX];
    for(int i = 0; i < PATH_MAX; i++)
    {
        cwd2[i] = '\0';
    }
    strcpy(cwd2, cwd);
    fgets(holder2, BUFF_SIZE, fptr3);
    for(int i = 0; i < BUFF_SIZE + 1; i++)
    {
        if (holder2[i] == '\n')
        {
            holder2[i] = '\0';
        }
    }
    if (!strcasecmp(holder2, "t_LessThanSign"))
    {
        fgets(holder2, BUFF_SIZE, fptr3);
        for(int i = 0; i < BUFF_SIZE + 1; i++)
        {
            if (holder2[i] == '\n')
            {
                holder2[i] = '\0';
            }
        }
        char folder[PATH_MAX];
        for(int i = 0; i < PATH_MAX; i++)
        {
            folder[i] = '\0';
        }
        while (strcasecmp(holder2, "t_GreaterThanSign"))
        {
            if (!strcasecmp(holder2, "t_Astrix"))
            {
                for(int i = PATH_MAX - 1; i >= 0; i--)
                {
                    if (cwd[i] != '\\')
                    {
                         cwd[i] = '\0';
                    }
                    else
                    {
                        cwd[i] = '\0';
                        break;
                    }
                }
                strcpy(folder, cwd);
                fgets(holder2, BUFF_SIZE, fptr3);
                for(int i = 0; i < BUFF_SIZE + 1; i++)
                {
                    if (holder2[i] == '\n')
                    {
                        holder2[i] = '\0';
                    }
                }
                if (!strcasecmp(holder2, "t_ForwardSlash"))
                {
                     fgets(holder2, BUFF_SIZE, fptr3);
                    for(int i = 0; i < BUFF_SIZE + 1; i++)
                    {
                        if (holder2[i] == '\n')
                        {
                            holder2[i] = '\0';
                        }
                    }
                }
            }
            else if (strcasecmp(holder2, "t_GreaterThanSign"))
            {

                if (!strcasecmp(holder2, "t_ForwardSlash"))
                {
                    fgets(holder2, BUFF_SIZE, fptr3);
                    for(int i = 0; i < BUFF_SIZE + 1; i++)
                    {
                        if (holder2[i] == '\n')
                        {
                            holder2[i] = '\0';
                        }
                    }
                    if (!strcasecmp(holder2, "t_GreaterThanSign"))
                    {
                        break;
                    }
                }
                strcpy(folder, cwd);
                while (true)
                {
                    if(strcmp(holder2, ""))
                    {
                        strcat(folder, "\\");
                    }
                    strcat(folder, holder2);
                    fgets(holder2, BUFF_SIZE, fptr3);
                    for(int i = 0; i < BUFF_SIZE + 1; i++)
                    {
                        if (holder2[i] == '\n')
                        {
                            holder2[i] = '\0';
                        }
                    }
                    if (!strcasecmp(holder2, "t_ForwardSlash")) strcpy(holder2, "");
                    if (!strcasecmp(holder2, "t_GreaterThanSign"))
                    {
                        break;
                    }
                }
            }
        }
        struct stat sb;
        if (stat(folder, &sb) == 0 && S_ISDIR(sb.st_mode)) {
            printf("Path exists. Ifnot command will not be executed.\n");
            while (fgets(holder2, BUFF_SIZE, fptr3) != NULL){
            for(int i = 0; i < BUFF_SIZE + 1; i++){
                if (holder2[i] == '\n')
                {
                    holder2[i] = '\0';
                }
            }
            if (feof(fptr3)) {
                printf("Error. End of file reached without a command completing.\n");
                exit(0);}
            if (!(strcasecmp(holder2, "t_RightCurlyBrace"))) break;
        }
        }
        else {
        printf("Path: %s does not exist. Command following ifnot clause will execute.\n", folder);
        fgets(holder2, BUFF_SIZE, fptr3);
        for(int i = 0; i < BUFF_SIZE + 1; i++)
        {
            if (holder2[i] == '\n')
            {
                holder2[i] = '\0';
            }
        }
        if (feof(fptr3)) {
            printf("Error. End of file reached with a command completing.\n");
            exit(0);}
        if (!strcasecmp(holder2, "t_go"))
        {
            go(holder2, cwd, fptr3);
        }
        else if (!strcasecmp(holder2, "t_make"))
        {
            make(holder2, cwd, fptr3);
        }
        if (!strcasecmp(holder2, "t_LeftCurlyBrace"))
        {
            translate(holder2, fptr3, cwd);
            if (!(strcasecmp(holder2, "t_RightCurlyBrace")));
            else printf("Error. Left curly brace not closed with a right curly brace.\n");
        }
        }
    }
    else
    {
        printf("Error. 'if' statement should be followed by a path name: '<PATH_NAME>'.\n");
        exit(0);
    }
}
