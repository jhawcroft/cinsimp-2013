/*
 
 xTalk Engine Operator Parsing Unit
 xtalk_ops.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Responsible for parsing operators
 
 */

#include "xtalk_internal.h"


int _xte_operator_operand_count(XTEASTOperator in_op)
{
    switch (in_op)
    {
            /* unary */
        case XTE_AST_OP_NEGATE://- (preceeded by any operator, left parenthesis, beginning of stream, newline or OF)
        case XTE_AST_OP_NOT://not
        case XTE_AST_OP_THERE_IS_A:
        case XTE_AST_OP_THERE_IS_NO:
            return 1;
            
            /* binary */
        case XTE_AST_OP_EQUAL://is, =
        case XTE_AST_OP_NOT_EQUAL:// is not, <>
        case XTE_AST_OP_GREATER://>
        case XTE_AST_OP_LESSER://<
        case XTE_AST_OP_LESS_EQ://<=
        case XTE_AST_OP_GREATER_EQ://>=
        case XTE_AST_OP_CONCAT://&
        case XTE_AST_OP_CONCAT_SP://&&
        case XTE_AST_OP_EXPONENT://^
        case XTE_AST_OP_MULTIPLY://*
        case XTE_AST_OP_DIVIDE_FP:///
        case XTE_AST_OP_ADD://+
        case XTE_AST_OP_SUBTRACT://-
        case XTE_AST_OP_IS_IN: // reverse operands for "contains"
        case XTE_AST_OP_CONTAINS:
        case XTE_AST_OP_IS_NOT_IN:
        case XTE_AST_OP_DIVIDE_INT://div
        case XTE_AST_OP_MODULUS://mod
        case XTE_AST_OP_AND://and
        case XTE_AST_OP_OR://or
        case XTE_AST_OP_IS_WITHIN://is within (geometric)
        case XTE_AST_OP_IS_NOT_WITHIN://is not within (geometric)
            return 2;
            

    }
    return 0;
}


int _xte_is_unary_op(XTEAST *in_op)
{
    if (!in_op) return XTE_FALSE;
    if (in_op->type != XTE_AST_OPERATOR) return XTE_FALSE;
    switch (in_op->value.op)
    {
        case XTE_AST_OP_NEGATE:
        case XTE_AST_OP_NOT:
        case XTE_AST_OP_THERE_IS_A:
        case XTE_AST_OP_THERE_IS_NO:
            return XTE_TRUE;
        default: break;
    }
    return XTE_FALSE;
}


static void _parse_unary(XTE *in_engine, XTEAST *in_stmt)
{
    /* scan for unary operators */
    for (int i = 0; i < in_stmt->children_count; i++)
    {
        XTEAST *op = in_stmt->children[i];
        if (op && (op->type == XTE_AST_OPERATOR) && (op->children_count == 0))
        {
            switch (op->value.op)
            {
                case XTE_AST_OP_NEGATE:
                case XTE_AST_OP_NOT:
                case XTE_AST_OP_THERE_IS_A:
                case XTE_AST_OP_THERE_IS_NO:
                    /* found a unary operator; move the operand */
                    _xte_ast_list_append(op, _xte_ast_list_remove(in_stmt, i+1));
                    break;
                default:
                    break;
            }
        }
    }
}


static void _parse_binary(XTE *in_engine, XTEAST *in_stmt, const XTEASTOperator *in_ops)
{
    /* scan for binary operators */
    for (int i = 0; i < in_stmt->children_count; i++)
    {
        XTEAST *op = in_stmt->children[i];
        if (op && (op->type == XTE_AST_OPERATOR) && (op->children_count == 0))
        {
            const XTEASTOperator *op_ptr = in_ops;
            while (*op_ptr)
            {
                if (*op_ptr == op->value.op)
                {
                    /* found a binary operator; move the operands */
                    XTEAST *operand1, *operand2;
                    operand1 = _xte_ast_list_child(in_stmt, i-1);
                    operand2 = _xte_ast_list_child(in_stmt, i+1);
                    if (operand1 && operand2)
                    {
                        _xte_ast_list_append(op, _xte_ast_list_remove(in_stmt, i-1));
                        _xte_ast_list_append(op, _xte_ast_list_remove(in_stmt, i));
                        i--;
                    }
                    break;
                }
                op_ptr++;
            }
        }
    }
}


const XTEASTOperator OPS_MATH_SPECIAL[] = {
    XTE_AST_OP_EXPONENT, 0
};

const XTEASTOperator OPS_MATH_MPYDIV[] = {
    XTE_AST_OP_MULTIPLY, XTE_AST_OP_DIVIDE_FP, XTE_AST_OP_MODULUS, XTE_AST_OP_DIVIDE_INT, 0
};

const XTEASTOperator OPS_MATH_ADDSUB[] = {
    XTE_AST_OP_ADD, XTE_AST_OP_SUBTRACT, 0
};

const XTEASTOperator OPS_STRING[] = {
    XTE_AST_OP_CONCAT, XTE_AST_OP_CONCAT_SP, 0
};

const XTEASTOperator OPS_COMPREL[] = {
    XTE_AST_OP_GREATER_EQ, XTE_AST_OP_GREATER, XTE_AST_OP_LESS_EQ, XTE_AST_OP_LESSER,
    XTE_AST_OP_CONTAINS, XTE_AST_OP_IS_IN, XTE_AST_OP_IS_NOT_IN, XTE_AST_OP_IS_WITHIN, XTE_AST_OP_IS_NOT_WITHIN,
    XTE_AST_OP_THERE_IS_A, XTE_AST_OP_THERE_IS_NO, 0
};

const XTEASTOperator OPS_COMPEQ[] = {
    XTE_AST_OP_EQUAL, XTE_AST_OP_NOT_EQUAL, 0
};

const XTEASTOperator OPS_LOGIC_AND[] = {
    XTE_AST_OP_AND, 0
};

const XTEASTOperator OPS_LOGIC_OR[] = {
    XTE_AST_OP_OR, 0
};


void _parse_unary_ops(XTE *in_engine, XTEAST *in_stmt)
{
    /* scan for subexpressions */
    for (int i = 0; i < in_stmt->children_count; i++)
    {
        XTEAST *sexp = in_stmt->children[i];
        if (sexp && sexp->type == XTE_AST_EXPRESSION)
        /* parse subexpression */
            _parse_unary(in_engine, sexp);
    }
    
    _parse_unary(in_engine, in_stmt);
}


static void _parse_exp(XTE *in_engine, XTEAST *in_stmt)
{
    
    /* scan for subexpressions */
    for (int i = 0; i < in_stmt->children_count; i++)
    {
        XTEAST *sexp = in_stmt->children[i];
        if (sexp && sexp->type == XTE_AST_EXPRESSION)
        /* parse subexpression */
            _parse_exp(in_engine, sexp);
    }
    
    /* parse everything else in precedence order */
    _parse_unary(in_engine, in_stmt);
    
    _parse_binary(in_engine, in_stmt, OPS_MATH_SPECIAL);
    _parse_binary(in_engine, in_stmt, OPS_MATH_MPYDIV);
    _parse_binary(in_engine, in_stmt, OPS_MATH_ADDSUB);
    _parse_binary(in_engine, in_stmt, OPS_STRING);
    _parse_binary(in_engine, in_stmt, OPS_COMPREL);
    _parse_binary(in_engine, in_stmt, OPS_COMPEQ);
    _parse_binary(in_engine, in_stmt, OPS_LOGIC_AND);
    _parse_binary(in_engine, in_stmt, OPS_LOGIC_OR);
    
    
    //_xte_post_process_funcs(in_engine, in_stmt);
   // _xte_ast_debug(in_stmt);
}


void _xte_parse_operators(XTE *in_engine, XTEAST *in_stmt)
{
    _parse_exp(in_engine, in_stmt);
}



