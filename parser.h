#ifndef IFJ_BRATWURST2021_PARSER_H
#define IFJ_BRATWURST2021_PARSER_H

#include <stdbool.h>
#include "scanner.h"
#include "symtable.h"
#include "sym_linked_list.h"

#define PARSE_NO_ERR false
#define PARSE_ERR true

typedef bool parser_error_t;

typedef enum e_arg_ret {
    ARG_DEF_TYPE,
    RET_DEF_TYPE
} arg_ret_t;

typedef struct ids_list {
    data_type_t type;
    struct ids_list* next;
} ids_list_t;

typedef struct p_data {
    token_t* token;
    LList* tbl_list;
    char* func_name;    

    arg_ret_t arg_ret;    
    data_type_t type;
    data_type_t psa_data_type; // psa data type

    symData_t* function_declaration_data;
    function_params_t* param;
    ids_list_t* ids_list;
} *p_data_ptr_t;

void idInsert(ids_list_t* ids_list, data_type_t type); /* TODO naimplementovat */
parser_error_t parser ();
void next_token(p_data_ptr_t data);

#endif //IFJ_BRATWURST2021_PARSER_H