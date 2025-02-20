#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_TOKEN_LEN 100
#define MAX_ALIASES 100
#define MAX_PARAMS 20

typedef enum {
    KEYWORD, IDENTIFIER, OPERATOR, NUMERIC_CONSTANT,
    STRING_LITERAL, SPECIAL_SYMBOL, VARIABLE,
    COMMAND, ALIAS, REDIRECTION, PIPE,
    BUILTIN_COMMAND, ENV_VARIABLE, HISTORY_REF
} TokenType;

typedef struct {
    char lexeme[MAX_TOKEN_LEN];
    TokenType type;
} Token;

typedef struct {
    char name[MAX_TOKEN_LEN];
    char command[MAX_TOKEN_LEN];
} Alias;

Alias symbolTable[MAX_ALIASES];
int aliasCount = 0;

// C Shell specific keywords
const char *keywords[] = {
    "alias", "unalias", "if", "then", "else", "endif", "foreach",
    "end", "while", "switch", "case", "breaksw", "default", "goto",
    "continue", "setenv", "unsetenv", "source", "rehash", "repeat",
    "exit", "cd", "popd", "pushd", "dirs", "umask", "history"
};
#define KEYWORDS_COUNT (sizeof(keywords) / sizeof(keywords[0]))

// C Shell built-in commands
const char *builtins[] = {
    "echo", "pwd", "ls", "mkdir", "rmdir", "touch", "cp", "mv",
    "rm", "cat", "grep", "set", "unset", "time", "nice", "nohup",
    "kill", "jobs", "fg", "bg", "wait", "which", "where"
};
#define BUILTINS_COUNT (sizeof(builtins) / sizeof(builtins[0]))

int isKeyword(const char *lexeme) {
    for (int i = 0; i < KEYWORDS_COUNT; i++) {
        if (strcmp(lexeme, keywords[i]) == 0) return 1;
    }
    return 0;
}

int isBuiltin(const char *lexeme) {
    for (int i = 0; i < BUILTINS_COUNT; i++) {
        if (strcmp(lexeme, builtins[i]) == 0) return 1;
    }
    return 0;
}

void skipComments(FILE *file) {
    char ch;
    while ((ch = fgetc(file)) != EOF && ch != '\n');
}

int getNextToken(FILE *file, Token *token) {
    char ch;
    char buffer[MAX_TOKEN_LEN];
    int bufIndex = 0;

    while ((ch = fgetc(file)) != EOF) {
        if (isspace(ch)) continue;
        if (ch == '#') {
            skipComments(file);
            continue;
        }
        break;
    }

    if (ch == EOF) return 0;

    // Handle history references (!, !!, !$, !*)
    if (ch == '!') {
        buffer[bufIndex++] = ch;
        ch = fgetc(file);
        if (ch == '!' || ch == '$' || ch == '*' || isdigit(ch) || isalpha(ch)) {
            buffer[bufIndex++] = ch;
            if (isalpha(ch)) {
                while ((ch = fgetc(file)) != EOF && isalnum(ch)) {
                    buffer[bufIndex++] = ch;
                }
                ungetc(ch, file);
            }
        } else {
            ungetc(ch, file);
        }
        buffer[bufIndex] = '\0';
        token->type = HISTORY_REF;
        strcpy(token->lexeme, buffer);
        return 1;
    }

    // Handle variables ($var, $?var, ${var})
    if (ch == '$') {
        buffer[bufIndex++] = ch;
        ch = fgetc(file);
        if (ch == '?' || ch == '#' || ch == '$' || ch == '!') {
            buffer[bufIndex++] = ch;
        } else if (ch == '{') {
            buffer[bufIndex++] = ch;
            while ((ch = fgetc(file)) != EOF && ch != '}') {
                buffer[bufIndex++] = ch;
            }
            if (ch == '}') buffer[bufIndex++] = ch;
        } else {
            while (ch != EOF && (isalnum(ch) || ch == '_')) {
                buffer[bufIndex++] = ch;
                ch = fgetc(file);
            }
            ungetc(ch, file);
        }
        buffer[bufIndex] = '\0';
        token->type = VARIABLE;
        strcpy(token->lexeme, buffer);
        return 1;
    }

    // Handle strings (both single and double quotes)
    if (ch == '"' || ch == '\'') {
        char quote = ch;
        buffer[bufIndex++] = ch;
        while ((ch = fgetc(file)) != EOF && ch != quote) {
            if (ch == '\\') {
                buffer[bufIndex++] = ch;
                ch = fgetc(file);
            }
            buffer[bufIndex++] = ch;
        }
        if (ch == quote) buffer[bufIndex++] = ch;
        buffer[bufIndex] = '\0';
        token->type = STRING_LITERAL;
        strcpy(token->lexeme, buffer);
        return 1;
    }

    // Handle redirections and pipes
    if (ch == '>' || ch == '<' || ch == '|' || ch == '&') {
        buffer[bufIndex++] = ch;
        ch = fgetc(file);
        if ((buffer[0] == '>' && (ch == '>' || ch == '&')) || 
            (buffer[0] == '<' && (ch == '<' || ch == '&')) ||
            (buffer[0] == '|' && ch == '&') ||
            (buffer[0] == '&' && ch == '&')) {
            buffer[bufIndex++] = ch;
        } else {
            ungetc(ch, file);
        }
        buffer[bufIndex] = '\0';
        token->type = (buffer[0] == '|') ? PIPE : REDIRECTION;
        strcpy(token->lexeme, buffer);
        return 1;
    }

    // Handle identifiers, keywords, and commands
    if (isalpha(ch) || ch == '_' || ch == '.') {
        buffer[bufIndex++] = ch;
        while ((ch = fgetc(file)) != EOF && 
               (isalnum(ch) || ch == '_' || ch == '.')) {
            buffer[bufIndex++] = ch;
        }
        ungetc(ch, file);
        buffer[bufIndex] = '\0';
        
        if (isKeyword(buffer)) {
            token->type = KEYWORD;
        } else if (isBuiltin(buffer)) {
            token->type = BUILTIN_COMMAND;
        } else {
            token->type = IDENTIFIER;
        }
        strcpy(token->lexeme, buffer);
        return 1;
    }

    // Handle numbers
    if (isdigit(ch)) {
        buffer[bufIndex++] = ch;
        while ((ch = fgetc(file)) != EOF && isdigit(ch)) {
            buffer[bufIndex++] = ch;
        }
        ungetc(ch, file);
        buffer[bufIndex] = '\0';
        token->type = NUMERIC_CONSTANT;
        strcpy(token->lexeme, buffer);
        return 1;
    }

    // Handle operators and special characters
    buffer[bufIndex++] = ch;
    buffer[bufIndex] = '\0';
    token->type = SPECIAL_SYMBOL;
    strcpy(token->lexeme, buffer);
    return 1;
}

void extractAlias(FILE *file) {
    Token token;
    char aliasName[MAX_TOKEN_LEN];
    char aliasCommand[MAX_TOKEN_LEN];

    // Get alias name
    if (getNextToken(file, &token) && token.type == IDENTIFIER) {
        strcpy(aliasName, token.lexeme);
        
        // Get alias command
        if (getNextToken(file, &token)) {
            strcpy(aliasCommand, token.lexeme);
            while (getNextToken(file, &token) && token.lexeme[0] != '\n') {
                strcat(aliasCommand, " ");
                strcat(aliasCommand, token.lexeme);
            }
            
            // Store in symbol table
            strcpy(symbolTable[aliasCount].name, aliasName);
            strcpy(symbolTable[aliasCount].command, aliasCommand);
            aliasCount++;
        }
    }
}

void analyzeCShellFile(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        return;
    }

    Token token;
    printf("\nLexical Analysis Output:\n");
    printf("------------------------\n");
    printf("Lexeme\t\tType\n");
    printf("------------------------\n");

    while (getNextToken(file, &token)) {
        printf("%s\t\t", token.lexeme);
        switch (token.type) {
            case KEYWORD: printf("KEYWORD"); break;
            case IDENTIFIER: printf("IDENTIFIER"); break;
            case OPERATOR: printf("OPERATOR"); break;
            case NUMERIC_CONSTANT: printf("NUMERIC CONSTANT"); break;
            case STRING_LITERAL: printf("STRING LITERAL"); break;
            case SPECIAL_SYMBOL: printf("SPECIAL SYMBOL"); break;
            case VARIABLE: printf("VARIABLE"); break;
            case COMMAND: printf("COMMAND"); break;
            case ALIAS: printf("ALIAS"); break;
            case REDIRECTION: printf("REDIRECTION"); break;
            case PIPE: printf("PIPE"); break;
            case BUILTIN_COMMAND: printf("BUILTIN COMMAND"); break;
            case ENV_VARIABLE: printf("ENVIRONMENT VARIABLE"); break;
            case HISTORY_REF: printf("HISTORY REFERENCE"); break;
            default: printf("UNKNOWN"); break;
        }
        printf("\n");

        // Process aliases
        if (token.type == KEYWORD && strcmp(token.lexeme, "alias") == 0) {
            extractAlias(file);
        }
    }

    fclose(file);
}

void displaySymbolTable() {
    printf("\nC Shell Alias Table:\n");
    printf("------------------------\n");
    for (int i = 0; i < aliasCount; i++) {
        printf("Alias: %s\n", symbolTable[i].name);
        printf("Command: %s\n", symbolTable[i].command);
        printf("------------------------\n");
    }
}

int main() {
    char filename[100];
    printf("Enter the C Shell script name: ");
    scanf("%s", filename);
    analyzeCShellFile(filename);
    displaySymbolTable();
    return 0;
}