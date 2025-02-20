#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_TOKEN_LEN 100
#define MAX_QUERIES 100
#define MAX_COLUMNS 20

typedef enum {
    KEYWORD, IDENTIFIER, OPERATOR, NUMERIC_CONSTANT, 
    STRING_LITERAL, SPECIAL_SYMBOL, FUNCTION
} TokenType;

typedef struct {
    char lexeme[MAX_TOKEN_LEN];
    TokenType type;
} Token;

typedef struct {
    char queryType[MAX_TOKEN_LEN];     // SELECT, INSERT, UPDATE, etc.
    char tableName[MAX_TOKEN_LEN];     // Target table
    char columns[MAX_COLUMNS][MAX_TOKEN_LEN];  // Columns involved
    int column_count;
} SQLQuery;

SQLQuery symbolTable[MAX_QUERIES];
int queryCount = 0;

// SQL specific keywords
const char *keywords[] = { 
    "SELECT", "FROM", "WHERE", "INSERT", "INTO", "VALUES",
    "UPDATE", "SET", "DELETE", "CREATE", "TABLE", "DROP",
    "ALTER", "INDEX", "GROUP", "BY", "HAVING", "ORDER",
    "JOIN", "LEFT", "RIGHT", "INNER", "OUTER", "ON",
    "AND", "OR", "NOT", "NULL", "IS", "IN", "BETWEEN"
};
#define KEYWORDS_COUNT (sizeof(keywords) / sizeof(keywords[0]))

// SQL specific operators
const char *operators[] = {
    "=", "<", ">", "<=", ">=", "<>", "!=", "+", "-", "*", "/"
};
#define OPERATORS_COUNT (sizeof(operators) / sizeof(operators[0]))

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

int isOperator(const char *lexeme) {
    for (int i = 0; i < OPERATORS_COUNT; i++) {
        if (strcmp(lexeme, operators[i]) == 0) return 1;
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

    // Skip whitespace and comments
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

    // Handle identifiers and keywords
    if (isalpha(ch) || ch == '_' || ch == '@') {
        buffer[bufIndex++] = ch;
        while ((ch = fgetc(file)) != EOF && (isalnum(ch) || ch == '_' || ch == '@')) {
            buffer[bufIndex++] = ch;
        }
        ungetc(ch, file);
        buffer[bufIndex] = '\0';
        token->type = isKeyword(buffer) ? KEYWORD : IDENTIFIER;
        strcpy(token->lexeme, buffer);
        return 1;
    }

    // Handle numbers
    if (isdigit(ch) || ch == '.') {
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

    // Handle string literals
    if (ch == '\'' || ch == '"') {
        char quote = ch;
        buffer[bufIndex++] = ch;
        while ((ch = fgetc(file)) != EOF && ch != quote) {
            buffer[bufIndex++] = ch;
        }
        buffer[bufIndex++] = quote;
        buffer[bufIndex] = '\0';
        token->type = STRING_LITERAL;
        strcpy(token->lexeme, buffer);
        return 1;
    }

    // Handle operators and special symbols
    buffer[bufIndex++] = ch;
    if (ch == '<' || ch == '>' || ch == '!' || ch == '=') {
        ch = fgetc(file);
        if (ch == '=' || (ch == '>' && buffer[0] == '<')) {
            buffer[bufIndex++] = ch;
        } else {
            ungetc(ch, file);
        }
    }
    buffer[bufIndex] = '\0';
    
    token->type = isOperator(buffer) ? OPERATOR : SPECIAL_SYMBOL;
    strcpy(token->lexeme, buffer);
    return 1;
}

void extractQuery(FILE *file, Token firstToken) {
    Token token;
    SQLQuery currentQuery = {0};
    int columnIndex = 0;
    
    // Store query type (SELECT, INSERT, etc.)
    strcpy(currentQuery.queryType, firstToken.lexeme);
    
    // Parse the query
    while (getNextToken(file, &token)) {
        if (token.type == KEYWORD) {
            if (strcmp(token.lexeme, "FROM") == 0 || 
                strcmp(token.lexeme, "INTO") == 0) {
                // Next token should be table name
                if (getNextToken(file, &token) && token.type == IDENTIFIER) {
                    strcpy(currentQuery.tableName, token.lexeme);
                }
            }
        } else if (token.type == IDENTIFIER) {
            // Store column names
            if (columnIndex < MAX_COLUMNS) {
                strcpy(currentQuery.columns[columnIndex++], token.lexeme);
            }
        } else if (token.type == SPECIAL_SYMBOL && token.lexeme[0] == ';') {
            break;  // End of query
        }
    }
    
    currentQuery.column_count = columnIndex;
    symbolTable[queryCount++] = currentQuery;
}

void analyzeSQLFile(const char *filename) {
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
            case FUNCTION: printf("FUNCTION"); break;
            default: printf("UNKNOWN"); break;
        }
        printf("\n");

        // Extract query information when a query-initiating keyword is found
        if (token.type == KEYWORD && 
            (strcmp(token.lexeme, "SELECT") == 0 || 
             strcmp(token.lexeme, "INSERT") == 0 ||
             strcmp(token.lexeme, "UPDATE") == 0 ||
             strcmp(token.lexeme, "DELETE") == 0 ||
             strcmp(token.lexeme, "CREATE") == 0 ||
             strcmp(token.lexeme, "DROP") == 0)) {
            extractQuery(file, token);
        }
    }

    fclose(file);
}

void displaySymbolTable() {
    printf("\nQuery Analysis Table:\n");
    printf("------------------------\n");
    for (int i = 0; i < queryCount; i++) {
        printf("Query Type: %s\n", symbolTable[i].queryType);
        printf("Table: %s\n", symbolTable[i].tableName);
        printf("Columns: ");
        for (int j = 0; j < symbolTable[i].column_count; j++) {
            printf("%s", symbolTable[i].columns[j]);
            if (j < symbolTable[i].column_count - 1) printf(", ");
        }
        printf("\n------------------------\n");
    }
}

int main() {
    char filename[100];
    printf("Enter the SQL file name: ");
    scanf("%s", filename);
    analyzeSQLFile(filename);
    displaySymbolTable();
    return 0;
}