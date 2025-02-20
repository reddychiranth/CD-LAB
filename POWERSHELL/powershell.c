#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_TOKEN_LEN 100
#define MAX_FUNCTIONS 100
#define MAX_PARAMS 10

typedef enum {
    KEYWORD, IDENTIFIER, OPERATOR, NUMERIC_CONSTANT, 
    STRING_LITERAL, SPECIAL_SYMBOL, VARIABLE, CMDLET, 
    PARAMETER
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

// PowerShell specific keywords and cmdlets
const char *keywords[] = {
    "function", "if", "else", "elseif", "while", "do", "for",
    "foreach", "switch", "break", "continue", "return", "param",
    "begin", "process", "end", "try", "catch", "finally",
    "throw", "trap", "using", "class", "enum", "exit"
};
#define KEYWORDS_COUNT (sizeof(keywords) / sizeof(keywords[0]))

const char *cmdlets[] = {
    "Get-Item", "Set-Item", "New-Item", "Remove-Item",
    "Get-Content", "Set-Content", "Write-Host", "Write-Output",
    "Get-Process", "Start-Process", "Stop-Process",
    "Get-Service", "Start-Service", "Stop-Service"
};
#define CMDLETS_COUNT (sizeof(cmdlets) / sizeof(cmdlets[0]))

int isKeyword(const char *lexeme) {
    for (int i = 0; i < KEYWORDS_COUNT; i++) {
        if (strcasecmp(lexeme, keywords[i]) == 0) return 1;
    }
    return 0;
}

int isCmdlet(const char *lexeme) {
    for (int i = 0; i < CMDLETS_COUNT; i++) {
        if (strcasecmp(lexeme, cmdlets[i]) == 0) return 1;
    }
    return 0;
}

void skipComments(FILE *file) {
    char ch = fgetc(file);
    if (ch == '#') {  // Single line comment
        while ((ch = fgetc(file)) != EOF && ch != '\n');
    } else if (ch == '<' && (ch = fgetc(file)) == '#') {  // Multi-line comment
        while ((ch = fgetc(file)) != EOF) {
            if (ch == '#' && (ch = fgetc(file)) == '>') break;
        }
    } else {
        ungetc(ch, file);
    }
}

int getNextToken(FILE *file, Token *token) {
    char ch;
    char buffer[MAX_TOKEN_LEN];
    int bufIndex = 0;

    while ((ch = fgetc(file)) != EOF) {
        if (isspace(ch)) continue;
        if (ch == '#' || ch == '<') {
            ungetc(ch, file);
            skipComments(file);
            continue;
        }
        break;
    }

    if (ch == EOF) return 0;

    // Handle variables (starting with $)
    if (ch == '$') {
        buffer[bufIndex++] = ch;
        while ((ch = fgetc(file)) != EOF && (isalnum(ch) || ch == '_')) {
            buffer[bufIndex++] = ch;
        }
        ungetc(ch, file);
        buffer[bufIndex] = '\0';
        token->type = VARIABLE;
        strcpy(token->lexeme, buffer);
        return 1;
    }

    // Handle parameters (starting with -)
    if (ch == '-') {
        buffer[bufIndex++] = ch;
        while ((ch = fgetc(file)) != EOF && (isalnum(ch) || ch == '_')) {
            buffer[bufIndex++] = ch;
        }
        ungetc(ch, file);
        buffer[bufIndex] = '\0';
        token->type = PARAMETER;
        strcpy(token->lexeme, buffer);
        return 1;
    }

    // Handle identifiers, keywords, and cmdlets
    if (isalpha(ch) || ch == '_') {
        buffer[bufIndex++] = ch;
        while ((ch = fgetc(file)) != EOF && (isalnum(ch) || ch == '_' || ch == '-')) {
            buffer[bufIndex++] = ch;
        }
        ungetc(ch, file);
        buffer[bufIndex] = '\0';
        
        if (isKeyword(buffer)) {
            token->type = KEYWORD;
        } else if (isCmdlet(buffer)) {
            token->type = CMDLET;
        } else {
            token->type = IDENTIFIER;
        }
        strcpy(token->lexeme, buffer);
        return 1;
    }

    // Handle numbers
    if (isdigit(ch)) {
        buffer[bufIndex++] = ch;
        while ((ch = fgetc(file)) != EOF && (isdigit(ch) || ch == '.')) {
            buffer[bufIndex++] = ch;
        }
        ungetc(ch, file);
        buffer[bufIndex] = '\0';
        token->type = NUMERIC_CONSTANT;
        strcpy(token->lexeme, buffer);
        return 1;
    }

    // Handle strings (single and double quotes)
    if (ch == '"' || ch == '\'') {
        char quote = ch;
        buffer[bufIndex++] = ch;
        while ((ch = fgetc(file)) != EOF && ch != quote) {
            if (ch == '`') {  // Handle PowerShell escape character
                ch = fgetc(file);
                if (ch == EOF) break;
            }
            buffer[bufIndex++] = ch;
        }
        if (ch == quote) buffer[bufIndex++] = ch;
        buffer[bufIndex] = '\0';
        token->type = STRING_LITERAL;
        strcpy(token->lexeme, buffer);
        return 1;
    }

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

    // Skip to function name
    if (getNextToken(file, &token) && token.type == IDENTIFIER) {
        strcpy(functionName, token.lexeme);
        
        // Look for param block or parameters
        if (getNextToken(file, &token)) {
            if (strcmp(token.lexeme, "(") == 0) {
                // Parse parameters
                while (getNextToken(file, &token) && strcmp(token.lexeme, ")") != 0) {
                    if (token.type == VARIABLE) {
                        strcpy(parameters[paramCount++], token.lexeme);
                    }
                }
            } else if (token.type == KEYWORD && strcmp(token.lexeme, "param") == 0) {
                // Parse param block
                if (getNextToken(file, &token) && strcmp(token.lexeme, "(") == 0) {
                    while (getNextToken(file, &token) && strcmp(token.lexeme, ")") != 0) {
                        if (token.type == VARIABLE) {
                            strcpy(parameters[paramCount++], token.lexeme);
                        }
                    }
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

void analyzePowerShellFile(const char *filename) {
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
            case CMDLET: printf("CMDLET"); break;
            case PARAMETER: printf("PARAMETER"); break;
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
    printf("\nSymbol Table (Functions and Parameters):\n");
    printf("--------------------------------------\n");
    for (int i = 0; i < functionCount; i++) {
        printf("Function: %s(", symbolTable[i].name);
        for (int j = 0; j < symbolTable[i].param_count; j++) {
            printf("%s", symbolTable[i].parameters[j]);
            if (j < symbolTable[i].param_count - 1) printf(", ");
        }
        printf(")\n");
    }
}

int main() {
    char filename[100];
    printf("Enter the PowerShell file name: ");
    scanf("%s", filename);
    analyzePowerShellFile(filename);
    displaySymbolTable();
    return 0;
}