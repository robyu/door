#ifndef enum_descr_h
#define enum_descr_h

/*
macros for adding string "tags" to enums
e.g. see doormon.cpp::doormon_state_descr 
*/
typedef struct 
{
    int value;
    const char *pvalue_str;
} enum_descr_t;

#define ENUM_DESCR_DECLARE(ENUM) {ENUM, #ENUM}
#define ENUM_DESCR_END {0, NULL}

#endif // enum_descr_h

