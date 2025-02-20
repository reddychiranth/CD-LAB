#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_TOKEN_LEN 100
#define MAX_MODULES 100
#define MAX_PORTS 50

typedef enum {
    KEYWORD, IDENTIFIER, OPERATOR, NUMERIC_CONSTANT,
    STRING_LITERAL, SPECIAL_SYMBOL, PORT_TYPE,
    MODULE_NAME, NET_TYPE, STRENGTH, TIME_UNIT,
    PARAMETER, GATE_TYPE
} TokenType;

typedef struct {
    char lexeme[MAX_TOKEN_LEN];
    TokenType type;
} Token;

typedef struct {
    char name[MAX_TOKEN_LEN];
    char ports[MAX_PORTS][MAX_TOKEN_LEN];
    char port_types[MAX_PORTS][MAX_TOKEN_LEN];  // input, output, inout
    char port_nets[MAX_PORTS][MAX_TOKEN_LEN];   // wire, reg
    int port_count;
} Module;

Module symbolTable[MAX_MODULES];
int moduleCount = 0;

// Verilog keywords
const char *keywords[] = {
    "module", "endmodule", "input", "output", "inout", "wire", "reg",
    "always", "assign", "begin", "end", "case", "endcase", "default",
    "else", "for", "if", "initial", "parameter", "localparam", "posedge",
    "negedge", "wait", "while", "forever", "repeat", "generate",
    "endgenerate", "task", "endtask", "function", "endfunction"
};
#define KEYWORDS_COUNT (sizeof(keywords) / sizeof(keywords[0]))

// Port types
const char *port_types[] = {"input", "output", "inout"};
#define PORT_TYPES_COUNT (sizeof(port_types) / sizeof(port_types[0]))

// Net types
const char *net_types[] = {
    "wire", "reg", "tri", "tri0", "tri1", "supply0",
    "supply1", "wand", "wor", "trireg"
};
#define NET_TYPES_COUNT (sizeof(net_types) / sizeof(net_types[0]))

// Gate types
const char *gate_types[] = {
    "and", "nand", "or", "nor", "xor", "xnor", "not",
    "buf", "bufif0", "bufif1", "notif0", "notif1"
};
#define GATE_TYPES_COUNT (sizeof(gate_types) / sizeof(gate_types[0]))

int isKeyword(const char *lexeme) {
    for (int i = 0; i < KEYWORDS_COUNT; i++) {
        if (strcmp(lexeme, keywords[i]) == 0) return 1;
    }
    return 0;
}

int isPortType(const char *lexeme) {
    for (int i = 0; i < PORT_TYPES_COUNT; i++) {
        if (strcmp(lexeme, port_types[i]) == 0) return 1;
    }
    return 0;
}

int isNetType(const char *lexeme) {
    for (int i = 0; i < NET_TYPES_COUNT; i++) {
        if (strcmp(lexeme, net_types[i]) == 0) return 1;
    }
    return 0;
}

int isGateType(const char *lexeme) {
    for (int i = 0; i < GATE_TYPES_COUNT; i++) {
        if (strcmp(lexeme, gate_types[i]) == 0) return 1;
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

    // Handle identifiers, keywords, etc.
    if (isalpha(ch) || ch == '_') {
        buffer[bufIndex++] = ch;
        while ((ch = fgetc(file)) != EOF && 
               (isalnum(ch) || ch == '_' || ch == '$')) {
            buffer[bufIndex++] = ch;
        }
        ungetc(ch, file);
        buffer[bufIndex] = '\0';
        
        if (isKeyword(buffer)) {
            token->type = KEYWORD;
        } else if (isPortType(buffer)) {
            token->type = PORT_TYPE;
        } else if (isNetType(buffer)) {
            token->type = NET_TYPE;
        } else if (isGateType(buffer)) {
            token->type = GATE_TYPE;
        } else {
            token->type = IDENTIFIER;
        }
        strcpy(token->lexeme, buffer);
        return 1;
    }

    // Handle numbers including base specifiers
    if (isdigit(ch) || ch == '\'') {
        buffer[bufIndex++] = ch;
        if (ch == '\'') {
            ch = fgetc(file);
            if (ch == 'b' || ch == 'B' || ch == 'h' || ch == 'H' || 
                ch == 'd' || ch == 'D' || ch == 'o' || ch == 'O') {
                buffer[bufIndex++] = ch;
                while ((ch = fgetc(file)) != EOF && 
                       (isalnum(ch) || ch == '_' || ch == 'x' || ch == 'X' || ch == 'z' || ch == 'Z')) {
                    buffer[bufIndex++] = ch;
                }
            }
        } else {
            while ((ch = fgetc(file)) != EOF && 
                   (isdigit(ch) || ch == '.' || ch == '_' || 
                    ch == 'e' || ch == 'E')) {
                buffer[bufIndex++] = ch;
            }
        }
        ungetc(ch, file);
        buffer[bufIndex] = '\0';
        token->type = NUMERIC_CONSTANT;
        strcpy(token->lexeme, buffer);
        return 1;
    }

    // Handle string literals
    if (ch == '"') {
        buffer[bufIndex++] = ch;
        while ((ch = fgetc(file)) != EOF && ch != '"') {
            if (ch == '\\') {
                buffer[bufIndex++] = ch;
                ch = fgetc(file);
            }
            buffer[bufIndex++] = ch;
        }
        buffer[bufIndex++] = '"';
        buffer[bufIndex] = '\0';
        token->type = STRING_LITERAL;
        strcpy(token->lexeme, buffer);
        return 1;
    }

    // Handle operators and special symbols
    if (strchr("+-*/<>=!&|^%()[]{},;:#.", ch)) {
        buffer[bufIndex++] = ch;
        ch = fgetc(file);
        if (strchr("=<>&|", ch)) {
            buffer[bufIndex++] = ch;
        } else {
            ungetc(ch, file);
        }
        buffer[bufIndex] = '\0';
        token->type = SPECIAL_SYMBOL;
        strcpy(token->lexeme, buffer);
        return 1;
    }

    buffer[bufIndex++] = ch;
    buffer[bufIndex] = '\0';
    token->type = SPECIAL_SYMBOL;
    strcpy(token->lexeme, buffer);
    return 1;
}

void extractModule(FILE *file) {
    Token token;
    char moduleName[MAX_TOKEN_LEN];
    char ports[MAX_PORTS][MAX_TOKEN_LEN];
    char portTypes[MAX_PORTS][MAX_TOKEN_LEN];
    char portNets[MAX_PORTS][MAX_TOKEN_LEN];
    int portCount = 0;

    // Get module name
    if (getNextToken(file, &token) && token.type == IDENTIFIER) {
        strcpy(moduleName, token.lexeme);
        
        // Parse port list
        if (getNextToken(file, &token) && token.lexeme[0] == '(') {
            while (getNextToken(file, &token) && token.lexeme[0] != ')') {
                if (token.type == IDENTIFIER) {
                    strcpy(ports[portCount], token.lexeme);
                    strcpy(portTypes[portCount], "");  // Will be filled later
                    strcpy(portNets[portCount], "");   // Will be filled later
                    portCount++;
                }
            }
        }

        // Parse port declarations
        while (getNextToken(file, &token)) {
            if (token.type == KEYWORD && strcmp(token.lexeme, "endmodule") == 0) {
                break;
            }
            
            if (token.type == PORT_TYPE) {
                char currentType[MAX_TOKEN_LEN];
                strcpy(currentType, token.lexeme);
                
                // Get net type if specified
                char currentNet[MAX_TOKEN_LEN] = "";
                if (getNextToken(file, &token) && token.type == NET_TYPE) {
                    strcpy(currentNet, token.lexeme);
                    getNextToken(file, &token);
                }
                
                // Update port information
                if (token.type == IDENTIFIER) {
                    for (int i = 0; i < portCount; i++) {
                        if (strcmp(ports[i], token.lexeme) == 0) {
                            strcpy(portTypes[i], currentType);
                            if (strlen(currentNet) > 0) {
                                strcpy(portNets[i], currentNet);
                            }
                            break;
                        }
                    }
                }
            }
        }

        // Store in symbol table
        strcpy(symbolTable[moduleCount].name, moduleName);
        symbolTable[moduleCount].port_count = portCount;
        for (int i = 0; i < portCount; i++) {
            strcpy(symbolTable[moduleCount].ports[i], ports[i]);
            strcpy(symbolTable[moduleCount].port_types[i], portTypes[i]);
            strcpy(symbolTable[moduleCount].port_nets[i], portNets[i]);
        }
        moduleCount++;
    }
}

void analyzeVerilogFile(const char *filename) {
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
            case PORT_TYPE: printf("PORT TYPE"); break;
            case MODULE_NAME: printf("MODULE NAME"); break;
            case NET_TYPE: printf("NET TYPE"); break;
            case STRENGTH: printf("STRENGTH"); break;
            case TIME_UNIT: printf("TIME UNIT"); break;
            case PARAMETER: printf("PARAMETER"); break;
            case GATE_TYPE: printf("GATE TYPE"); break;
            default: printf("UNKNOWN"); break;
        }
        printf("\n");

        if (token.type == KEYWORD && strcmp(token.lexeme, "module") == 0) {
            extractModule(file);
        }
    }

    fclose(file);
}

void displaySymbolTable() {
    printf("\nVerilog Module Analysis:\n");
    printf("------------------------\n");
    for (int i = 0; i < moduleCount; i++) {
        printf("Module: %s\n", symbolTable[i].name);
        printf("Ports:\n");
        for (int j = 0; j < symbolTable[i].port_count; j++) {
            printf("  %s: %s", symbolTable[i].ports[j], 
                   symbolTable[i].port_types[j]);
            if (strlen(symbolTable[i].port_nets[j]) > 0) {
                printf(" (%s)", symbolTable[i].port_nets[j]);
            }
            printf("\n");
        }
        printf("------------------------\n");
    }
}

int main() {
    char filename[100];
    printf("Enter the Verilog file name: ");
    scanf("%s", filename);
    analyzeVerilogFile(filename);
    displaySymbolTable();
    return 0;
}