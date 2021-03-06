/**
 * Project: IFJ21 imperative language compiler
 * 
 * Brief:   Syntax and Semantic Analysis for IFJ21 compiler - psa part
 *  
 * Author:  Stepan Bakaj     <xbakaj00>
 * Author:  Radek Serejch    <xserej00> 
 * 
 */

#include <stdbool.h>
#include <string.h>

#include "parser.h"
#include "psa.h"
#include "symstack.h"
#include "data_types.h"
#include "error.h"
#include "code_generator.h"

#define P_TAB_SIZE 18

static char prec_table[P_TAB_SIZE][P_TAB_SIZE] = {
/*    *//* #   +   -   *   /   //  ..  <   >   <=  >=  ~=  ==  (   )   i   s   $*/
/* #  */ {' ','>','>','>',' ','>',' ','>','>','>','>','>','>','<','>','<','<','>'},
/* +  */ {'<','>','>','<','<','<',' ','>','>','>','>','>','>','<','>','<',' ','>'},
/* -  */ {'<','>','>','<','<','<',' ','>','>','>','>','>','>','<','>','<',' ','>'},
/* *  */ {'<','>','>','>','>','>',' ','>','>','>','>','>','>','<','>','<',' ','>'},
/* /  */ {'<','>','>','>','>','>',' ','>','>','>','>','>','>','<','>','<',' ','>'},
/* // */ {'<','>','>','>','>','>',' ','>','>','>','>','>','>','<','>','<',' ','>'},
/* .. */ {' ',' ',' ',' ',' ',' ','>',' ',' ',' ',' ',' ','>','<','>','<','<','>'},
/* <  */ {'<','<','<','<','<','<',' ','>','>','>','>','>','>','<','>','<','<','>'},
/* >  */ {'<','<','<','<','<','<',' ','>','>','>','>','>','>','<','>','<','<','>'},
/* <= */ {'<','<','<','<','<','<',' ','>','>','>','>','>','>','<','>','<','<','>'},
/* >= */ {'<','<','<','<','<','<',' ','>','>','>','>','>','>','<','>','<','<','>'},
/* ~= */ {'<','<','<','<','<','<',' ','>','>','>','>','>','>','<','>','<','<','>'},
/* == */ {'<','<','<','<','<','<','<','>','>','>','>','>','>','<','>','<','<','>'},
/* (  */ {'<','<','<','<','<','<','<','<','<','<','<','<','<','<','=','<','<',' '},
/* )  */ {' ','>','>','>','>','>','>','>','>','>','>','>','>',' ','>',' ',' ','>'},
/* i  */ {' ','>','>','>','>','>','>','>','>','>','>','>','>',' ','>',' ',' ','>'},
/* s  */ {' ','>','>','>',' ','>','>','>','>','>','>','>','>',' ','>',' ',' ','>'},
/* $  */ {'<','<','<','<','<','<','<','<','<','<','<','<','<','<',' ','<','<',' '},

};


// Function to get the index of the precedent table from the token
int get_index_token(p_data_ptr_t data){
    switch (data->token->type) {
        case T_MUL:
            return 3;
        case T_DIV:
            return 4;
        case T_INT_DIV:
            return 5;
        case T_PLUS:
            return 1;
        case T_MINUS:
            return 2;
        case T_CHAR_CNT:
            return 0;
        case T_CONCAT:
            return 6;
        case T_LESS_THAN:
            return 7;
        case T_LESS_EQ:
            return 9;
        case T_GTR_THAN:
            return 8;
        case T_GTR_EQ:
            return 10;
        case T_NOT_EQ:
            return 11;
        case T_EQ:
            return 12;
        case T_LEFT_BRACKET:
            return 13;
        case T_RIGHT_BRACKET:
            return 14;
        case T_IDENTIFIER:
            if (is_func(data->tbl_list,data->token->attribute.string)) {
                return 17;
            }
            return 15;
        case T_COMMA:
            return 17;
        case T_DECIMAL:
            return 15;
        case T_INT:
            return 15;
        case T_STRING:
            return 16;
        case T_DECIMAL_W_EXP:
            return 15;
        case T_KEYWORD:
            if(data->token->attribute.keyword == K_THEN || data->token->attribute.keyword == K_DO
                || data->token->attribute.keyword == K_LOCAL || data->token->attribute.keyword == K_IF
                || data->token->attribute.keyword == K_WHILE || data->token->attribute.keyword == K_RETURN
                || data->token->attribute.keyword == K_END || data->token->attribute.keyword == K_ELSE
                || data->token->attribute.keyword == K_FUNCTION || data->token->attribute.keyword == K_GLOBAL) {
                return 17;//continue to case $
            }
            else if(data->token->attribute.keyword == K_NIL){
                return 15;
            }
            else{
                break;
            }
        default:
            return -1;
    }
    return -1;
}

// Function to get the index of the precedent table from the symbol
int get_index_enum(psa_table_symbol_enum e){
    switch (e) {
        case MUL:
            return 3;
        case DIV:
            return 4;
        case INT_DIV:
            return 5;
        case PLUS:
            return 1;
        case MINUS:
            return 2;
        case HASHTAG:
            return 0;
        case CONCAT:
            return 6;
        case LESS_THAN:
            return 7;
        case LESS_EQ:
            return 9;
        case GTR_THAN:
            return 8;
        case GTR_EQ:
            return 10;
        case NOT_EQ:
            return 11;
        case EQ:
            return 12;
        case LEFT_BRACKET:
            return 13;
        case RIGHT_BRACKET:
            return 14;
        case IDENTIFIER:
            return 15;
        case INT_NUMBER:
            return 15;
        case STRN:
            return 16;
        case DOUBLE_NUMBER:
            return 15;
        case DOLLAR:
            return 17;
        default:
            return -1;
    }
    return -1;
}

/**
 * Function tests if symbols in parameters are valid according to rules.
 *
 * @param num Number of valid symbols in parameter.
 * @param op1 Symbol 1.
 * @param op2 Symbol 2.
 * @param op3 Symbol 3.
 * @return NOT_A_RULE if no rule is found or returns rule which is valid.
 */
static psa_rules_enum test_rule(int num, sym_stack_item* op1, sym_stack_item* op2, sym_stack_item* op3)
{
    switch (num)
    {
        case 1:
            // rule E -> i
            if (op1->symbol == IDENTIFIER || op1->symbol == INT_NUMBER || op1->symbol == DOUBLE_NUMBER || op1->symbol == STRN)
                return OPERAND;

            return NOT_A_RULE;
        case 2:
            // rule E -> #E
            if (op2->symbol == HASHTAG && op1->symbol == NON_TERM)
                return NT_HASHTAG;
            return NOT_A_RULE;
        case 3:
            // rule E -> (E)
            if (op3->symbol == LEFT_BRACKET && op2->symbol == NON_TERM && op1->symbol == RIGHT_BRACKET)
                return LBR_NT_RBR;

            if (op1->symbol == NON_TERM && op3->symbol == NON_TERM)
            {
                switch (op2->symbol)
                {
                    // rule E -> E + E
                    case PLUS:
                        return NT_PLUS_NT;
                    // rule E -> E - E
                    case MINUS:
                        return NT_MINUS_NT;
                    // rule E -> E .. E
                    case CONCAT:
                        return NT_CONCAT_NT;
                    // rule E -> E * E
                    case MUL:
                        return NT_MUL_NT;
                    // rule E -> E / E
                    case DIV:
                        return NT_DIV_NT;
                    // rule E -> E // E
                    case INT_DIV:
                        return NT_IDIV_NT;
                    // rule E -> E = E
                    case EQ:
                        return NT_EQ_NT;
                    // rule E -> E ~= E
                    case NOT_EQ:
                        return NT_NEQ_NT;
                    // rule E -> E <= E
                    case LESS_EQ:
                        return NT_LEQ_NT;
                    // rule E -> E < E
                    case LESS_THAN:
                        return NT_LTN_NT;
                    // rule E -> E >= E
                    case GTR_EQ:
                        return NT_GEQ_NT;
                    // rule E -> E > E
                    case GTR_THAN:
                        return NT_GTN_NT;
                    // invalid operator
                    default:

                        return NOT_A_RULE;
                }
            }
            return NOT_A_RULE;
    }
    return NOT_A_RULE;
}

/**
 * Function converts token type to symbol.
 *
 * @param token Pointer to token.
 * @return Returns dollar if symbol is not supported or converted symbol if symbol is supported.
 */
static psa_table_symbol_enum get_symbol_from_token(token_t *token)
{
    switch (token->type)
    {
        case T_CHAR_CNT:
            return HASHTAG;
        case T_CONCAT:
            return CONCAT;
        case T_PLUS:
            return PLUS;
        case T_MINUS:
            return MINUS;
        case T_MUL:
            return MUL;
        case T_DIV:
            return DIV;
        case T_INT_DIV:
            return INT_DIV;
        case T_EQ:
            return EQ;
        case T_NOT_EQ:
            return NOT_EQ;
        case T_LESS_EQ:
            return LESS_EQ;
        case T_LESS_THAN:
            return LESS_THAN;
        case T_GTR_EQ:
            return GTR_EQ;
        case T_GTR_THAN:
            return GTR_THAN;
        case T_LEFT_BRACKET:
            return LEFT_BRACKET;
        case T_RIGHT_BRACKET:
            return RIGHT_BRACKET;
        case T_IDENTIFIER:
            return IDENTIFIER;
        case T_INT:
            return INT_NUMBER;
        case T_DECIMAL:
            return DOUBLE_NUMBER;
        case T_DECIMAL_W_EXP:
            return DOUBLE_NUMBER;
        case T_STRING:
            return STRN;
        case T_KEYWORD:
            if(token->attribute.keyword == K_NIL){
                return IDENTIFIER;
            }else{
                return DOLLAR;
            }
        default:
            return DOLLAR;
    }
}

// Function for obtaining data type for semantic analysis
data_type_t get_type(p_data_ptr_t data){
    switch(data->token->type){
        case T_MUL:
            return OP;
        case T_DIV:
            return OP;
        case T_INT_DIV:
            return OP;
        case T_PLUS:
            return OP;
        case T_MINUS:
            return OP;
        case T_CHAR_CNT:
            return OP;
        case T_CONCAT:
            return OP;
        case T_LESS_THAN:
            return OP;
        case T_LESS_EQ:
            return OP;
        case T_GTR_THAN:
            return OP;
        case T_GTR_EQ:
            return OP;
        case T_NOT_EQ:
            return OP;
        case T_ASSIGN:
            return OP;
        case T_EQ:
            return OP;
        case T_COLON:
            return ELSE;
        case T_LEFT_BRACKET:
            return OP;
        case T_RIGHT_BRACKET:
            return OP;
        case T_COMMA:
            return ELSE;
        case T_INT:
            return INT;
        case T_DECIMAL:
            return NUMBER;
        case T_DECIMAL_W_EXP:
            return NUMBER;
        case T_IDENTIFIER:  
            // For the identifier have to check if it has been defined                   
            if(!check_identifier_is_defined(data->tbl_list,data->token->attribute.string)){                
                return DERR;
            }
            return identifier_type(data->tbl_list,data->token->attribute.string);
        case T_KEYWORD:
            if(data->token->attribute.keyword == K_NIL){
                return NIL;
            }else{
                return ELSE;
            }
        case T_STRING:
            return STR;
        default:
            return ELSE;
    }
}

// Function for checking semantics when reducing expression
static bool check_semantic(psa_rules_enum rule, sym_stack_item* op1, sym_stack_item* op2, sym_stack_item* op3, data_type_t* final_type){

    bool op1_to_number = false;
    bool op3_to_number = false;

    switch (rule) {                
        case NT_HASHTAG: ;
            if (op2->data != STR){
                err = E_SEM_INCOMPATIBLE;
                return false;
            }
            *final_type = INT;
            break;
        case NT_CONCAT_NT: ;
            if (op1->data != STR || op3->data != STR){
                err = E_SEM_INCOMPATIBLE;
                return false;
            }
            *final_type = STR;
            break;
        case NT_PLUS_NT: ;
        case NT_MINUS_NT: ;
        case NT_MUL_NT: ;
            // Error
            if (op1->data == STR || op3->data == STR){
                err = E_SEM_INCOMPATIBLE;
                return false;
            }

            if (op1->data == INT && op3->data == INT){
                *final_type = INT;
                break;
            }
            if (op1->data == NUMBER && op3->data == NUMBER){
                *final_type = NUMBER;
                break;
            }

            // Retotyping
            if (op1->data == INT){
                op1_to_number = true;
                *final_type = NUMBER;
            }
            if (op3->data == INT){
                op3_to_number = true;
                *final_type = NUMBER;
            }
            break;
        case NT_DIV_NT: ;
            *final_type = NUMBER;

            // Error
            if (op1->data == STR || op3->data == STR){
                err = E_SEM_INCOMPATIBLE;
                return false;
            }

            // Retotyping
            if (op1->data == INT){
                op1_to_number = true;
            }
            if (op3->data == INT){
                op3_to_number = true;
            }

            break;
        case NT_IDIV_NT: ;
            *final_type = INT;

            // Error
            if (op1->data == STR || op3->data == STR){
                err = E_SEM_INCOMPATIBLE;
                return false;
            }

            break;
        case NT_EQ_NT: ;
        case NT_NEQ_NT: ;
            *final_type = ELSE;

            // Error
            if (op1->data == STR && ( op3->data == INT || op3->data == NUMBER )){
                err = E_SEM_INCOMPATIBLE;
                return false;
            }
            if (( op1->data == INT || op1->data == NUMBER ) && op3->data == STR){
                err = E_SEM_INCOMPATIBLE;
                return false;
            }


            if (op1->data == STR && op3->data == STR){
                break;
            }
            if (op1->data == INT && op3->data == INT){
                break;
            }
            if (op1->data == NUMBER && op3->data == NUMBER){
                break;
            }

            // Retotyping
            if (op1->data == INT){
                op1_to_number = true;
            }
            if (op3->data == INT){
                op3_to_number = true;
            }
          
            break;
        case NT_LEQ_NT: ;
        case NT_GEQ_NT: ;
        case NT_LTN_NT: ;
        case NT_GTN_NT: ;
            *final_type = ELSE;

            if ((op1->data == STR && (op3->data == INT || op3->data == NUMBER) ) || ( (op1->data == INT || op1->data == NUMBER) && op3->data == STR)){
                err = E_SEM_INCOMPATIBLE;
                return false;
            }

            if (op1->data == STR && op3->data == STR){
                break;
            }

            if (op1->data == INT && op3->data == INT){
                break;
            }
            if (op1->data == NUMBER && op3->data == NUMBER){
                break;
            }

            // Retotyping
            if (op1->data == INT){
                op1_to_number = true;
            }
            if (op3->data == INT){
                op3_to_number = true;
            }


            break;
        default: ;
            break;
    }

    if(op1_to_number == true){
        // Generate code for retotyping (first on stack)       
        generate_IntToFloat1();
    }
    if(op3_to_number == true){
        // Generate code for retotyping (second on stack)    
        generate_IntToFloat2();
    }

    return true;
}

psa_error_t psa (p_data_ptr_t data)
{
    sym_stack stack;
    sym_stack_init(&stack);
    symbol_stack_push(&stack,DOLLAR,ELSE);

    int ind_a;
    int ind_b;
    sym_stack_item* a;
    bool end_while = false;
    data_type_t final_type = ELSE;
    do{
        // Stack token and input token
        a = symbol_stack_top_terminal(&stack);
        
        // Indexes of tokens in the table
        ind_a = get_index_enum(a->symbol);

        ind_b = get_index_token(data);

        if(data->token->type == T_ASSIGN){
            err = E_INTERNAL;
            return PSA_ERR;
        }
        
        // If there is another after one identifier, it is already part of another expression
        if ((ind_b == 15 )&& (ind_a == 15 || ind_a == 16 || symbol_stack_top(&stack)->symbol == NON_TERM))
        {
            ind_b = 17;
        }        


        // Data from table
        char tbl_data = prec_table[ind_a][ind_b];        

        // Action according to data from the precedence table
        switch (tbl_data) {
            case '=': ;
                if(get_type(data) == DERR){                                  
                    err = E_SEM_DEF;                    
                    return PSA_ERR;
                }
                if(!symbol_stack_push(&stack, get_symbol_from_token(data->token),get_type(data))){
                    err = E_INTERNAL;
                    return PSA_ERR;
                }
                next_token(data);
                break;
            case '<': ;                
                // Insert after top terminal
                if(!symbol_stack_insert_after_top_terminal(&stack,STOP,ELSE)){                                     
                    err = E_INTERNAL;
                    return PSA_ERR;
                }
                
                if(get_type(data) == DERR){                                                                                              
                    err = E_SEM_DEF;                    
                    return PSA_ERR;
                }
                if(!symbol_stack_push(&stack, get_symbol_from_token(data->token), get_type(data)))
                {                    
                    err = E_INTERNAL;
                    return PSA_ERR;
                }

                // Generate code
                // If it is an identifier, it can be a function
                if(data->token->type == T_IDENTIFIER){
                    char* id = (char*) malloc(strlen(data->token->attribute.string) + 1);

                    if (id == NULL)
                    {
                        err = E_INTERNAL;
                        return PSA_ERR;
                    }
                    
                    strcpy(id, data->token->attribute.string);

                    next_token(data);

                    if (data->token->type == T_LEFT_BRACKET)
                    {
                        free(id);
                        id = NULL;                        
                        return PSA_ERR;
                    }
                    /*
                     * If it is not a function, I will generate code to push the value of the variable
                     * to the stack in the resulting code
                     */ 
                    else
                    {
                        // If it's just a value, I'll run its value on the stack in the resulting code
                        codeGen_push_var(id);  
                        free(id);
                        id = NULL;
                    }                                                          
                }else{
                    switch(get_type(data)){
                        case INT:
                            codeGen_push_int(data->token->attribute.integer);
                        break;
                        case NUMBER:
                            codeGen_push_float(data->token->attribute.decimal);
                        break;
                        case STR:
                            codeGen_push_string(data->token->attribute.string);
                        break;
                        case NIL:
                            codeGen_push_nil();
                        break;
                        default:
                        break;
                    }

                    next_token(data);
                }                                
                
                break;
            case '>': ;
                // Reduction
                // Find out how many symbols I have on the stack by <
                sym_stack_item symbol1;
                sym_stack_item symbol2;
                sym_stack_item symbol3;
                int num=0;
            
                if(symbol_stack_top(&stack)->symbol != STOP){
                    symbol1 = *(symbol_stack_top(&stack));
                    symbol_stack_pop(&stack);
                    num++;
                
                    if(symbol_stack_top(&stack)->symbol != STOP){
                        symbol2 = *(symbol_stack_top(&stack));
                        symbol_stack_pop(&stack);
                        num++;
                        
                        if(symbol_stack_top(&stack)->symbol != STOP){
                            symbol3 = *(symbol_stack_top(&stack));
                            symbol_stack_pop(&stack);
                            num++;
                            
                            // Ma three symbols, and behind the last stop sign
                            if(symbol_stack_top(&stack)->symbol != STOP)
                            {                                                      
                                err = E_INTERNAL;
                                return PSA_ERR;
                            }                                
                        }
                    }
                }
                // Remove the stopwatch from the stack
                if(symbol_stack_top(&stack)->symbol == STOP){
                    symbol_stack_pop(&stack);
                }
                                
                // Find out if there is a rule for the given symbols from the stack
                psa_rules_enum rule = test_rule(num,&symbol1,&symbol2,&symbol3);
		
                /*
                 * According to the rule, the semantics are checked, the code for the given operation is generated, 
                 * and the resulting nonterminal is returned to the stack
                 */ 
                switch (rule) {                    
                    case OPERAND:
                        // rule E -> i                        
                        if(!symbol_stack_push(&stack,NON_TERM,symbol1.data)){

                            err = E_INTERNAL;
                            return PSA_ERR;
                        }
                        break;
                    case NT_HASHTAG:
                        // rule E -> #E      
                        // Check semantic                  
                        if(symbol1.data != STR){
                            err = E_SEM_INCOMPATIBLE;
                            return PSA_ERR;
                        }
                        if(!symbol_stack_push(&stack,NON_TERM,INT)){    
                        
                            err = E_INTERNAL;
                            return PSA_ERR;
                        }
                        generate_operation(rule);
                        break;
                    case LBR_NT_RBR:
                        // rule E -> (E)
                        if(!symbol_stack_push(&stack,NON_TERM,symbol2.data)){                            
                            err = E_INTERNAL;
                            return PSA_ERR;
                        }
                        break;
                    case NT_CONCAT_NT:
                        // rule E -> E .. E
                        if(!check_semantic(NT_CONCAT_NT,&symbol1,&symbol2,&symbol3,&final_type)){
                            return PSA_ERR;
                        }
                        if(!symbol_stack_push(&stack,NON_TERM,final_type)){                            
                            err = E_INTERNAL;
                            return PSA_ERR;
                        }                        
                        generate_operation(rule);
                        break;
                    case NT_PLUS_NT:
                        // rule E -> E + E
                        if(!check_semantic(NT_PLUS_NT,&symbol1,&symbol2,&symbol3,&final_type)){
                            return PSA_ERR;
                        }
                        if(!symbol_stack_push(&stack,NON_TERM,final_type)){                            
                            err = E_INTERNAL;
                            return PSA_ERR;
                        }
                        generate_operation(rule);
                        break;
                    case NT_MINUS_NT:
                        // rule E -> E - E
                        if(!check_semantic(NT_MINUS_NT,&symbol1,&symbol2,&symbol3,&final_type)){
                            return PSA_ERR;
                        }
                        if(!symbol_stack_push(&stack,NON_TERM,final_type)){                            
                            err = E_INTERNAL;
                            return PSA_ERR;
                        }
                        generate_operation(rule);
                        break;
                    case NT_MUL_NT:
                        // rule E -> E * E
                        if(!check_semantic(NT_MUL_NT,&symbol1,&symbol2,&symbol3,&final_type)){
                            return PSA_ERR;
                        }
                        if(!symbol_stack_push(&stack,NON_TERM,final_type)){                            
                            err = E_INTERNAL;
                            return PSA_ERR;
                        }
                        generate_operation(rule);
                        break;
                    case NT_DIV_NT:
                        // rule E -> E / E
                        if(!check_semantic(NT_DIV_NT,&symbol1,&symbol2,&symbol3,&final_type)){
                            return PSA_ERR;
                        }
                        if(!symbol_stack_push(&stack,NON_TERM,final_type)){
                            err = E_INTERNAL;
                            return PSA_ERR;
                        }
                        generate_operation(rule);
                        break;
                    case NT_IDIV_NT:
                        // rule E -> E // E
                        if(!check_semantic(NT_IDIV_NT,&symbol1,&symbol2,&symbol3,&final_type)){
                            return PSA_ERR;
                        }
                        if(!symbol_stack_push(&stack,NON_TERM,final_type)){
                            err = E_INTERNAL;
                            return PSA_ERR;
                        }
                        generate_operation(rule);
                        break;
                    case NT_EQ_NT:
                        // rule E -> E == E
                        if(!check_semantic(NT_EQ_NT,&symbol1,&symbol2,&symbol3,&final_type)){
                            return PSA_ERR;
                        }
                        if(!symbol_stack_push(&stack,NON_TERM,final_type)){
                            err = E_INTERNAL;
                            return PSA_ERR;
                        }
                        generate_operation(rule);
                        break;
                    case NT_NEQ_NT:
                        // rule E -> E ~= E
                        if(!check_semantic(NT_NEQ_NT,&symbol1,&symbol2,&symbol3,&final_type)){
                            return PSA_ERR;
                        }
                        if(!symbol_stack_push(&stack,NON_TERM,final_type)){                            
                            err = E_INTERNAL;
                            return PSA_ERR;
                        }
                        generate_operation(rule);
                        break;
                    case NT_LEQ_NT:
                        // rule E -> E <= E
                        if(!check_semantic(NT_LEQ_NT,&symbol1,&symbol2,&symbol3,&final_type)){
                            return PSA_ERR;
                        }
                        if(!symbol_stack_push(&stack,NON_TERM,final_type)){ 
                            err = E_INTERNAL;
                            return PSA_ERR;
                        }
                        generate_operation(rule);
                        break;
                    case NT_GEQ_NT:
                        // rule E -> E >= E
                        if(!check_semantic(NT_GEQ_NT,&symbol1,&symbol2,&symbol3,&final_type)){
                            return PSA_ERR;
                        }
                        if(!symbol_stack_push(&stack,NON_TERM,final_type)){
                            err = E_INTERNAL;
                            return PSA_ERR;
                        }
                        generate_operation(rule);
                        break;
                    case NT_LTN_NT:
                        // rule E -> E < E
                        if(!check_semantic(NT_LTN_NT,&symbol1,&symbol2,&symbol3,&final_type)){
                            return PSA_ERR;
                        }
                        if(!symbol_stack_push(&stack,NON_TERM,final_type)){
                            err = E_INTERNAL;
                            return PSA_ERR;
                        }
                        generate_operation(rule);
                        break;
                    case NT_GTN_NT:
                        // rule E -> E > E
                        if(!check_semantic(NT_GTN_NT,&symbol1,&symbol2,&symbol3,&final_type)){
                            return PSA_ERR;
                        }
                        if(!symbol_stack_push(&stack,NON_TERM,final_type)){
                            err = E_INTERNAL;
                            return PSA_ERR;
                        }
                        generate_operation(rule);
                        break;
                    case NOT_A_RULE:                      
                        err = E_SYNTAX;
                        return PSA_ERR;
                        break;
                }
                
                // Check if I'm not at the end of the reduction
                if(ind_b == 17 && symbol_stack_top_terminal(&stack)->symbol == DOLLAR){                    
                    end_while = true;
                }
                break;
                
            default:
                err = E_SYNTAX;            
                return PSA_ERR;                
        }

    }while(!end_while);

    /*
     * At the end of the reduction there should be a nonterminal at the top of the stack
     *
     * Save the data type of the resulting terminal for subsequent comparison 
     * with the type of variable in which I save the expression
     */
    if(symbol_stack_top(&stack)->symbol == NON_TERM)
    {        
        if (data->if_while == true && symbol_stack_top(&stack)->data != ELSE){
            generate_toBool();
        }
        data->psa_data_type = symbol_stack_top(&stack)->data;
        symbol_stack_free(&stack);
        return PSA_NO_ERR;    
    }
    
    return  PSA_ERR;
}
