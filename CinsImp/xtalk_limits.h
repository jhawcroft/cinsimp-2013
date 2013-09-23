/*
 
 xTalk Engine (XTE), Limits
 xtalk_limits.h
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Design limitations
 
 *************************************************************************************************
 */

#ifndef XTALK_LIMITS_H
#define XTALK_LIMITS_H


/*
 *  XTALK_LIMIT_IDENTIFIER_CHARS
 *  ---------------------------------------------------------------------------------------------
 *  In practice, only limits the number of characters allowable for variable names, identifiers 
 *  and individual command syntax words (XTE_AST_WORD).
 *
 *  Exceeding this limit results in a syntax error.
 *
 *  Implemented in _lexer.c.
 */
#define XTALK_LIMIT_IDENTIFIER_CHARS 200

/*
 *  XTALK_LIMIT_STRING_LITERAL_CHARS
 *  ---------------------------------------------------------------------------------------------
 *  Restricts the maximum number of characters for a quoted string literal " ".
 *
 *  Exceeding this limit results in a syntax error.
 *
 *  Implemented in _lexer.c.
 */
#define XTALK_LIMIT_STRING_LITERAL_CHARS 1000000

/*
 *  XTALK_LIMIT_MAX_SOURCE_LINES
 *  ---------------------------------------------------------------------------------------------
 *  Restricts the maximum number of actual lines for a script.
 *
 *  Exceeding this limit results in a syntax error.
 *
 *  Implemented in _hdlr.c.
 */
#define XTALK_LIMIT_MAX_SOURCE_LINES 1000000

/*
 *  XTALK_LIMIT_MAX_NESTED_BLOCKS
 *  ---------------------------------------------------------------------------------------------
 *  Restricts the maximum number of nested blocks, where a block is a group of statements within
 *  a handler, a loop or a conditional structure.
 *
 *  Exceeding this limit results in a syntax error.
 *
 *  Implemented in _hdlr.c.
 */
#define XTALK_LIMIT_MAX_NESTED_BLOCKS 50

/*
 *  XTALK_LIMIT_SANE_ITEM_COUNT
 *  ---------------------------------------------------------------------------------------------
 *  Upper limit on the sane value of a count variable, ie. a variable that must hold a total
 *  number of items of a given thing, usually loaded into memory.  It must be a positive whole 
 *  number.  Should be set to something that could reasonably be held in memory.
 *
 *  ! Should exceed all other limits on the number of things of a specific type.
 *
 *  Designed to be used with IS_COUNT() macro.
 */
#define XTALK_LIMIT_SANE_ITEM_COUNT 25000000

#define XTALK_LIMIT_MAX_LINE_LENGTH_BYTES 8000

#define XTALK_LIMIT_NESTED_HANDLERS 500



#endif
