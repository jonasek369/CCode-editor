#ifndef _H_UTILS
#define _H_UTILS

char* stringview_to_str(const char* view, size_t size) {
    if (!view || size == 0) {
        char* str = malloc(1);
        if (!str) return NULL;
        str[0] = '\0';
        return str;
    }

    char* str = malloc(size + 1);
    if (!str) return NULL;

    memcpy(str, view, size);
    str[size] = '\0';
    return str;
}

char* str_to_arr(const char* str){
    char* arr = NULL;
    for(int i = 0; i < strlen(str); i++){
        arrput(arr, str[i]);
    }
    arrput(arr, '\0');
    return arr;
}

bool can_read_file(const char* filepath) {
    #if _WIN32
    return _access(filepath, R_OK) == 0;
    #else
    return access(filepath, R_OK) == 0;
    #endif
}

char* resolve_path(const char *path, char *out){
    #ifdef _WIN32
    _fullpath(out, path, MAX_PATH);
    #else
    realpath(path, out);
    #endif
    return out;
}

#ifdef __linux__
void itoa(int num, char *str, int base) {
    (void) base;
    int i = 0;
    int isNegative = 0;

    if (num == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return;
    }

    if (num < 0) {
        isNegative = 1;
        num = -num;
    }

    while (num > 0) {
        str[i++] = (num % 10) + '0';
        num /= 10;
    }

    if (isNegative) {
        str[i++] = '-';
    }

    str[i] = '\0';

    int start = 0, end = i - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}

#endif

int32_t atoin(const char *str, int n) {
    int32_t result = 0;
    int32_t sign = 1;
    int i = 0;

    if(str[0] != '\0' && str[0] == '-'){
        sign = -1;
        i++;
    }

    while (i < n && str[i] != '\0') {
        result = result * 10 + (str[i] - '0');
        i++;
    }

    return result * sign;
}

#endif