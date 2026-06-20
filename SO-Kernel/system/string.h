/**
 * ============================================================================
 * STRING & MEMORY UTILS - CABEÇALHO (V1.1)
 * ============================================================================
 */

#ifndef STRING_H
#define STRING_H

#include <stdint.h>
#include <stddef.h>

// Memória
void* memset(void* s, int c, size_t n);
void* memcpy(void* dest, const void* src, size_t n);
int   memcmp(const void *s1, const void *s2, size_t n);

// Strings
size_t strlen(const char *str);
char* strcpy(char* dest, const char* src);
char* strcat(char* dest, const char* src);
int    strcmp(const char *s1, const char *s2);
int    strncmp(const char *s1, const char *s2, size_t n);
char* strncpy(char* dest, const char* src, size_t n);

// Conversões
void itoa(uint64_t n, char* str, int base);
int  atoi(const char* str);
char* strchr(const char* s, int c);

void int_to_string(int n, char* buffer); // Adicione esta linha aqui!
char* strtok(char* str, const char* delimiters);

#endif
