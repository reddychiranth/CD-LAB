PHP

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_TOKEN_LEN 100
#define MAX_FUNCTIONS 100
#define MAX_PARAMS 10

typedef enum {
    KEYWORD, IDENTIFIER, OPERATOR, NUMERIC_CONSTANT, STRING_LITERAL, SPECIAL_SYMBOL, VARIABLE
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

const char *keywords[] = { "function", "return", "echo", "foreach", "as" };
#define KEYWORDS_COUNT (sizeof(keywords) / sizeof(keywords[0]))

int isKeyword(const char *lexeme) {
    for (int i = 0; i < KEYWORDS_COUNT; i++) {
        if (strcmp(lexeme, keywords[i]) == 0) return 1;
    }
    return 0;
}

void skipComments(FILE *file) {
    char ch = fgetc(file);
    if (ch == '/') {
        ch = fgetc(file);
        if (ch == '/') { // Single-line comment
            while ((ch = fgetc(file)) != EOF && ch != '\n');
        } else if (ch == '*') { // Multi-line comment
            while ((ch = fgetc(file)) != EOF) {
                if (ch == '*' && (ch = fgetc(file)) == '/') break;
            }
        } else {
            ungetc(ch, file);
        }
    } else if (ch == '#') { // Shell-style comment
        while ((ch = fgetc(file)) != EOF && ch != '\n');
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
        if (ch == '/' || ch == '#') {
            ungetc(ch, file);
            skipComments(file);
            continue;
        }
        break;
    }

    if (ch == EOF) return 0;

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

    if (isalpha(ch) || ch == '_') {
        buffer[bufIndex++] = ch;
        while ((ch = fgetc(file)) != EOF && (isalnum(ch) || ch == '_')) {
            buffer[bufIndex++] = ch;
        }
        ungetc(ch, file);
        buffer[bufIndex] = '\0';
        token->type = isKeyword(buffer) ? KEYWORD : IDENTIFIER;
        strcpy(token->lexeme, buffer);
        return 1;
    }

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

    if (ch == '"') {
        buffer[bufIndex++] = ch;
        while ((ch = fgetc(file)) != EOF && ch != '"') {
            buffer[bufIndex++] = ch;
        }
        buffer[bufIndex++] = '"';
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

    if (getNextToken(file, &token) && token.type == IDENTIFIER) {
        strcpy(functionName, token.lexeme);
        if (getNextToken(file, &token) && token.lexeme[0] == '(') {
            while (getNextToken(file, &token) && token.lexeme[0] != ')') {
                if (token.type == VARIABLE) {
                    strcpy(parameters[paramCount++], token.lexeme);
                }
            }
        }
        strcpy(symbolTable[functionCount].name, functionName);
        symbolTable[functionCount].param_count = paramCount;
        for (int i = 0; i < paramCount; i++) {
            strcpy(symbolTable[functionCount].parameters[i], parameters[i]);
        }
        functionCount++;
    }
}

void analyzePHPFile(const char *filename) {
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
    printf("Enter the PHP file name: ");
    scanf("%s", filename);
    analyzePHPFile(filename);
    displaySymbolTable();
    return 0;
}


JS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_TOKEN_LEN 100
#define MAX_FUNCTIONS 100
#define MAX_PARAMS 10

typedef enum {
    KEYWORD, IDENTIFIER, OPERATOR, NUMERIC_CONSTANT, STRING_LITERAL, SPECIAL_SYMBOL, VARIABLE
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

const char *keywords[] = { "function", "return", "let", "const", "var", "if", "else", "for", "while" };
#define KEYWORDS_COUNT (sizeof(keywords) / sizeof(keywords[0]))

int isKeyword(const char *lexeme) {
    for (int i = 0; i < KEYWORDS_COUNT; i++) {
        if (strcmp(lexeme, keywords[i]) == 0) return 1;
    }
    return 0;
}

void skipComments(FILE *file) {
    char ch = fgetc(file);
    if (ch == '/') {
        ch = fgetc(file);
        if (ch == '/') {
            while ((ch = fgetc(file)) != EOF && ch != '\n');
        } else if (ch == '*') {
            while ((ch = fgetc(file)) != EOF) {
                if (ch == '*' && (ch = fgetc(file)) == '/') break;
            }
        } else {
            ungetc(ch, file);
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
        if (ch == '/') {
            ungetc(ch, file);
            skipComments(file);
            continue;
        }
        break;
    }

    if (ch == EOF) return 0;

    if (isalpha(ch) || ch == '_') {
        buffer[bufIndex++] = ch;
        while ((ch = fgetc(file)) != EOF && (isalnum(ch) || ch == '_')) {
            buffer[bufIndex++] = ch;
        }
        ungetc(ch, file);
        buffer[bufIndex] = '\0';
        token->type = isKeyword(buffer) ? KEYWORD : IDENTIFIER;
        strcpy(token->lexeme, buffer);
        return 1;
    }

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

    if (ch == '"' || ch == '\'') {
        buffer[bufIndex++] = ch;
        while ((ch = fgetc(file)) != EOF && ch != '"' && ch != '\'') {
            buffer[bufIndex++] = ch;
        }
        buffer[bufIndex++] = ch;
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

    if (getNextToken(file, &token) && token.type == IDENTIFIER) {
        strcpy(functionName, token.lexeme);
        if (getNextToken(file, &token) && token.lexeme[0] == '(') {
            while (getNextToken(file, &token) && token.lexeme[0] != ')') {
                if (token.type == IDENTIFIER) {
                    strcpy(parameters[paramCount++], token.lexeme);
                }
            }
        }
        strcpy(symbolTable[functionCount].name, functionName);
        symbolTable[functionCount].param_count = paramCount;
        for (int i = 0; i < paramCount; i++) {
            strcpy(symbolTable[functionCount].parameters[i], parameters[i]);
        }
        functionCount++;
    }
}

void analyzeJSFile(const char *filename) {
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
    printf("Enter the JS file name: ");
    scanf("%s", filename);
    analyzeJSFile(filename);
    displaySymbolTable();
    return 0;
}


HTML

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_TOKEN_LEN 100
#define MAX_TAGS 100
#define MAX_ATTRIBUTES 10

typedef enum {
    TAG, ATTRIBUTE, TEXT_CONTENT, SPECIAL_SYMBOL, TEXT
} TokenType;

typedef struct {
    char lexeme[MAX_TOKEN_LEN];
    TokenType type;
} Token;

typedef struct {
    char name[MAX_TOKEN_LEN];
    char attributes[MAX_ATTRIBUTES][MAX_TOKEN_LEN];
    int attr_count;
} HTMLTag;

HTMLTag symbolTable[MAX_TAGS];
int tagCount = 0;

void skipWhitespace(FILE *file) {
    char ch;
    while ((ch = fgetc(file)) != EOF && isspace(ch));
    if (ch != EOF) ungetc(ch, file);
}

void skipComment(FILE *file) {
    char ch = fgetc(file);
    if (ch == '!') {
        ch = fgetc(file);
        if (ch == '-' && fgetc(file) == '-') {
            while ((ch = fgetc(file)) != EOF) {
                if (ch == '-' && fgetc(file) == '-' && fgetc(file) == '>') {
                    return; // End of comment
                }
            }
        }
    }
}

int getNextToken(FILE *file, Token *token) {
    char ch;
    char buffer[MAX_TOKEN_LEN];
    int bufIndex = 0;

    skipWhitespace(file);

    ch = fgetc(file);
    if (ch == EOF) return 0;

    if (ch == '<') {
        buffer[bufIndex++] = ch;
        ch = fgetc(file);

        if (ch == '!') {
            ungetc(ch, file);
            skipComment(file);
            return getNextToken(file, token);
        }
        
        while (ch != EOF && ch != '>' && !isspace(ch)) {
            buffer[bufIndex++] = ch;
            ch = fgetc(file);
        }
        ungetc(ch, file);
        buffer[bufIndex] = '\0';
        token->type = TAG;
        strcpy(token->lexeme, buffer);
        return 1;
    }

    if (isalpha(ch)) {
        buffer[bufIndex++] = ch;
        while ((ch = fgetc(file)) != EOF && (isalnum(ch) || ch == '-' || ch == '_')) {
            buffer[bufIndex++] = ch;
        }
        ungetc(ch, file);
        buffer[bufIndex] = '\0';

        skipWhitespace(file);
        ch = fgetc(file);
        if (ch == '=') {
            token->type = ATTRIBUTE;
            strcpy(token->lexeme, buffer);
            return 1;
        }

        ungetc(ch, file);
        token->type = TEXT;
        strcpy(token->lexeme, buffer);
        return 1;
    }

    if (ch == '"' || ch == '\'') {
        char quote = ch;
        buffer[bufIndex++] = ch;
        while ((ch = fgetc(file)) != EOF && ch != quote) {
            buffer[bufIndex++] = ch;
        }
        buffer[bufIndex++] = quote;
        buffer[bufIndex] = '\0';
        token->type = TEXT_CONTENT;
        strcpy(token->lexeme, buffer);
        return 1;
    }

    if (!isspace(ch) && ch != '<' && ch != '>') {
        buffer[bufIndex++] = ch;
        while ((ch = fgetc(file)) != EOF && !isspace(ch) && ch != '<' && ch != '>') {
            buffer[bufIndex++] = ch;
        }
        ungetc(ch, file);
        buffer[bufIndex] = '\0';
        token->type = TEXT_CONTENT;
        strcpy(token->lexeme, buffer);
        return 1;
    }

    buffer[bufIndex++] = ch;
    buffer[bufIndex] = '\0';
    token->type = SPECIAL_SYMBOL;
    strcpy(token->lexeme, buffer);
    return 1;
}

void extractTag(FILE *file, Token tagToken) {
    Token token;
    char tagName[MAX_TOKEN_LEN];
    char attributes[MAX_ATTRIBUTES][MAX_TOKEN_LEN];
    char attributeValues[MAX_ATTRIBUTES][MAX_TOKEN_LEN];
    int attrCount = 0;

    strcpy(tagName, tagToken.lexeme);

    while (getNextToken(file, &token)) {
        if (token.type == ATTRIBUTE) {
            strcpy(attributes[attrCount], token.lexeme);
            attrCount++;
            if (!getNextToken(file, &token) || token.type != SPECIAL_SYMBOL || token.lexeme[0] != '=') {
                continue;
            }
            if (getNextToken(file, &token) && token.type == TEXT_CONTENT) {
                strcpy(attributeValues[attrCount - 1], token.lexeme);
            } else {
                strcpy(attributeValues[attrCount - 1], "");
            }
        } else if (token.type == SPECIAL_SYMBOL && token.lexeme[0] == '>') {
            break;
        }
    }

    strcpy(symbolTable[tagCount].name, tagName);
    symbolTable[tagCount].attr_count = attrCount;
    for (int i = 0; i < attrCount; i++) {
        sprintf(symbolTable[tagCount].attributes[i], "%s=%s", attributes[i], attributeValues[i]);
    }
    tagCount++;
}

void analyzeHTMLFile(const char *filename) {
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
            case TAG: printf("TAG\n"); break;
            case ATTRIBUTE: printf("ATTRIBUTE\n"); break;
            case TEXT_CONTENT: printf("TEXT CONTENT\n"); break;
            case SPECIAL_SYMBOL: printf("SPECIAL SYMBOL\n"); break;
            case TEXT: printf("TEXT\n"); break;
            default: printf("UNKNOWN\n"); break;
        }
    }
    rewind(file);
    tagCount = 0;
    while (getNextToken(file, &token)) {
        if (token.type == TAG) {
            extractTag(file, token);
        }
    }
    fclose(file);
}

void displaySymbolTable() {
    printf("\nSymbol Table (HTML Tags and Attributes):\n");
    printf("--------------------------------------\n");
    for (int i = 0; i < tagCount; i++) {
        printf("Tag: %s> (", symbolTable[i].name);
        for (int j = 0; j < symbolTable[i].attr_count; j++) {
            printf("%s", symbolTable[i].attributes[j]);
            if (j < symbolTable[i].attr_count - 1) printf(", ");
        }
        printf(")\n");
    }
}

int main() {
    char filename[100];
    printf("Enter the HTML file name: ");
    scanf("%s", filename);
    analyzeHTMLFile(filename);
    displaySymbolTable();
    return 0;
}

JAVA

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_TOKEN_LEN 100
#define MAX_FUNCTIONS 100
#define MAX_PARAMS 10

typedef enum {
    KEYWORD, IDENTIFIER, OPERATOR, NUMERIC_CONSTANT, STRING_LITERAL, SPECIAL_SYMBOL, VARIABLE
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

const char *keywords[] = { "public", "private", "protected", "static", "void", "int", "double", "char", "float", "class", "return" };
#define KEYWORDS_COUNT (sizeof(keywords) / sizeof(keywords[0]))

int isKeyword(const char *lexeme) {
    for (int i = 0; i < KEYWORDS_COUNT; i++) {
        if (strcmp(lexeme, keywords[i]) == 0) return 1;
    }
    return 0;
}

void skipComments(FILE *file) {
    char ch = fgetc(file);
    if (ch == '/') {
        ch = fgetc(file);
        if (ch == '/') {
            while ((ch = fgetc(file)) != EOF && ch != '\n');
        } else if (ch == '*') {
            while ((ch = fgetc(file)) != EOF) {
                if (ch == '*' && (ch = fgetc(file)) == '/') break;
            }
        } else {
            ungetc(ch, file);
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
        if (ch == '/') {
            ungetc(ch, file);
            skipComments(file);
            continue;
        }
        break;
    }

    if (ch == EOF) return 0;

    if (isalpha(ch) || ch == '_') {
        buffer[bufIndex++] = ch;
        while ((ch = fgetc(file)) != EOF && (isalnum(ch) || ch == '_')) {
            buffer[bufIndex++] = ch;
        }
        ungetc(ch, file);
        buffer[bufIndex] = '\0';
        token->type = isKeyword(buffer) ? KEYWORD : IDENTIFIER;
        strcpy(token->lexeme, buffer);
        return 1;
    }

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

    if (ch == '"') {
        buffer[bufIndex++] = ch;
        while ((ch = fgetc(file)) != EOF && ch != '"') {
            buffer[bufIndex++] = ch;
        }
        buffer[bufIndex++] = '"';
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
    char returnType[MAX_TOKEN_LEN];
    char functionName[MAX_TOKEN_LEN];
    char parameters[MAX_PARAMS][MAX_TOKEN_LEN];
    int paramCount = 0;

    if (!getNextToken(file, &token) || token.type != KEYWORD) return; // Read return type
    strcpy(returnType, token.lexeme);

    if (!getNextToken(file, &token) || token.type != IDENTIFIER) return; // Read function name
    strcpy(functionName, token.lexeme);

    if (!getNextToken(file, &token) || token.lexeme[0] != '(') return; // Read '('

    while (getNextToken(file, &token) && token.lexeme[0] != ')') {
        if (token.type == KEYWORD) {  
            if (!getNextToken(file, &token) || token.type != IDENTIFIER) return; 
            strcpy(parameters[paramCount++], token.lexeme);
        }
    }

    // Store function in symbol table
    strcpy(symbolTable[functionCount].name, functionName);
    symbolTable[functionCount].param_count = paramCount;
    for (int i = 0; i < paramCount; i++) {
        strcpy(symbolTable[functionCount].parameters[i], parameters[i]);
    }
    functionCount++;
}

void analyzeJavaFile(const char *filename) {
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
            default: printf("UNKNOWN"); break;
        }
        printf("\n");

        if (token.type == KEYWORD && 
            (strcmp(token.lexeme, "public") == 0 || 
             strcmp(token.lexeme, "private") == 0 || 
             strcmp(token.lexeme, "protected") == 0 ||
             strcmp(token.lexeme, "static") == 0)) {
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
    printf("Enter the Java file name: ");
    scanf("%s", filename);
    analyzeJavaFile(filename);
    displaySymbolTable();
    return 0;
}


PYTHON

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_TOKEN_LEN 100
#define MAX_FUNCTIONS 100
#define MAX_PARAMS 10

typedef enum {
    KEYWORD, IDENTIFIER, OPERATOR, NUMERIC_CONSTANT, STRING_LITERAL, SPECIAL_SYMBOL, VARIABLE
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

const char *keywords[] = { "def", "return", "if", "else", "elif", "for", "while", "import", "from", "class", "try", "except", "finally", "with", "as" };
#define KEYWORDS_COUNT (sizeof(keywords) / sizeof(keywords[0]))

int isKeyword(const char *lexeme) {
    for (int i = 0; i < KEYWORDS_COUNT; i++) {
        if (strcmp(lexeme, keywords[i]) == 0) return 1;
    }
    return 0;
}

void skipComments(FILE *file) {
    char ch = fgetc(file);
    if (ch == '#') {
        while ((ch = fgetc(file)) != EOF && ch != '\n');
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
        if (ch == '#') {
            skipComments(file);
            continue;
        }
        break;
    }

    if (ch == EOF) return 0;

    if (isalpha(ch) || ch == '_') {
        buffer[bufIndex++] = ch;
        while ((ch = fgetc(file)) != EOF && (isalnum(ch) || ch == '_')) {
            buffer[bufIndex++] = ch;
        }
        ungetc(ch, file);
        buffer[bufIndex] = '\0';
        token->type = isKeyword(buffer) ? KEYWORD : IDENTIFIER;
        strcpy(token->lexeme, buffer);
        return 1;
    }

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

    if (ch == '"' || ch == '\'') {
        buffer[bufIndex++] = ch;
        while ((ch = fgetc(file)) != EOF && ch != buffer[0]) {
            buffer[bufIndex++] = ch;
        }
        buffer[bufIndex++] = ch;
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

  // Get function name (we've already found "def")
  if (getNextToken(file, &token) && token.type == IDENTIFIER) {
      strcpy(functionName, token.lexeme);
      
      // Look for opening parenthesis
      if (getNextToken(file, &token) && token.lexeme[0] == '(') {
          // Read parameters until closing parenthesis
          while (getNextToken(file, &token) && token.lexeme[0] != ')') {
              if (token.type == IDENTIFIER) {
                  strcpy(parameters[paramCount++], token.lexeme);
              }
              // Skip commas between parameters
              else if (token.lexeme[0] == ',') {
                  continue;
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
}

void analyzePythonFile(const char *filename) {
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
          default: printf("UNKNOWN"); break;
      }
      printf("\n");

      // Check for 'def' keyword to extract function details
      if (token.type == KEYWORD && strcmp(token.lexeme, "def") == 0) {
          // Save current position in the file
          long currentPos = ftell(file);
          
          // Extract function
          extractFunction(file);
          
          // Return to current position for continued lexical analysis
          fseek(file, currentPos, SEEK_SET);
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
    printf("Enter the Python file name: ");
    scanf("%s", filename);
    analyzePythonFile(filename);
    displaySymbolTable();
    return 0;
}


C++

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_TOKEN_LEN 100
#define MAX_FUNCTIONS 100
#define MAX_PARAMS 10

typedef enum {
    KEYWORD, IDENTIFIER, OPERATOR, NUMERIC_CONSTANT, STRING_LITERAL, SPECIAL_SYMBOL, VARIABLE
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

// List of C++ Keywords
const char *keywords[] = { 
    "int", "float", "double", "char", "if", "else", "while", "for", "return", 
    "switch", "case", "break", "continue", "void", "class", "struct", "public",
    "private", "protected", "virtual", "new", "delete", "try", "catch", "throw"
};
#define KEYWORDS_COUNT (sizeof(keywords) / sizeof(keywords[0]))

int isKeyword(const char *lexeme) {
    for (int i = 0; i < KEYWORDS_COUNT; i++) {
        if (strcmp(lexeme, keywords[i]) == 0) return 1;
    }
    return 0;
}

void skipComments(FILE *file) {
    char ch = fgetc(file);
    if (ch == '/') {
        char next = fgetc(file);
        if (next == '/') { // Single-line comment
            while ((ch = fgetc(file)) != EOF && ch != '\n');
        } else if (next == '*') { // Multi-line comment
            while ((ch = fgetc(file)) != EOF) {
                if (ch == '*' && (ch = fgetc(file)) == '/') break;
            }
        } else {
            ungetc(next, file);
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
        if (ch == '/') {
            skipComments(file);
            continue;
        }
        break;
    }

    if (ch == EOF) return 0;

    if (isalpha(ch) || ch == '_') {
        buffer[bufIndex++] = ch;
        while ((ch = fgetc(file)) != EOF && (isalnum(ch) || ch == '_')) {
            buffer[bufIndex++] = ch;
        }
        ungetc(ch, file);
        buffer[bufIndex] = '\0';
        token->type = isKeyword(buffer) ? KEYWORD : IDENTIFIER;
        strcpy(token->lexeme, buffer);
        return 1;
    }

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

    if (ch == '"' || ch == '\'') {
        buffer[bufIndex++] = ch;
        while ((ch = fgetc(file)) != EOF && ch != buffer[0]) {
            buffer[bufIndex++] = ch;
        }
        buffer[bufIndex++] = ch;
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

    // Get function name (after return type)
    if (getNextToken(file, &token) && token.type == IDENTIFIER) {
        strcpy(functionName, token.lexeme);
        
        // Look for opening parenthesis
        if (getNextToken(file, &token) && token.lexeme[0] == '(') {
            // Read parameters until closing parenthesis
            while (getNextToken(file, &token) && token.lexeme[0] != ')') {
                if (token.type == IDENTIFIER) {
                    strcpy(parameters[paramCount++], token.lexeme);
                }
                // Skip commas between parameters
                else if (token.lexeme[0] == ',') {
                    continue;
                }
            }
            
            // Store function in symbol table
            strcpy(symbolTable[functionCount].name, functionName);
            symbolTable[functionCount].param_count = paramCount;
            for (int i = 0; i < paramCount; i++) {
                strcpy(symbolTable[functionCount].parameters[i], parameters[i]);
            }
            functionCount++;
        }
    }
}

void analyzeCppFile(const char *filename) {
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
            default: printf("UNKNOWN"); break;
        }
        printf("\n");

        // Check for function definitions
        if (token.type == KEYWORD) {
            long currentPos = ftell(file);
            extractFunction(file);
            fseek(file, currentPos, SEEK_SET);
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
    printf("Enter the C++ file name: ");
    scanf("%s", filename);
    analyzeCppFile(filename);
    displaySymbolTable();
    return 0;
}


PEARL

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_TOKEN_LEN 100
#define MAX_FUNCTIONS 100
#define MAX_PARAMS 10

typedef enum {
    KEYWORD, IDENTIFIER, OPERATOR, NUMERIC_CONSTANT, STRING_LITERAL, SPECIAL_SYMBOL, VARIABLE
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

const char *keywords[] = {
    "sub", "my", "our", "if", "else", "elsif", "while", "for", "foreach", "return",
    "last", "next", "redo", "goto", "do", "unless", "package", "use", "require"
};
#define KEYWORDS_COUNT (sizeof(keywords) / sizeof(keywords[0]))

int isKeyword(const char *lexeme) {
    for (int i = 0; i < KEYWORDS_COUNT; i++) {
        if (strcmp(lexeme, keywords[i]) == 0) return 1;
    }
    return 0;
}

void skipComments(FILE *file) {
  int ch = fgetc(file);

  // If the first line contains a shebang (#!), skip the entire line
  if (ch == '#' && ftell(file) == 1) {
      while ((ch = fgetc(file)) != EOF && ch != '\n');
      return;
  }

  // Regular comment handling (skip until newline)
  if (ch == '#') {
      while ((ch = fgetc(file)) != EOF && ch != '\n');
  }

  // Put back the last character if it's not EOF
  if (ch != EOF) {
      ungetc(ch, file);
  }
}


int getNextToken(FILE *file, Token *token) {
    char ch;
    char buffer[MAX_TOKEN_LEN];
    int bufIndex = 0;

    while ((ch = fgetc(file)) != EOF) {
      if (isspace(ch)) continue;

      // Handle comments properly
      if (ch == '#') {
          skipComments(file);
          continue; // Restart token reading after skipping the comment
      }
      break;
  }

  if (ch == EOF) return 0; // End of file

    if (isalpha(ch) || ch == '_') {
        buffer[bufIndex++] = ch;
        while ((ch = fgetc(file)) != EOF && (isalnum(ch) || ch == '_')) {
            buffer[bufIndex++] = ch;
        }
        ungetc(ch, file);
        buffer[bufIndex] = '\0';
        token->type = isKeyword(buffer) ? KEYWORD : IDENTIFIER;
        strcpy(token->lexeme, buffer);
        return 1;
    }

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

    if (ch == '"' || ch == '\'') {
        char quote = ch;
        buffer[bufIndex++] = ch;
        while ((ch = fgetc(file)) != EOF && ch != quote) {
            buffer[bufIndex++] = ch;
        }
        buffer[bufIndex++] = ch;
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

    if (getNextToken(file, &token) && token.type == IDENTIFIER) {
        strcpy(functionName, token.lexeme);
        if (getNextToken(file, &token) && token.lexeme[0] == '(') {
            while (getNextToken(file, &token) && token.lexeme[0] != ')') {
                if (token.type == IDENTIFIER) {
                    strcpy(parameters[paramCount++], token.lexeme);
                }
                else if (token.lexeme[0] == ',') {
                    continue;
                }
            }
        }
        strcpy(symbolTable[functionCount].name, functionName);
        symbolTable[functionCount].param_count = paramCount;
        for (int i = 0; i < paramCount; i++) {
            strcpy(symbolTable[functionCount].parameters[i], parameters[i]);
        }
        functionCount++;
    }
}

void analyzePerlFile(const char *filename) {
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
            default: printf("UNKNOWN"); break;
        }
        printf("\n");

        if (token.type == KEYWORD && strcmp(token.lexeme, "sub") == 0) {
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
    printf("Enter the Perl file name: ");
    scanf("%s", filename);
    analyzePerlFile(filename);
    displaySymbolTable();
    return 0;
}


CSS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_TOKEN_LEN 100

typedef enum {
    SELECTOR, PROPERTY, VALUE, SPECIAL_SYMBOL, COMMENT
} TokenType;

typedef struct {
    char lexeme[MAX_TOKEN_LEN];
    TokenType type;
} Token;

void skipWhitespace(FILE *file) {
    char ch;
    while ((ch = fgetc(file)) != EOF && isspace(ch));
    if (ch != EOF) ungetc(ch, file);
}

void skipComment(FILE *file) {
    char ch = fgetc(file);
    if (ch == '*') {
        while ((ch = fgetc(file)) != EOF) {
            if (ch == '*' && fgetc(file) == '/') {
                return; // End of comment
            }
        }
    }
}

int getNextToken(FILE *file, Token *token) {
    char ch;
    char buffer[MAX_TOKEN_LEN];
    int bufIndex = 0;

    skipWhitespace(file);
    ch = fgetc(file);
    if (ch == EOF) return 0;

    // Handle comments
    if (ch == '/') {
        if (fgetc(file) == '*') {
            skipComment(file);
            strcpy(token->lexeme, "/* Comment */");
            token->type = COMMENT;
            return 1;
        } else {
            ungetc('/', file);
        }
    }

    // Handle special symbols
    if (ch == '{' || ch == '}' || ch == ':' || ch == ';') {
        buffer[0] = ch;
        buffer[1] = '\0';
        token->type = SPECIAL_SYMBOL;
        strcpy(token->lexeme, buffer);
        return 1;
    }

    // Handle selectors (ID, class, element)
    if (isalpha(ch) || ch == '#' || ch == '.') {
        buffer[bufIndex++] = ch;
        while ((ch = fgetc(file)) != EOF && (isalnum(ch) || ch == '-' || ch == '_')) {
            buffer[bufIndex++] = ch;
        }
        ungetc(ch, file);
        buffer[bufIndex] = '\0';
        token->type = SELECTOR;
        strcpy(token->lexeme, buffer);
        return 1;
    }

    // Handle properties inside `{ }`
    if (isalpha(ch)) {
        buffer[bufIndex++] = ch;
        while ((ch = fgetc(file)) != EOF && (isalnum(ch) || ch == '-')) {
            buffer[bufIndex++] = ch;
        }
        ungetc(ch, file);
        buffer[bufIndex] = '\0';
        token->type = PROPERTY;
        strcpy(token->lexeme, buffer);
        return 1;
    }

    // Handle values (colors, numbers, URLs)
    if (isdigit(ch) || ch == '"' || ch == '\'' || ch == '(') {
        buffer[bufIndex++] = ch;
        while ((ch = fgetc(file)) != EOF && !isspace(ch) && ch != ';' && ch != '}') {
            buffer[bufIndex++] = ch;
        }
        ungetc(ch, file);
        buffer[bufIndex] = '\0';
        token->type = VALUE;
        strcpy(token->lexeme, buffer);
        return 1;
    }

    return 0;
}

void analyzeCSSFile(const char *filename) {
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
            case SELECTOR: printf("SELECTOR\n"); break;
            case PROPERTY: printf("PROPERTY\n"); break;
            case VALUE: printf("VALUE\n"); break;
            case SPECIAL_SYMBOL: printf("SPECIAL SYMBOL\n"); break;
            case COMMENT: printf("COMMENT\n"); break;
            default: printf("UNKNOWN\n"); break;
        }
    }

    fclose(file);
}

int main() {
    char filename[100];
    printf("Enter the CSS file name: ");
    scanf("%s", filename);
    analyzeCSSFile(filename);
    return 0;
} 