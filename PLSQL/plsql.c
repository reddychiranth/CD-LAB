#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_TOKEN_LEN 100
#define MAX_BLOCKS 100
#define MAX_PARAMS 20

typedef enum {
    KEYWORD, IDENTIFIER, OPERATOR, NUMERIC_CONSTANT,
    STRING_LITERAL, SPECIAL_SYMBOL, DATATYPE,
    PACKAGE_NAME, PROCEDURE_NAME, FUNCTION_NAME,
    VARIABLE, PARAMETER
} TokenType;

typedef struct {
    char lexeme[MAX_TOKEN_LEN];
    TokenType type;
} Token;

typedef struct {
    char name[MAX_TOKEN_LEN];
    char type[MAX_TOKEN_LEN];  // PACKAGE, PROCEDURE, FUNCTION
    char parameters[MAX_PARAMS][MAX_TOKEN_LEN];
    char return_type[MAX_TOKEN_LEN];
    int param_count;
} Block;

Block symbolTable[MAX_BLOCKS];
int blockCount = 0;

// PL/SQL specific keywords
const char *keywords[] = {
    "PACKAGE", "PROCEDURE", "FUNCTION", "BEGIN", "END", "IF", "THEN", "ELSE",
    "ELSIF", "LOOP", "WHILE", "FOR", "IN", "OUT", "INOUT", "RETURN", "EXIT",
    "CONTINUE", "GOTO", "NULL", "RAISE", "DECLARE", "EXCEPTION", "WHEN",
    "OTHERS", "PRAGMA", "CREATE", "ALTER", "DROP", "SELECT", "INSERT", "UPDATE",
    "DELETE", "FROM", "WHERE", "GROUP", "HAVING", "ORDER", "BY", "SET",
    "VALUES", "INTO", "CURSOR", "TRIGGER", "BEFORE", "AFTER", "INSTEAD",
    "OF", "ON", "EACH", "ROW", "REFERENCING", "OLD", "NEW", "TABLE"
};
#define KEYWORDS_COUNT (sizeof(keywords) / sizeof(keywords[0]))

// PL/SQL datatypes
const char *datatypes[] = {
    "VARCHAR2", "NUMBER", "DATE", "TIMESTAMP", "BOOLEAN", "INTEGER", "FLOAT",
    "CHAR", "CLOB", "BLOB", "XMLTYPE", "REF", "CURSOR", "BINARY_INTEGER",
    "PLS_INTEGER", "NATURAL", "POSITIVE", "ROWID", "UROWID", "REAL"
};
#define DATATYPES_COUNT (sizeof(datatypes) / sizeof(datatypes[0]))

int isKeyword(const char *lexeme) {
    char upperLexeme[MAX_TOKEN_LEN];
    strcpy(upperLexeme, lexeme);
    for(int i = 0; upperLexeme[i]; i++) {
        upperLexeme[i] = toupper(upperLexeme[i]);
    }
    
    for (int i = 0; i < KEYWORDS_COUNT; i++) {
        if (strcmp(upperLexeme, keywords[i]) == 0) return 1;
    }
    return 0;
}

int isDatatype(const char *lexeme) {
    char upperLexeme[MAX_TOKEN_LEN];
    strcpy(upperLexeme, lexeme);
    for(int i = 0; upperLexeme[i]; i++) {
        upperLexeme[i] = toupper(upperLexeme[i]);
    }
    
    for (int i = 0; i < DATATYPES_COUNT; i++) {
        if (strcmp(upperLexeme, datatypes[i]) == 0) return 1;
    }
    return 0;
}

void skipComments(FILE *file) {
    char ch = fgetc(file);
    if (ch == '-' && (ch = fgetc(file)) == '-') {  // Single line comment
        while ((ch = fgetc(file)) != EOF && ch != '\n');
    } else if (ch == '/' && (ch = fgetc(file)) == '*') {  // Multi-line comment
        while ((ch = fgetc(file)) != EOF) {
            if (ch == '*' && (ch = fgetc(file)) == '/') break;
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
        if (ch == '-' || ch == '/') {
            ungetc(ch, file);
            skipComments(file);
            continue;
        }
        break;
    }

    if (ch == EOF) return 0;

    // Handle identifiers, keywords, datatypes
    if (isalpha(ch) || ch == '_' || ch == '$' || ch == '#') {
        buffer[bufIndex++] = ch;
        while ((ch = fgetc(file)) != EOF && 
               (isalnum(ch) || ch == '_' || ch == '$' || ch == '#')) {
            buffer[bufIndex++] = ch;
        }
        ungetc(ch, file);
        buffer[bufIndex] = '\0';
        
        if (isKeyword(buffer)) {
            token->type = KEYWORD;
        } else if (isDatatype(buffer)) {
            token->type = DATATYPE;
        } else {
            token->type = IDENTIFIER;
        }
        strcpy(token->lexeme, buffer);
        return 1;
    }

    // Handle numbers
    if (isdigit(ch) || ch == '.') {
        buffer[bufIndex++] = ch;
        while ((ch = fgetc(file)) != EOF && 
               (isdigit(ch) || ch == '.' || ch == 'e' || ch == 'E' || 
                ch == '+' || ch == '-')) {
            buffer[bufIndex++] = ch;
        }
        ungetc(ch, file);
        buffer[bufIndex] = '\0';
        token->type = NUMERIC_CONSTANT;
        strcpy(token->lexeme, buffer);
        return 1;
    }

    // Handle string literals
    if (ch == '\'' || ch == '"') {
        char quote = ch;
        buffer[bufIndex++] = ch;
        while ((ch = fgetc(file)) != EOF && ch != quote) {
            if (ch == quote && (ch = fgetc(file)) == quote) {  // Handle doubled quotes
                buffer[bufIndex++] = ch;
                buffer[bufIndex++] = ch;
                continue;
            }
            if (ch != quote) {
                buffer[bufIndex++] = ch;
            }
            if (ch == '\\') {
                ch = fgetc(file);
                buffer[bufIndex++] = ch;
            }
        }
        if (ch == quote) buffer[bufIndex++] = ch;
        buffer[bufIndex] = '\0';
        token->type = STRING_LITERAL;
        strcpy(token->lexeme, buffer);
        return 1;
    }

    // Handle operators and special symbols
    if (strchr("+-*/<>=!&|^%", ch)) {
        buffer[bufIndex++] = ch;
        ch = fgetc(file);
        if (strchr("=<>", ch)) {
            buffer[bufIndex++] = ch;
        } else {
            ungetc(ch, file);
        }
        buffer[bufIndex] = '\0';
        token->type = OPERATOR;
        strcpy(token->lexeme, buffer);
        return 1;
    }

    buffer[bufIndex++] = ch;
    buffer[bufIndex] = '\0';
    token->type = SPECIAL_SYMBOL;
    strcpy(token->lexeme, buffer);
    return 1;
}

void extractBlock(FILE *file, const char *blockType) {
    Token token;
    char blockName[MAX_TOKEN_LEN];
    char parameters[MAX_PARAMS][MAX_TOKEN_LEN];
    char paramTypes[MAX_PARAMS][MAX_TOKEN_LEN];
    int paramCount = 0;
    char returnType[MAX_TOKEN_LEN] = "";

    // Get block name
    if (getNextToken(file, &token) && token.type == IDENTIFIER) {
        strcpy(blockName, token.lexeme);
        
        // Check for parameters
        if (getNextToken(file, &token) && token.lexeme[0] == '(') {
            while (getNextToken(file, &token) && token.lexeme[0] != ')') {
                if (token.type == IDENTIFIER) {
                    strcpy(parameters[paramCount], token.lexeme);
                    
                    // Get parameter type
                    if (getNextToken(file, &token) && 
                        (token.type == DATATYPE || token.type == IDENTIFIER)) {
                        strcpy(paramTypes[paramCount], token.lexeme);
                        paramCount++;
                    }
                }
            }
        }

        // Check for return type (for functions)
        if (strcmp(blockType, "FUNCTION") == 0) {
            while (getNextToken(file, &token)) {
                if (token.type == RETURN) {
                    if (getNextToken(file, &token) && 
                        (token.type == DATATYPE || token.type == IDENTIFIER)) {
                        strcpy(returnType, token.lexeme);
                        break;
                    }
                }
                if (token.type == KEYWORD && 
                    (strcmp(token.lexeme, "IS") == 0 || 
                     strcmp(token.lexeme, "AS") == 0)) {
                    break;
                }
            }
        }

        // Store in symbol table
        strcpy(symbolTable[blockCount].name, blockName);
        strcpy(symbolTable[blockCount].type, blockType);
        symbolTable[blockCount].param_count = paramCount;
        for (int i = 0; i < paramCount; i++) {
            sprintf(symbolTable[blockCount].parameters[i], "%s %s", 
                    parameters[i], paramTypes[i]);
        }
        strcpy(symbolTable[blockCount].return_type, returnType);
        blockCount++;
    }
}

void analyzePLSQLFile(const char *filename) {
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
            case DATATYPE: printf("DATATYPE"); break;
            case PACKAGE_NAME: printf("PACKAGE NAME"); break;
            case PROCEDURE_NAME: printf("PROCEDURE NAME"); break;
            case FUNCTION_NAME: printf("FUNCTION NAME"); break;
            case VARIABLE: printf("VARIABLE"); break;
            case PARAMETER: printf("PARAMETER"); break;
            default: printf("UNKNOWN"); break;
        }
        printf("\n");

        if (token.type == KEYWORD) {
            if (strcmp(token.lexeme, "PACKAGE") == 0) {
                extractBlock(file, "PACKAGE");
            } else if (strcmp(token.lexeme, "PROCEDURE") == 0) {
                extractBlock(file, "PROCEDURE");
            } else if (strcmp(token.lexeme, "FUNCTION") == 0) {
                extractBlock(file, "FUNCTION");
            }
        }
    }

    fclose(file);
}

void displaySymbolTable() {
    printf("\nSymbol Table (PL/SQL Blocks):\n");
    printf("--------------------------------------\n");
    for (int i = 0; i < blockCount; i++) {
        printf("Type: %s\n", symbolTable[i].type);
        printf("Name: %s\n", symbolTable[i].name);
        if (symbolTable[i].param_count > 0) {
            printf("Parameters:\n");
            for (int j = 0; j < symbolTable[i].param_count; j++) {
                printf("  %s\n", symbolTable[i].parameters[j]);
            }
        }
        if (strlen(symbolTable[i].return_type) > 0) {
            printf("Return Type: %s\n", symbolTable[i].return_type);
        }
        printf("--------------------------------------\n");
    }
}

int main() {
    char filename[100];
    printf("Enter the PL/SQL file name: ");
    scanf("%s", filename);
    analyzePLSQLFile(filename);
    displaySymbolTable();
    return 0;
}