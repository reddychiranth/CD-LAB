#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_TOKEN_LEN 100
#define MAX_FUNCTIONS 100

typedef enum {
    KEYWORD,
    IDENTIFIER,
    OPERATOR,
    NUMERIC_CONSTANT,
    STRING_LITERAL,
    SPECIAL_SYMBOL,
    JQUERY_FUNCTION
} TokenType;

typedef struct {
    char lexeme[MAX_TOKEN_LEN];
    TokenType type;
} Token;

// JavaScript and jQuery Keywords
const char *keywords[] = {
    "if", "else", "var", "let", "const", "function", "return",
    "true", "false", "null", "undefined", "new", "this", "typeof",
    "break", "continue", "for", "while", "do", "switch", "case"
};
#define KEYWORDS_COUNT (sizeof(keywords) / sizeof(keywords[0]))

// jQuery Functions
const char *jqueryFunctions[] = {
    "$", "jQuery", "ready", "click", "hover", "on", "off",
    "hide", "show", "toggle", "ajax", "get", "post", "find",
    "children", "parent", "append", "remove", "addClass", "removeClass"
};
#define JQUERY_FUNCTIONS_COUNT (sizeof(jqueryFunctions) / sizeof(jqueryFunctions[0]))

// Operators
const char *operators[] = {
    "==", "===", "!=", "!==", ">=", "<=", "&&", "||",
    "++", "--", "+=", "-=", "*=", "/=", "%=", "=",
    "+", "-", "*", "/", "%", ">", "<", "!"
};
#define OPERATORS_COUNT (sizeof(operators) / sizeof(operators[0]))

int isKeyword(const char *lexeme) {
    for (int i = 0; i < KEYWORDS_COUNT; i++) {
        if (strcmp(lexeme, keywords[i]) == 0) return 1;
    }
    return 0;
}

int isJQueryFunction(const char *lexeme) {
    for (int i = 0; i < JQUERY_FUNCTIONS_COUNT; i++) {
        if (strcmp(lexeme, jqueryFunctions[i]) == 0) return 1;
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
    if (ch == '/') {
        ch = fgetc(file);
        if (ch == '/') {  // Single line comment
            while ((ch = fgetc(file)) != EOF && ch != '\n');
        } else if (ch == '*') {  // Multi-line comment
            int done = 0;
            while (!done && (ch = fgetc(file)) != EOF) {
                if (ch == '*') {
                    if ((ch = fgetc(file)) == '/') done = 1;
                    else ungetc(ch, file);
                }
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

    // Skip whitespace and comments
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

    // Handle jQuery selector ($) and jQuery object
    if (ch == '$' || ch == '.') {
        buffer[bufIndex++] = ch;
        ch = fgetc(file);
        
        if (ch == '(') {
            buffer[bufIndex++] = ch;
            // Read until matching closing parenthesis
            int parentheses = 1;
            while ((ch = fgetc(file)) != EOF && parentheses > 0) {
                buffer[bufIndex++] = ch;
                if (ch == '(') parentheses++;
                if (ch == ')') parentheses--;
            }
        } else {
            ungetc(ch, file);
        }
        
        buffer[bufIndex] = '\0';
        token->type = JQUERY_FUNCTION;
        strcpy(token->lexeme, buffer);
        return 1;
    }

    // Handle strings
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

    // Handle numbers (including hex and decimals)
    if (isdigit(ch) || (ch == '.' && isdigit(fgetc(file)))) {
        ungetc(ch, file);
        while ((ch = fgetc(file)) != EOF && 
               (isdigit(ch) || ch == '.' || ch == 'x' || 
                (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F'))) {
            buffer[bufIndex++] = ch;
        }
        ungetc(ch, file);
        buffer[bufIndex] = '\0';
        token->type = NUMERIC_CONSTANT;
        strcpy(token->lexeme, buffer);
        return 1;
    }

    // Handle identifiers and keywords
    if (isalpha(ch) || ch == '_' || ch == '$') {
        buffer[bufIndex++] = ch;
        while ((ch = fgetc(file)) != EOF && 
               (isalnum(ch) || ch == '_' || ch == '$')) {
            buffer[bufIndex++] = ch;
        }
        ungetc(ch, file);
        buffer[bufIndex] = '\0';
        
        if (isKeyword(buffer)) {
            token->type = KEYWORD;
        } else if (isJQueryFunction(buffer)) {
            token->type = JQUERY_FUNCTION;
        } else {
            token->type = IDENTIFIER;
        }
        strcpy(token->lexeme, buffer);
        return 1;
    }

    // Handle operators and special symbols
    if (strchr("+-*/%=<>!&|.,;(){}[]?:", ch)) {
        buffer[bufIndex++] = ch;
        ch = fgetc(file);
        
        // Check for two-character operators
        if (strchr("=&|<>!", ch)) {
            buffer[bufIndex++] = ch;
            if (ch == '=' && buffer[0] == '=' || buffer[0] == '!') {
                ch = fgetc(file);
                if (ch == '=') buffer[bufIndex++] = ch;
                else ungetc(ch, file);
            }
        } else {
            ungetc(ch, file);
        }
        
        buffer[bufIndex] = '\0';
        token->type = isOperator(buffer) ? OPERATOR : SPECIAL_SYMBOL;
        strcpy(token->lexeme, buffer);
        return 1;
    }

    // Handle any other single-character tokens
    buffer[bufIndex++] = ch;
    buffer[bufIndex] = '\0';
    token->type = SPECIAL_SYMBOL;
    strcpy(token->lexeme, buffer);
    return 1;
}

void analyzeJQueryFile(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        return;
    }

    Token token;
    printf("\nLexical Analysis Output:\n");
    printf("------------------------\n");
    printf("Lexeme\t\t\tType\n");
    printf("------------------------\n");

    while (getNextToken(file, &token)) {
        printf("%-20s\t", token.lexeme);
        switch (token.type) {
            case KEYWORD: printf("KEYWORD"); break;
            case IDENTIFIER: printf("IDENTIFIER"); break;
            case OPERATOR: printf("OPERATOR"); break;
            case NUMERIC_CONSTANT: printf("NUMERIC CONSTANT"); break;
            case STRING_LITERAL: printf("STRING LITERAL"); break;
            case SPECIAL_SYMBOL: printf("SPECIAL SYMBOL"); break;
            case JQUERY_FUNCTION: printf("JQUERY FUNCTION"); break;
            default: printf("UNKNOWN"); break;
        }
        printf("\n");
    }

    fclose(file);
}

int main() {
    char filename[100];
    printf("Enter the jQuery file name (.js): ");
    scanf("%s", filename);
    analyzeJQueryFile(filename);
    return 0;
}
