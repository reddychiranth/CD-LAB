#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_TOKEN_LEN 100
#define MAX_LABELS 100
#define MAX_OPERANDS 4

typedef enum {
    INSTRUCTION, REGISTER, LABEL, DIRECTIVE,
    NUMERIC_CONSTANT, STRING_LITERAL, SPECIAL_SYMBOL,
    MEMORY_REFERENCE, OPERAND
} TokenType;

typedef struct {
    char lexeme[MAX_TOKEN_LEN];
    TokenType type;
} Token;

typedef struct {
    char name[MAX_TOKEN_LEN];
    char operands[MAX_OPERANDS][MAX_TOKEN_LEN];
    int operand_count;
    int address;
} Label;

Label symbolTable[MAX_LABELS];
int labelCount = 0;
int currentAddress = 0;

// Assembly specific instructions (x86)
const char *instructions[] = {
    "mov", "add", "sub", "mul", "div", "inc", "dec",
    "and", "or", "xor", "not", "shl", "shr",
    "push", "pop", "call", "ret", "jmp", "je", "jne",
    "jg", "jl", "jge", "jle", "cmp", "test"
};
#define INSTRUCTIONS_COUNT (sizeof(instructions) / sizeof(instructions[0]))

// Assembly registers (x86)
const char *registers[] = {
    "eax", "ebx", "ecx", "edx", "esi", "edi", "esp", "ebp",
    "ax", "bx", "cx", "dx", "al", "bl", "cl", "dl",
    "ah", "bh", "ch", "dh", "rax", "rbx", "rcx", "rdx"
};
#define REGISTERS_COUNT (sizeof(registers) / sizeof(registers[0]))

// Assembly directives
const char *directives[] = {
    "section", "global", "extern", "db", "dw", "dd",
    "equ", "times", "org", "align", "bits", "end"
};
#define DIRECTIVES_COUNT (sizeof(directives) / sizeof(directives[0]))

int isInstruction(const char *lexeme) {
    char lowerLexeme[MAX_TOKEN_LEN];
    strcpy(lowerLexeme, lexeme);
    for(int i = 0; lowerLexeme[i]; i++) {
        lowerLexeme[i] = tolower(lowerLexeme[i]);
    }
    
    for (int i = 0; i < INSTRUCTIONS_COUNT; i++) {
        if (strcmp(lowerLexeme, instructions[i]) == 0) return 1;
    }
    return 0;
}

int isRegister(const char *lexeme) {
    char lowerLexeme[MAX_TOKEN_LEN];
    strcpy(lowerLexeme, lexeme);
    for(int i = 0; lowerLexeme[i]; i++) {
        lowerLexeme[i] = tolower(lowerLexeme[i]);
    }
    
    for (int i = 0; i < REGISTERS_COUNT; i++) {
        if (strcmp(lowerLexeme, registers[i]) == 0) return 1;
    }
    return 0;
}

int isDirective(const char *lexeme) {
    char lowerLexeme[MAX_TOKEN_LEN];
    strcpy(lowerLexeme, lexeme);
    for(int i = 0; lowerLexeme[i]; i++) {
        lowerLexeme[i] = tolower(lowerLexeme[i]);
    }
    
    for (int i = 0; i < DIRECTIVES_COUNT; i++) {
        if (strcmp(lowerLexeme, directives[i]) == 0) return 1;
    }
    return 0;
}

void skipComments(FILE *file) {
    char ch = fgetc(file);
    if (ch == ';') {  // Assembly comment
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
        if (ch == ';') {
            ungetc(ch, file);
            skipComments(file);
            continue;
        }
        break;
    }

    if (ch == EOF) return 0;

    // Handle labels (ending with :)
    if (isalpha(ch) || ch == '_' || ch == '.') {
        buffer[bufIndex++] = ch;
        while ((ch = fgetc(file)) != EOF && (isalnum(ch) || ch == '_' || ch == '.')) {
            buffer[bufIndex++] = ch;
        }
        
        if (ch == ':') {
            buffer[bufIndex++] = ch;
            buffer[bufIndex] = '\0';
            token->type = LABEL;
            strcpy(token->lexeme, buffer);
            return 1;
        }
        
        ungetc(ch, file);
        buffer[bufIndex] = '\0';
        
        if (isInstruction(buffer)) {
            token->type = INSTRUCTION;
        } else if (isRegister(buffer)) {
            token->type = REGISTER;
        } else if (isDirective(buffer)) {
            token->type = DIRECTIVE;
        } else {
            token->type = OPERAND;
        }
        strcpy(token->lexeme, buffer);
        return 1;
    }

    // Handle memory references [...]
    if (ch == '[') {
        buffer[bufIndex++] = ch;
        while ((ch = fgetc(file)) != EOF && ch != ']') {
            buffer[bufIndex++] = ch;
        }
        if (ch == ']') buffer[bufIndex++] = ch;
        buffer[bufIndex] = '\0';
        token->type = MEMORY_REFERENCE;
        strcpy(token->lexeme, buffer);
        return 1;
    }

    // Handle numbers (including hex)
    if (isdigit(ch) || ch == '$' || ch == '0') {
        buffer[bufIndex++] = ch;
        if (ch == '0') {
            ch = fgetc(file);
            if (ch == 'x' || ch == 'X') {
                buffer[bufIndex++] = ch;
                while ((ch = fgetc(file)) != EOF && 
                       (isdigit(ch) || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F'))) {
                    buffer[bufIndex++] = ch;
                }
            }
        } else {
            while ((ch = fgetc(file)) != EOF && (isdigit(ch) || ch == 'h')) {
                buffer[bufIndex++] = ch;
            }
        }
        ungetc(ch, file);
        buffer[bufIndex] = '\0';
        token->type = NUMERIC_CONSTANT;
        strcpy(token->lexeme, buffer);
        return 1;
    }

    // Handle strings
    if (ch == '\'' || ch == '"') {
        char quote = ch;
        buffer[bufIndex++] = ch;
        while ((ch = fgetc(file)) != EOF && ch != quote) {
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

void extractLabel(FILE *file, Token labelToken) {
    Token token;
    char labelName[MAX_TOKEN_LEN];
    char operands[MAX_OPERANDS][MAX_TOKEN_LEN];
    int operandCount = 0;

    // Remove colon from label
    strncpy(labelName, labelToken.lexeme, strlen(labelToken.lexeme) - 1);
    labelName[strlen(labelToken.lexeme) - 1] = '\0';

    // Get instruction and operands
    while (getNextToken(file, &token) && token.type != SPECIAL_SYMBOL) {
        if (token.type == INSTRUCTION || token.type == DIRECTIVE) {
            while (getNextToken(file, &token) && 
                   token.type != SPECIAL_SYMBOL && 
                   operandCount < MAX_OPERANDS) {
                strcpy(operands[operandCount++], token.lexeme);
            }
            break;
        }
    }

    // Store in symbol table
    strcpy(symbolTable[labelCount].name, labelName);
    symbolTable[labelCount].operand_count = operandCount;
    for (int i = 0; i < operandCount; i++) {
        strcpy(symbolTable[labelCount].operands[i], operands[i]);
    }
    symbolTable[labelCount].address = currentAddress;
    labelCount++;
    currentAddress += 4;  // Simple address increment
}

void analyzeAssemblyFile(const char *filename) {
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
            case INSTRUCTION: printf("INSTRUCTION"); break;
            case REGISTER: printf("REGISTER"); break;
            case LABEL: printf("LABEL"); break;
            case DIRECTIVE: printf("DIRECTIVE"); break;
            case NUMERIC_CONSTANT: printf("NUMERIC CONSTANT"); break;
            case STRING_LITERAL: printf("STRING LITERAL"); break;
            case SPECIAL_SYMBOL: printf("SPECIAL SYMBOL"); break;
            case MEMORY_REFERENCE: printf("MEMORY REFERENCE"); break;
            case OPERAND: printf("OPERAND"); break;
            default: printf("UNKNOWN"); break;
        }
        printf("\n");

        if (token.type == LABEL) {
            extractLabel(file, token);
        }
    }

    fclose(file);
}

void displaySymbolTable() {
    printf("\nSymbol Table (Labels and Addresses):\n");
    printf("--------------------------------------\n");
    for (int i = 0; i < labelCount; i++) {
        printf("Label: %s\tAddress: 0x%04X\n", symbolTable[i].name, symbolTable[i].address);
        if (symbolTable[i].operand_count > 0) {
            printf("Operands: ");
            for (int j = 0; j < symbolTable[i].operand_count; j++) {
                printf("%s", symbolTable[i].operands[j]);
                if (j < symbolTable[i].operand_count - 1) printf(", ");
            }
            printf("\n");
        }
        printf("--------------------------------------\n");
    }
}

int main() {
    char filename[100];
    printf("Enter the Assembly file name: ");
    scanf("%s", filename);
    analyzeAssemblyFile(filename);
    displaySymbolTable();
    return 0;
}