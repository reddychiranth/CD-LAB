#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_TOKEN_LEN 100
#define MAX_FUNCTIONS 100
#define MAX_PARAMS 20

typedef enum {
    KEYWORD, IDENTIFIER, OPERATOR, NUMERIC_CONSTANT,
    STRING_LITERAL, SPECIAL_SYMBOL, FUNCTION_NAME,
    MATRIX_OPERATOR, SCIENTIFIC_FUNCTION, COMMAND
} TokenType;

typedef struct {
    char lexeme[MAX_TOKEN_LEN];
    TokenType type;
} Token;

typedef struct {
    char name[MAX_TOKEN_LEN];
    char parameters[MAX_PARAMS][MAX_TOKEN_LEN];
    int param_count;
    int is_script;  // 1 if script file, 0 if function
} Function;

Function symbolTable[MAX_FUNCTIONS];
int functionCount = 0;

// MATLAB keywords
const char *keywords[] = {
    "function", "end", "if", "else", "elseif", "while", "for",
    "break", "continue", "return", "switch", "case", "otherwise",
    "try", "catch", "global", "persistent", "classdef", "properties",
    "methods", "enumeration", "events", "parfor", "spmd"
};
#define KEYWORDS_COUNT (sizeof(keywords) / sizeof(keywords[0]))

// MATLAB scientific functions
const char *scientific_functions[] = {
    "sin", "cos", "tan", "exp", "log", "sqrt", "abs", "real",
    "imag", "angle", "conj", "round", "floor", "ceil", "fix",
    "mean", "std", "var", "max", "min", "sum", "prod"
};
#define SCIENTIFIC_FUNCTIONS_COUNT (sizeof(scientific_functions) / sizeof(scientific_functions[0]))

// MATLAB matrix operators
const char *matrix_operators[] = {
    ".*", ".^", "./", ".\\", "'", ".'", "*", "/", "\\", "^",
    "+=", "-=", "*=", "/=", ".+=", ".-=", ".*=", "./="
};
#define MATRIX_OPERATORS_COUNT (sizeof(matrix_operators) / sizeof(matrix_operators[0]))

int isKeyword(const char *lexeme) {
    for (int i = 0; i < KEYWORDS_COUNT; i++) {
        if (strcmp(lexeme, keywords[i]) == 0) return 1;
    }
    return 0;
}

int isScientificFunction(const char *lexeme) {
    for (int i = 0; i < SCIENTIFIC_FUNCTIONS_COUNT; i++) {
        if (strcmp(lexeme, scientific_functions[i]) == 0) return 1;
    }
    return 0;
}

int isMatrixOperator(const char *lexeme) {
    for (int i = 0; i < MATRIX_OPERATORS_COUNT; i++) {
        if (strcmp(lexeme, matrix_operators[i]) == 0) return 1;
    }
    return 0;
}

void skipComments(FILE *file) {
    char ch = fgetc(file);
    if (ch == '%') {  // Single line comment
        while ((ch = fgetc(file)) != EOF && ch != '\n');
    } else if (ch == '{' && (ch = fgetc(file)) == '%') {  // Block comment
        while ((ch = fgetc(file)) != EOF) {
            if (ch == '%' && (ch = fgetc(file)) == '}') break;
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
        if (ch == '%' || ch == '{') {
            ungetc(ch, file);
            skipComments(file);
            continue;
        }
        break;
    }

    if (ch == EOF) return 0;

    // Handle identifiers and keywords
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
        } else if (isScientificFunction(buffer)) {
            token->type = SCIENTIFIC_FUNCTION;
        } else {
            token->type = IDENTIFIER;
        }
        strcpy(token->lexeme, buffer);
        return 1;
    }

    // Handle numbers (including complex and scientific notation)
    if (isdigit(ch) || ch == '.') {
        buffer[bufIndex++] = ch;
        int hasDecimal = (ch == '.');
        
        while ((ch = fgetc(file)) != EOF) {
            if (isdigit(ch)) {
                buffer[bufIndex++] = ch;
            } else if (ch == '.' && !hasDecimal) {
                buffer[bufIndex++] = ch;
                hasDecimal = 1;
            } else if ((ch == 'e' || ch == 'E') && 
                      (isdigit(buffer[bufIndex-1]) || buffer[bufIndex-1] == '.')) {
                buffer[bufIndex++] = ch;
                ch = fgetc(file);
                if (ch == '+' || ch == '-') {
                    buffer[bufIndex++] = ch;
                    ch = fgetc(file);
                }
                if (isdigit(ch)) {
                    buffer[bufIndex++] = ch;
                    while ((ch = fgetc(file)) != EOF && isdigit(ch)) {
                        buffer[bufIndex++] = ch;
                    }
                }
                break;
            } else if (ch == 'i' || ch == 'j') {  // Complex numbers
                buffer[bufIndex++] = ch;
                break;
            } else {
                break;
            }
        }
        ungetc(ch, file);
        buffer[bufIndex] = '\0';
        token->type = NUMERIC_CONSTANT;
        strcpy(token->lexeme, buffer);
        return 1;
    }

    // Handle strings (both single and double quotes)
    if (ch == '\'' || ch == '"') {
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

    // Handle matrix operators and other operators
    if (strchr("+-*/<>=!&|^%.[\\]", ch)) {
        buffer[bufIndex++] = ch;
        ch = fgetc(file);
        if (ch == '.' || ch == '=' || ch == '*' || ch == '/' || 
            ch == '\\' || ch == '+' || ch == '-' || ch == '\'' ||
            ch == '&' || ch == '|') {
            buffer[bufIndex++] = ch;
        } else {
            ungetc(ch, file);
        }
        buffer[bufIndex] = '\0';
        
        if (isMatrixOperator(buffer)) {
            token->type = MATRIX_OPERATOR;
        } else {
            token->type = OPERATOR;
        }
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
    int isScript = 1;  // Assume script until function keyword found

    // Check if it's a function or script
    if (getNextToken(file, &token) && token.type == KEYWORD && 
        strcmp(token.lexeme, "function") == 0) {
        isScript = 0;
        
        // Handle output arguments
        if (getNextToken(file, &token)) {
            if (token.lexeme[0] == '[') {
                while (getNextToken(file, &token) && token.lexeme[0] != ']');
                getNextToken(file, &token);
            }
            
            if (token.type == IDENTIFIER) {
                strcpy(functionName, token.lexeme);
                
                // Handle input arguments
                if (getNextToken(file, &token) && token.lexeme[0] == '(') {
                    while (getNextToken(file, &token) && token.lexeme[0] != ')') {
                        if (token.type == IDENTIFIER) {
                            strcpy(parameters[paramCount++], token.lexeme);
                        }
                    }
                }
            }
        }
    } else {
        // It's a script file
        strcpy(functionName, "script");
    }

    // Store in symbol table
    strcpy(symbolTable[functionCount].name, functionName);
    symbolTable[functionCount].param_count = paramCount;
    symbolTable[functionCount].is_script = isScript;
    for (int i = 0; i < paramCount; i++) {
        strcpy(symbolTable[functionCount].parameters[i], parameters[i]);
    }
    functionCount++;
}

void analyzeMATLABFile(const char *filename) {
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

    // First pass to determine if it's a function or script
    extractFunction(file);
    rewind(file);

    // Second pass for detailed analysis
    while (getNextToken(file, &token)) {
        printf("%s\t\t", token.lexeme);
        switch (token.type) {
            case KEYWORD: printf("KEYWORD"); break;
            case IDENTIFIER: printf("IDENTIFIER"); break;
            case OPERATOR: printf("OPERATOR"); break;
            case NUMERIC_CONSTANT: printf("NUMERIC CONSTANT"); break;
            case STRING_LITERAL: printf("STRING LITERAL"); break;
            case SPECIAL_SYMBOL: printf("SPECIAL SYMBOL"); break;
            case FUNCTION_NAME: printf("FUNCTION NAME"); break;
            case MATRIX_OPERATOR: printf("MATRIX OPERATOR"); break;
            case SCIENTIFIC_FUNCTION: printf("SCIENTIFIC FUNCTION"); break;
            case COMMAND: printf("COMMAND"); break;
            default: printf("UNKNOWN"); break;
        }
        printf("\n");
    }

    fclose(file);
}

void displaySymbolTable() {
    printf("\nMATLAB Analysis:\n");
    printf("------------------------\n");
    for (int i = 0; i < functionCount; i++) {
        if (symbolTable[i].is_script) {
            printf("Script File\n");
        } else {
            printf("Function: %s\n", symbolTable[i].name);
            if (symbolTable[i].param_count > 0) {
                printf("Parameters: ");
                for (int j = 0; j < symbolTable[i].param_count; j++) {
                    printf("%s", symbolTable[i].parameters[j]);
                    if (j < symbolTable[i].param_count - 1) printf(", ");
                }
                printf("\n");
            }
        }
        printf("------------------------\n");
    }
}

int main() {
    char filename[100];
    printf("Enter the MATLAB file name: ");
    scanf("%s", filename);
    analyzeMATLABFile(filename);
    displaySymbolTable();
    return 0;
}