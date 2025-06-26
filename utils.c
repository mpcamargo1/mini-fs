
#include <stdbool.h>
#include <stdio.h>
#include "utils.h"

// String
bool is_string_empty(const char *str) {
    return str == NULL || str[0] == '\0';
}