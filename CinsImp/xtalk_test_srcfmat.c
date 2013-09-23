/*
 
 xTalk Engine Tests: Source Formatting
 xtalk_test_exprs.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Unit tests for source formatting
 
 *************************************************************************************************
 */

#include "xtalk_internal.h"



#if XTALK_TESTS


static const char *TEST_1_SOURCE =
"  on mouseUp\n"
"beep 3 times\n"
"put 5 * 2 into x\n"
"repeat with x = 1 to 5\n"
"put 9 * x\n"
"if pies = 2\n"
"then -- check for too many pies\n"
"put \"Too many pies!\"\n"
"else if bob then\n"
"beep 2 times\n"
"else\n"
"bop()\n"
"end if\n"
"end repeat -- finish looping\n"
"answer \"Hello World.\"\n"
"end mouseUp\n"
"\n"
"\n"
"-- more comments\n"
"-- on these lines\n";

static const char *TEST_1_RESULT =
"on mouseUp\n"
"  beep 3 times\n"
"  put 5 * 2 into x\n"
"  repeat with x = 1 to 5\n"
"    put 9 * x\n"
"    if pies = 2\n"
"    then -- check for too many pies\n"
"      put \"Too many pies!\"\n"
"    else if bob then\n"
"      beep 2 times\n"
"    else\n"
"      bop()\n"
"    end if\n"
"  end repeat -- finish looping\n"
"  answer \"Hello World.\"\n"
"end mouseUp\n"
"\n"
"\n"
"-- more comments\n"
"-- on these lines\n";


static const char *TEST_2_SOURCE =
"  on mouseUp\n"
"beep 3 times\n"
"put 5 * 2 into x\n"
"      repeat with x = 1 to 5\n"
"      put 9 * x\n"
"      if pies = 2\n" // this line's indentation is causing an issue
"      then -- check for too many pies\n"
"      put \"Too many pies!\"\n"
"      else if bob then\n"
"      beep 2 times\n"
"      else\n"
"      bop()\n"
"      end if\n"
"      end repeat -- finish looping\n"
"      answer \"Hello World.\"\n"
"      end mouseUp\n"
"\n"
"\n"
"-- more comments\n"
"-- on these lines\n";

static const char *TEST_2_RESULT =
"on mouseUp\n"
"  beep 3 times\n"
"  put 5 * 2 into x\n"
"  repeat with x = 1 to 5\n"
"    put 9 * x\n"
"    if pies = 2\n"
"    then -- check for too many pies\n"
"      put \"Too many pies!\"\n"
"    else if bob then\n"
"      beep 2 times\n"
"    else\n"
"      bop()\n"
"    end if\n"
"  end repeat -- finish looping\n"
"  answer \"Hello World.\"\n"
"end mouseUp\n"
"\n"
"\n"
"-- more comments\n"
"-- on these lines\n";


static const char *TEST_3_SOURCE =
"on mouseUp\n"//11
"if true then\n"//13
"bop\n" // 27 @ end of bop
"boop\n"
"end if\n"
"if false then bop\n"
"boop\n"
"if true then bop\n"
"else bip\n"
"if false then bop\n"
"else if true then beep\n"
"else if zork then zog\n"
"end mouseUp\n"
;

static const char *TEST_3_RESULT =
"on mouseUp\n"//11
"  if true then\n"//15
"    bop\n"// should be 33 @ end of bop
"    boop\n"
"  end if\n"
"  if false then bop\n"
"  boop\n"
"  if true then bop\n"
"  else bip\n"
"  if false then bop\n"
"  else if true then beep\n"
"  else if zork then zog\n"
"end mouseUp\n"
;


static const char *TEST_4_SOURCE =
"on mouseUp\n"//11
"repeat blah\n"//12
"put 9 * x\n"//10
"next repeat\n"// @ 33 begin of line
"exit repeat\n"
"end repeat\n"
"end mouseUp\n"
;

static const char *TEST_4_RESULT =
"on mouseUp\n"//11
"  repeat blah\n"//14
"    put 9 * x\n"//14
"    next repeat\n"// @ 39
"    exit repeat\n"
"  end repeat\n"
"end mouseUp\n"
;



static const char *TEST_5_SOURCE =
"on mouseUp\n"
"if bob is your auntie then\n"
" repeat until bob is your uncle  \n"
"  change his sex a bit -- just a little \n"
"  -- not too much!\n"
"  if he squirms too much\n"
"  then giveHimAnetsthetic 0.5\n"
"  else\n"
"       put \"Procedure is going well.\"\n"
"  end if\n"
"  if he is dead then exit repeat\n"
"end repeat\n"
"if bob is dead then\n"
" answer \"Sorry we killed Bob.\"\n"
"else\n"
"  repeat with x = 1 to 5\n"
"   put \"Horray, we fixed Bob!\"\n"
" end repeat\n"
"end if\n"
"else put \"Yay! Bob is 'normal'\"\n"
"cleanup\n"
"end mouseUp\n"
;

static const char *TEST_5_RESULT =
"on mouseUp\n"
"  if bob is your auntie then\n"
"    repeat until bob is your uncle  \n"
"      change his sex a bit -- just a little \n"
"      -- not too much!\n"
"      if he squirms too much\n"
"      then giveHimAnetsthetic 0.5\n"
"      else\n"
"        put \"Procedure is going well.\"\n"
"      end if\n"
"      if he is dead then exit repeat\n"
"    end repeat\n"
"    if bob is dead then\n"
"      answer \"Sorry we killed Bob.\"\n"
"    else\n"
"      repeat with x = 1 to 5\n"
"        put \"Horray, we fixed Bob!\"\n"
"      end repeat\n"
"    end if\n"
"  else put \"Yay! Bob is 'normal'\"\n"
"  cleanup\n"
"end mouseUp\n"
;


static const char *TEST_6_SOURCE =
"on mouseUp\n"//11
"if bob is your auntie\n"//22
" repeat until bob is your uncle\n"//@47 bob begin
"  change his sex a bit\n"
;

static const char *TEST_6_RESULT =
"on mouseUp\n"
"  if bob is your auntie\n"
"  repeat until bob is your uncle\n"//@50
"  change his sex a bit\n"
"  "
;


static const char *TEST_7_SOURCE =
"on mouseup\n"
"beep 3 times\n"
"visual effect dissolve\n"
"if there is a card \"boo\" then\n"
"\n"
"go to next card\n"
"end mouseup\n"
;

static const char *TEST_7_RESULT =
"on mouseup\n"
"  beep 3 times\n"
"  visual effect dissolve\n"
"  if there is a card \"boo\" then\n"
"    \n"
"    go to next card\n"
"    end mouseup\n"
"    "
;



/*
 
 This test cannot work with our current test harness, because we don't manipulate characters
 in our adjustment handler - as we should.
 
static const char *TEST_8_SOURCE =
"on dummy\n"
"-- this just takes up space\n"
"put 5 ¬\n"
"into x\n"
"end dummy"
;

static const char *TEST_8_RESULT =
"on dummy\n"
"  -- this just takes up space\n"
"  put 5 ¬\n"
"  into x\n"
"end dummy"
;
*/



static char *g_source = NULL;
static XTE *g_engine;


/* this method isn't very good - will probably need fixes */
// problem here might actually have to do with when there's a syntax error, and hanging indent becomes fixed
// particularly for comment lines... 
static void do_indent(XTE *in_engine, void *in_context, long in_char_begin, long in_char_length, char const *in_spaces, int in_space_count)
{
    //printf("DO-INDENT: %ld, %ld: \"%s\" ::\n%s\n", in_char_begin, in_char_length, in_spaces, g_source + in_char_begin);
    memmove(g_source + in_char_begin + in_space_count, g_source + in_char_begin + in_char_length,
            strlen(g_source) - (in_char_begin + in_char_length) + 1);
    //if (in_char_begin + in_char_length == 0) g_source[strlen(g_source)] = 0;
    memcpy(g_source + in_char_begin, in_spaces, in_space_count);
    
    //printf("---ADJUSTED---\n%s\n", g_source);
}


static int _run_test(char const *in_source, char const *in_result, long in_selection_offset, long in_required_selection, int in_number)
{
    long selection_offset = in_selection_offset;
    long selection_length = 0;
    int result;
    
    strcpy(g_source, in_source);
    xte_source_indent(g_engine, in_source, &selection_offset, &selection_length, &do_indent, NULL);
    result = (strcmp(g_source, in_result) == 0);
    if (!result)
    {
        printf("Failed srcfmat test %d!\n", in_number);
        printf("==============\nSource:\n%s\n----------", in_source);
        printf("==============\nOutput:\n%s\n----------", g_source);
        printf("==============\nRequired:\n%s\n-------------\n", in_result);
        return XTE_FALSE;
    }
    
    if (selection_offset != in_required_selection)
    {
        printf("Failed srcfmat test %d!\n", in_number);
        printf("  Selection offset doesn't match: %ld (result) != %ld (required)\n", selection_offset, in_required_selection);
        return XTE_FALSE;
    }
    
    return XTE_TRUE;
}


/* test runner */
void _xte_srcfmat_test(void)
{
    /* prepare */
    g_engine = xte_create(NULL);
    g_source = malloc(4 * 1024 * 1024);
    assert(g_engine != NULL);
    assert(g_source != NULL);
    
    /* tests */
    if (!_run_test(TEST_1_SOURCE, TEST_1_RESULT, 5, 3, 1)) goto srcfmat_cleanup;
    if (!_run_test(TEST_1_SOURCE, TEST_1_RESULT, 32, 34, 1)) goto srcfmat_cleanup;
    if (!_run_test(TEST_2_SOURCE, TEST_2_RESULT, 32, 34, 2)) goto srcfmat_cleanup;
    if (!_run_test(TEST_3_SOURCE, TEST_3_RESULT, 27, 33, 3)) goto srcfmat_cleanup;
    if (!_run_test(TEST_4_SOURCE, TEST_4_RESULT, 33, 39, 4)) goto srcfmat_cleanup;
    if (!_run_test(TEST_5_SOURCE, TEST_5_RESULT, 323, 370, 5)) goto srcfmat_cleanup;
    if (!_run_test(TEST_6_SOURCE, TEST_6_RESULT, 47, 50, 6)) goto srcfmat_cleanup;
    if (!_run_test(TEST_7_SOURCE, TEST_7_RESULT, 0, 0, 7)) goto srcfmat_cleanup;
    //if (!_run_test(TEST_8_SOURCE, TEST_8_RESULT, 0, 0, 7)) goto srcfmat_cleanup;
    
    /* cleanup */
srcfmat_cleanup:
    free(g_source);
    xte_dispose(g_engine);
}




/*
 // this function doesn't pass memory testing, as it contains a static variable
 static const char* _quote_string_lines(const char *in_string)
 {
 long length = strlen(in_string);
 int line_quote_count = 0;
 long line_offset = 0;
 long line_length;
 static char *output_buffer = NULL;
 if (output_buffer) free(output_buffer);
 output_buffer = NULL;
 char *new_output_buffer;
 long output_buffer_size = 0;
 for (long i = 0; i <= length; i++)
 {
 if (in_string[i] == '"') line_quote_count++;
 else if ((in_string[i] == '\n') || (i == length))
 {
 if ((in_string[i] != '\n') && (i == length) && (i - line_offset == 0)) break;
 line_length = i - line_offset;
 long line_alloc = line_length + line_quote_count + 5;
 new_output_buffer = realloc(output_buffer, output_buffer_size + line_alloc + 1);
 if (!new_output_buffer)
 {
 if (output_buffer) free(output_buffer);
 return NULL;
 }
 output_buffer = new_output_buffer;
 long output_ptr = output_buffer_size;
 output_buffer_size += line_alloc;
 output_buffer[output_ptr++] = '"';
 for (int c = 0; c < line_length; c++)
 {
 if (in_string[line_offset + c] == '"')
 output_buffer[output_ptr++] = '\\';
 output_buffer[output_ptr++] = in_string[line_offset + c];
 }
 if (in_string[i] == '\n')
 {
 output_buffer[output_ptr++] = '\\';
 output_buffer[output_ptr++] = 'n';
 }
 output_buffer[output_ptr++] = '"';
 output_buffer[output_ptr++] = '\n';
 output_buffer[output_ptr++] = 0;
 line_offset = i + 1;
 line_quote_count = 0;
 if (i == length) break;
 }
 }
 return output_buffer;
 }
 */
#endif
