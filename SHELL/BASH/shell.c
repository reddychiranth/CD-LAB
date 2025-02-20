#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_TOKEN_LEN 100
#define MAX_FUNCTIONS 100
#define MAX_PARAMS 20

typedef enum {
    KEYWORD, IDENTIFIER, OPERATOR, NUMERIC_CONSTANT,
    STRING_LITERAL, SPECIAL_SYMBOL, VARIABLE,
    COMMAND, PARAMETER, REDIRECTION, PIPE,
    FUNCTION_NAME, ENV_VARIABLE, SHEBANG
} TokenType;

typedef struct {
    char lexeme[MAX_TOKEN_LEN];
    TokenType type;
} Token;

typedef struct {
    char name[MAX_TOKEN_LEN];
    char parameters[MAX_PARAMS][MAX_TOKEN_LEN];
    int param_count;
} Function;

Function symbolTable[MAX_FUNCTIONS];
int functionCount = 0;

// Shell keywords
const char *keywords[] = {
    "if", "then", "else", "elif", "fi", "case", "esac",
    "for", "while", "until", "do", "done", "in", "select",
    "function", "time", "coproc", "return", "break", "continue",
    "eval", "exec", "exit", "export", "readonly", "set", "unset",
    "shift", "source", "alias", "declare", "local", "typeset"
};
#define KEYWORDS_COUNT (sizeof(keywords) / sizeof(keywords[0]))

// Built-in commands
const char *commands[] = {
    "echo", "cd", "pwd", "ls", "mkdir", "rmdir", "touch",
    "cp", "mv", "rm", "cat", "grep", "sed", "awk", "find",
    "tar", "gzip", "chmod", "chown", "kill", "ps", "top"
};
#define COMMANDS_COUNT (sizeof(commands) / sizeof(commands[0]))

int isKeyword(const char *lexeme) {
    for (int i = 0; i < KEYWORDS_COUNT; i++) {
        if (strcmp(lexeme, keywords[i]) == 0) return 1;
    }
    return 0;
}

int isCommand(const char *lexeme) {
    for (int i = 0; i < COMMANDS_COUNT; i++) {
        if (strcmp(lexeme, commands[i]) == 0) return 1;
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
        if (ch == '#' && bufIndex == 0) {
            // Check for shebang
            if ((ch = fgetc(file)) == '!') {
                buffer[bufIndex++] = '#';
                buffer[bufIndex++] = '!';
                while ((ch = fgetc(file)) != EOF && ch != '\n') {
                    buffer[bufIndex++] = ch;
                }
                buffer[bufIndex] = '\0';
                token->type = SHEBANG;
                strcpy(token->lexeme, buffer);
                return 1;
            } else {
                ungetc(ch, file);
                skipComments(file);
                continue;
            }
        }
        break;
    }

    if (ch == EOF) return 0;

    // Handle variables ($var, ${var}, $1, etc.)
    if (ch == '$') {
        buffer[bufIndex++] = ch;
        ch = fgetc(file);
        if (ch == '{') {
            buffer[bufIndex++] = ch;
            while ((ch = fgetc(file)) != EOF && ch != '}') {
                buffer[bufIndex++] = ch;
            }
            if (ch == '}') buffer[bufIndex++] = ch;
        } else if (isdigit(ch) || ch == '#' || ch == '@' || ch == '*' || 
                  ch == '?' || ch == '-' || ch == '$' || ch == '!') {
            buffer[bufIndex++] = ch;
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
    if (ch == '>' || ch == '<' || ch == '|') {
        buffer[bufIndex++] = ch;
        ch = fgetc(file);
        if ((buffer[0] == '>' && ch == '>') || 
            (buffer[0] == '<' && ch == '<')) {
            buffer[bufIndex++] = ch;
        } else {
            ungetc(ch, file);
        }
        buffer[bufIndex] = '\0';
        token->type = (buffer[0] == '|') ? PIPE : REDIRECTION;
        strcpy(token->lexeme, buffer);
        return 1;
    }

    // Handle command substitution $(command) or `command`
    if (ch == '`' || (ch == '$' && (ch = fgetc(file)) == '(')) {
        char end = (ch == '`') ? '`' : ')';
        buffer[bufIndex++] = '$';
        if (end == ')') buffer[bufIndex++] = '(';
        while ((ch = fgetc(file)) != EOF && ch != end) {
            buffer[bufIndex++] = ch;
        }
        if (ch == end) buffer[bufIndex++] = ch;
        buffer[bufIndex] = '\0';
        token->type = COMMAND;
        strcpy(token->lexeme, buffer);
        return 1;
    }

    // Handle identifiers, keywords, and commands
    if (isalpha(ch) || ch == '_') {
        buffer[bufIndex++] = ch;
        while ((ch = fgetc(file)) != EOF && 
               (isalnum(ch) || ch == '_')) {
            buffer[bufIndex++] = ch;
        }
        ungetc(ch, file);
        buffer[bufIndex] = '\0';
        
        if (isKeyword(buffer)) {
            token->type = KEYWORD;
        } else if (isCommand(buffer)) {
            token->type = COMMAND;
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

void extractFunction(FILE *file) {
    Token token;
    char functionName[MAX_TOKEN_LEN];
    char parameters[MAX_PARAMS][MAX_TOKEN_LEN];
    int paramCount = 0;

    // Get function name
    if (getNextToken(file, &token) && token.type == IDENTIFIER) {
        strcpy(functionName, token.lexeme);
        
        // Look for () or parameters
        if (getNextToken(file, &token)) {
            if (token.lexeme[0] == '(') {
                getNextToken(file, &token); // Skip )
            } else if (token.type == PARAMETER) {
                strcpy(parameters[paramCount++], token.lexeme);
                while (getNextToken(file, &token) && token.type == PARAMETER) {
                    strcpy(parameters[paramCount++], token.lexeme);
                }
            }
        }

        // Store in symbol table
        strcpy(symbolTable[functionCount].name, functionName);
        symbolTable[functionCount].param_count = paramCount;
        for (int i = 0; i < paramCount; i++) {
            strcpy(symbolTable[functionCount].parameters[i], parameters[i]);
        }
        functionCount++;
    }
}

void analyzeShellFile(const char *filename) {
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
            case PARAMETER: printf("PARAMETER"); break;
            case REDIRECTION: printf("REDIRECTION"); break;
            case PIPE: printf("PIPE"); break;
            case FUNCTION_NAME: printf("FUNCTION NAME"); break;
            case ENV_VARIABLE: printf("ENVIRONMENT VARIABLE"); break;
            case SHEBANG: printf("SHEBANG"); break;
            default: printf("UNKNOWN"); break;
        }
        printf("\n");

        if (token.type == KEYWORD && strcmp(token.lexeme, "function") == 0) {
            extractFunction(file);
        }
    }

    fclose(file);
}

void displaySymbolTable() {
    printf("\nShell Script Analysis:\n");
    printf("------------------------\n");
    for (int i = 0; i < functionCount; i++) {
        printf("Function: %s\n", symbolTable[i].name);
        if (symbolTable[i].param_count > 0) {
            printf("Parameters: ");
            for (int j = 0; j < symbolTable[i].param_count; j++) {
                printf("%s", symbolTable[i].parameters[j]);
                if (j < symbolTable[i].param_count - 1) printf(", ");
            }
            printf("\n");
        }
        printf("------------------------\n");
    }
}

int main() {
    char filename[100];
    printf("Enter the Shell script name: ");
    scanf("%s", filename);
    analyzeShellFile(filename);
    displaySymbolTable();
    return 0;
}