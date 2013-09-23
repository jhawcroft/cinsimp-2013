/*
 
 Application Control Unit: xTalk Thread: Command Implementations
 acu_xt_cmds.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Implementation of xTalk commands specific to the CinsImp environment (the engine provides basics
 inbuilt).
 
 Commands implemented by this translation unit are:
 
 -  find
 -  sort (cards)
 -  visual effect
 -  go
 -  beep
 -  answer, ask, ask password
 -  answer file, ask file, answer folder
 -  open file, close file, read, read line, write
 
 *************************************************************************************************
 
 Planned Future Architectural Improvements
 -------------------------------------------------------------------------------------------------
 -  Refactoring the implemenation of Find to take on more responsibility within the ACU
 -  sort (containers) should probably be implemented within the engine
 -  Global message box functionality, when no stack is open; use a basic xTalk engine instance
    with little or no environmental glue; ACU still to provide this facility
 
 *************************************************************************************************
 */

#include "acu_int.h"



/*
 *  _cmd_find
 *  ---------------------------------------------------------------------------------------------
 *  Implements find command.
 *  Currently decodes and verifies parameters, then passes to UI thread for actual implementation.
 */
static void _cmd_find(XTE *in_engine, StackMgrStack *in_stack, XTEVariant *in_params[], int in_param_count)
{
    if (!xte_variant_convert(in_engine, in_params[0], XTE_TYPE_STRING))
    {
        xte_callback_error(in_engine, NULL, NULL, NULL, NULL);
        return;
    }
    
    StackFindMode mode = FIND_WORDS_BEGINNING;
    if (xte_variant_as_cstring(in_params[1])[0] == 'c') mode = FIND_WORDS_CONTAINING;
    else if (xte_variant_as_cstring(in_params[1])[0] == 'w') mode = FIND_WHOLE_WORDS;
    else if (xte_variant_as_cstring(in_params[1])[0] == 's') mode = FIND_CHAR_PHRASE;
    else if (xte_variant_as_cstring(in_params[1])[0] == 'p') mode = FIND_WORD_PHRASE;
    
    int field = STACK_NO_OBJECT;
    // ***TODO*** field is defaulting to STACK_NO_OBJECT
    
    in_stack->xt_sic.param[0].value.string = xte_variant_as_cstring(in_params[0]);
    in_stack->xt_sic.param[1].value.integer = mode;
    in_stack->xt_sic.param[2].value.integer = field;

    void _acu_ut_find();
    _acu_xt_sic(in_stack, &_acu_ut_find);
}


/*
 *  _cmd_sort
 *  ---------------------------------------------------------------------------------------------
 *  Implements sort (cards) command.
 *  Delegates the task to the Stack unit, after decode of parameters.
 */
static void _cmd_sort(XTE *in_engine, StackMgrStack *in_stack, XTEVariant *in_params[], int in_param_count)
{
    /* decode parameters */
    if (in_param_count < 1)
    {
        xte_callback_error(in_engine, "Sort by what?", NULL, NULL, NULL);
        return;
    }
    StackSortMode mode = STACK_SORT_ASCENDING;
    if ((in_param_count >= 2) && (xte_variant_as_cstring(in_params[1])[0] == 'd'))
        mode = STACK_SORT_DESCENDING;
    
    /* background is specified */
    long bkgnd_id = STACK_NO_OBJECT;
    if (in_param_count == 3)
    {
        /* check it's either "this stack" or a background */
        if (xte_variant_is_class(in_params[2], "stack"))
        {
            if (HDEF(xte_variant_object_data_ptr(in_params[2])).session_id != in_stack->session_id)
            {
                xte_callback_error(in_engine, NULL, NULL, NULL, NULL);
                return;
            }
        }
        else if (!xte_variant_is_class(in_params[2], "bkgnd"))
        {
            xte_callback_error(in_engine, NULL, NULL, NULL, NULL);
            return;
        }
        else
        {
            bkgnd_id = HDEF(xte_variant_object_data_ptr(in_params[2])).layer_id;
        }
    }
    
    /* delegate sort of cards */
    stack_sort(in_stack->stack, in_engine, bkgnd_id, 0, in_params[0], mode);
}


/*
 *  _cmd_beep
 *  ---------------------------------------------------------------------------------------------
 *  Play system beep (optionally a specified number of times.)
 *
 *  ! Directly accesses UI glue.  Assumes no other part of the application will invoke beep/sound
 *    related UI functions, only xTalk.  (?? review)
 */
static void _cmd_beep(XTE *in_engine, StackMgrStack *in_registry_entry, XTEVariant *in_params[], int in_param_count)
{
    int times = 1;
    if (in_params[0]) {
        if (!xte_variant_convert(in_engine, in_params[0], XTE_TYPE_NUMBER))
        {
            xte_callback_error(in_engine, NULL, NULL, NULL, NULL);
            return;
        }
        times = xte_variant_as_int(in_params[0]);
    }
    for (int i = 0; i < times; i++)
    {
        _g_acu.callbacks.do_beep();
        if (xte_callback_still_busy(in_engine) != XTE_OK) return;
    }
}


/*
 *  _cmd_visual
 *  ---------------------------------------------------------------------------------------------
 *  Lodge a visual effect into the queue to be processed at the next unlocking of the screen.
 */
static void _cmd_visual(XTE *in_engine, StackMgrStack *in_stack, XTEVariant *in_params[], int in_param_count)
{
    if ((!in_params[0]) || (!xte_variant_convert(in_stack->xtalk, in_params[0], XTE_TYPE_NUMBER)))
    {
        xte_callback_error(in_engine, NULL, NULL, NULL, NULL);
        return;
    }
    if ((in_params[1]) && (!xte_variant_convert(in_stack->xtalk, in_params[1], XTE_TYPE_NUMBER)))
    {
        xte_callback_error(in_engine, NULL, NULL, NULL, NULL);
        return;
    }
    if ((in_params[2]) && (!xte_variant_convert(in_stack->xtalk, in_params[2], XTE_TYPE_NUMBER)))
    {
        xte_callback_error(in_engine, NULL, NULL, NULL, NULL);
        return;
    }
    
    in_stack->xt_sic.param[0].value.integer = xte_variant_as_int(in_params[0]);
    in_stack->xt_sic.param[1].value.integer = xte_variant_as_int(in_params[1]);
    in_stack->xt_sic.param[2].value.integer = xte_variant_as_int(in_params[2]);
    
    void _acu_ut_visual();
    _acu_xt_sic(in_stack, &_acu_ut_visual);
}



/*
 *  _cmd_go_which
 *  ---------------------------------------------------------------------------------------------
 *  Change the current card according to the specified ordinal, ie. next, previous, first, etc.
 */
static void _cmd_go_which(XTE *in_engine, StackMgrStack *in_stack, XTEVariant *in_params[], int in_param_count)
{
    if ((!in_params[0]) || (!xte_variant_convert(in_stack->xtalk, in_params[0], XTE_TYPE_NUMBER)))
    {
        xte_callback_error(in_engine, NULL, NULL, NULL, NULL);
        return;
    }
    
    in_stack->xt_sic.param[0].value.integer = xte_variant_as_int(in_params[0]);
    in_stack->xt_sic.param[1].value.boolean = xte_variant_as_int(in_params[1]);
    in_stack->xt_sic.param[3].value.boolean = ACU_TRUE;
    
    void _acu_ut_go_relative();
    _acu_xt_sic(in_stack, (ACUImplementor)_acu_ut_go_relative);
    
    if (!in_stack->xt_sic.param[3].value.boolean)
    {
        xte_callback_error(in_engine, "No such \"%s\".", "card", NULL, NULL);// needs localization
        return;
    }
}


/*
 *  _cmd_go_where
 *  ---------------------------------------------------------------------------------------------
 *  Change the current card to that specifically specified.
 */
static void _cmd_go_where(XTE *in_engine, StackMgrStack *in_stack, XTEVariant *in_params[], int in_param_count)
{
    if (!xte_variant_is_class(in_params[0], "card"))
    {
        xte_callback_error(in_engine, NULL, NULL, NULL, NULL);
        return;
    }
    
    in_stack->xt_sic.param[0].value.pointer = xte_variant_object_data_ptr(in_params[0]);
    
    if (stack_card_index_for_id(in_stack->stack, in_stack->xt_sic.param[0].value.hdef->reference.layer_id) == STACK_NO_OBJECT)
    {
        xte_callback_error(in_engine, NULL, NULL, NULL, NULL);
        return;
    }
    
    void _acu_ut_go_absolute();
    _acu_xt_sic(in_stack, (ACUImplementor)_acu_ut_go_absolute);
}



/**************/



static void _cmd_answer_choice(XTE *in_engine, StackMgrStack *in_stack, XTEVariant *in_params[], int in_param_count)
{
    if (!xte_variant_convert(in_stack->xtalk, in_params[0], XTE_TYPE_STRING))
    {
        xte_callback_error(in_engine, NULL, NULL, NULL, NULL);
        return;
    }
    if ((in_params[1]) && (!xte_variant_convert(in_stack->xtalk, in_params[1], XTE_TYPE_STRING)))
    {
        xte_callback_error(in_engine, NULL, NULL, NULL, NULL);
        return;
    }
    if ((in_params[2]) && (!xte_variant_convert(in_stack->xtalk, in_params[2], XTE_TYPE_STRING)))
    {
        xte_callback_error(in_engine, NULL, NULL, NULL, NULL);
        return;
    }
    if ((in_params[3]) && (!xte_variant_convert(in_stack->xtalk, in_params[3], XTE_TYPE_STRING)))
    {
        xte_callback_error(in_engine, NULL, NULL, NULL, NULL);
        return;
    }
    
    in_stack->thread_xte_imp_param[0] = in_params[0];
    in_stack->thread_xte_imp_param[1] = in_params[1];
    in_stack->thread_xte_imp_param[2] = in_params[2];
    in_stack->thread_xte_imp_param[3] = in_params[3];
    
    
    void _acu_ut_answer_choice();
    _acu_xt_sic(in_stack, (ACUImplementor)_acu_ut_answer_choice);
    
    XTEVariant *reply = in_stack->thread_xte_imp_param[0];
    
    //printf("Got reply: %s\n", xte_variant_as_cstring(reply));
    xte_set_global(in_engine, "it", reply);
    
    xte_variant_release(reply);
}


static void _cmd_answer_file(XTE *in_engine, StackMgrStack *in_stack, XTEVariant *in_params[], int in_param_count)
{
    if (!xte_variant_convert(in_stack->xtalk, in_params[0], XTE_TYPE_STRING))
    {
        xte_callback_error(in_engine, NULL, NULL, NULL, NULL);
        return;
    }
    if ((in_params[1]) && (!xte_variant_convert(in_stack->xtalk, in_params[1], XTE_TYPE_STRING)))
    {
        xte_callback_error(in_engine, NULL, NULL, NULL, NULL);
        return;
    }
    if ((in_params[2]) && (!xte_variant_convert(in_stack->xtalk, in_params[2], XTE_TYPE_STRING)))
    {
        xte_callback_error(in_engine, NULL, NULL, NULL, NULL);
        return;
    }
    if ((in_params[3]) && (!xte_variant_convert(in_stack->xtalk, in_params[3], XTE_TYPE_STRING)))
    {
        xte_callback_error(in_engine, NULL, NULL, NULL, NULL);
        return;
    }
    
    in_stack->thread_xte_imp_param[0] = in_params[0];
    in_stack->thread_xte_imp_param[1] = in_params[1];
    in_stack->thread_xte_imp_param[2] = in_params[2];
    in_stack->thread_xte_imp_param[3] = in_params[3];
    
    
    void _acu_ut_answer_file();
    _acu_xt_sic(in_stack, (ACUImplementor)_acu_ut_answer_file);
    
    XTEVariant *reply = in_stack->thread_xte_imp_param[0];
    
    //printf("Got reply: %s\n", xte_variant_as_cstring(reply));
    xte_set_global(in_engine, "it", reply);
    
    xte_variant_release(reply);
}


static void _cmd_answer_folder(XTE *in_engine, StackMgrStack *in_stack, XTEVariant *in_params[], int in_param_count)
{
    if (!xte_variant_convert(in_stack->xtalk, in_params[0], XTE_TYPE_STRING))
    {
        xte_callback_error(in_engine, NULL, NULL, NULL, NULL);
        return;
    }
    
    in_stack->thread_xte_imp_param[0] = in_params[0];
    
    
    void _acu_ut_answer_folder();
    _acu_xt_sic(in_stack, (ACUImplementor)_acu_ut_answer_folder);
    
    XTEVariant *reply = in_stack->thread_xte_imp_param[0];
    
    //printf("Got reply: %s\n", xte_variant_as_cstring(reply));
    xte_set_global(in_engine, "it", reply);
    
    xte_variant_release(reply);
}


static void _cmd_ask_text(XTE *in_engine, StackMgrStack *in_stack, XTEVariant *in_params[], int in_param_count)
{
    if (!xte_variant_convert(in_stack->xtalk, in_params[0], XTE_TYPE_STRING))
    {
        xte_callback_error(in_engine, NULL, NULL, NULL, NULL);
        return;
    }
    if ((in_params[1]) && (!xte_variant_convert(in_stack->xtalk, in_params[1], XTE_TYPE_STRING)))
    {
        xte_callback_error(in_engine, NULL, NULL, NULL, NULL);
        return;
    }
    
    in_stack->thread_xte_imp_param[0] = in_params[0];
    in_stack->thread_xte_imp_param[1] = in_params[1];
    in_stack->thread_xte_imp_param[2] = in_params[2];
    
    void _acu_ut_ask_text();
    _acu_xt_sic(in_stack, (ACUImplementor)_acu_ut_ask_text);
    
    XTEVariant *reply = in_stack->thread_xte_imp_param[0];
    if (!reply) reply = xte_string_create_with_cstring(in_engine, "");
    
    //printf("Got reply: %s\n", xte_variant_as_cstring(reply));
    xte_set_global(in_engine, "it", reply);
    
    xte_variant_release(reply);
}



static void _cmd_ask_file(XTE *in_engine, StackMgrStack *in_stack, XTEVariant *in_params[], int in_param_count)
{
    if (!xte_variant_convert(in_stack->xtalk, in_params[0], XTE_TYPE_STRING))
    {
        xte_callback_error(in_engine, NULL, NULL, NULL, NULL);
        return;
    }
    if ((in_params[1]) && (!xte_variant_convert(in_stack->xtalk, in_params[1], XTE_TYPE_STRING)))
    {
        xte_callback_error(in_engine, NULL, NULL, NULL, NULL);
        return;
    }
    
    in_stack->thread_xte_imp_param[0] = in_params[0];
    in_stack->thread_xte_imp_param[1] = in_params[1];
    
    
    void _acu_ut_ask_file();
    _acu_xt_sic(in_stack, (ACUImplementor)_acu_ut_ask_file);
    
    XTEVariant *reply = in_stack->thread_xte_imp_param[0];
    if (!reply) reply = xte_string_create_with_cstring(in_engine, "");
    
    //printf("Got reply: %s\n", xte_variant_as_cstring(reply));
    xte_set_global(in_engine, "it", reply);
    
    xte_variant_release(reply);
}







static void _cmd_file_open(XTE *in_engine, StackMgrStack *in_stack, XTEVariant *in_params[], int in_param_count)
{
    if (!xte_variant_convert(in_stack->xtalk, in_params[0], XTE_TYPE_STRING))
    {
        xte_callback_error(in_engine, NULL, NULL, NULL, NULL);
        return;
    }
    _acu_xt_fio_open(in_stack, xte_variant_as_cstring(in_params[0]));
}


static void _cmd_file_close(XTE *in_engine, StackMgrStack *in_stack, XTEVariant *in_params[], int in_param_count)
{
    if (!xte_variant_convert(in_stack->xtalk, in_params[0], XTE_TYPE_STRING))
    {
        xte_callback_error(in_engine, NULL, NULL, NULL, NULL);
        return;
    }
    _acu_xt_fio_close(in_stack, xte_variant_as_cstring(in_params[0]));
}


// begin,end,count
static void _cmd_file_read_chars(XTE *in_engine, StackMgrStack *in_stack, XTEVariant *in_params[], int in_param_count)
{
    if (!xte_variant_convert(in_stack->xtalk, in_params[0], XTE_TYPE_STRING))
    {
        xte_callback_error(in_engine, NULL, NULL, NULL, NULL);
        return;
    }
    if (in_params[1] && (!xte_variant_convert(in_stack->xtalk, in_params[1], XTE_TYPE_INTEGER)))
    {
        xte_callback_error(in_engine, NULL, NULL, NULL, NULL);
        return;
    }
    if (in_params[2] && (!xte_variant_convert(in_stack->xtalk, in_params[2], XTE_TYPE_STRING)))
    {
        xte_callback_error(in_engine, NULL, NULL, NULL, NULL);
        return;
    }
    if (in_params[1] && (!xte_variant_convert(in_stack->xtalk, in_params[3], XTE_TYPE_INTEGER)))
    {
        xte_callback_error(in_engine, NULL, NULL, NULL, NULL);
        return;
    }
    
    long charBegin, charCount;
    long *pBegin = NULL, *pCount = NULL;
    char *pEnd = NULL;
    
    if (in_params[1])
    {
        charBegin = xte_variant_as_int(in_params[1]);
        pBegin = &charBegin;
    }
    if (in_params[2])
    {
        pEnd = (char*)xte_variant_as_cstring(in_params[2]);
    }
    if (in_params[3])
    {
        charCount = xte_variant_as_int(in_params[3]);
        pCount = &charCount;
    }
    
    _acu_xt_fio_read(in_stack, xte_variant_as_cstring(in_params[0]), pBegin, pEnd, pCount);
}


static void _cmd_file_read_line(XTE *in_engine, StackMgrStack *in_stack, XTEVariant *in_params[], int in_param_count)
{
    if (!xte_variant_convert(in_stack->xtalk, in_params[0], XTE_TYPE_STRING))
    {
        xte_callback_error(in_engine, NULL, NULL, NULL, NULL);
        return;
    }
    _acu_xt_fio_read_line(in_stack, xte_variant_as_cstring(in_params[0]));
}


// filepath, text, charBegin
static void _cmd_file_write_chars(XTE *in_engine, StackMgrStack *in_stack, XTEVariant *in_params[], int in_param_count)
{
    if (!xte_variant_convert(in_stack->xtalk, in_params[0], XTE_TYPE_STRING))
    {
        xte_callback_error(in_engine, NULL, NULL, NULL, NULL);
        return;
    }
    if (!xte_variant_convert(in_stack->xtalk, in_params[1], XTE_TYPE_STRING))
    {
        xte_callback_error(in_engine, NULL, NULL, NULL, NULL);
        return;
    }
    if (in_params[2] && (!xte_variant_convert(in_stack->xtalk, in_params[2], XTE_TYPE_INTEGER)))
    {
        xte_callback_error(in_engine, NULL, NULL, NULL, NULL);
        return;
    }
    
    long charBegin;
    long *pBegin = NULL;
    
    if (in_params[2])
    {
        charBegin = xte_variant_as_int(in_params[2]);
        pBegin = &charBegin;
    }
    
    _acu_xt_fio_write(in_stack, xte_variant_as_cstring(in_params[0]), xte_variant_as_cstring(in_params[1]), pBegin);
}





struct XTECommandDef _acu_xt_commands[] = {
    {"beep [<times> [times]]","times", (XTECommandImp)&_cmd_beep},
    
    {"find [`mode``b`normal|`c`chars|`c`characters|`w`word|`w`words|`s`string|`p`whole] "
        "<text> [in <field>]","text,mode,field", (XTECommandImp)&_cmd_find},
    
    {"sort [[the] cards] of <bkgnd> [`dir``asc`ascending|`des`descending] by <sortKey>",
        "sortKey<,dir,bkgnd", (XTECommandImp)&_cmd_sort},
    {"sort [[the] cards] [`dir``asc`ascending|`des`descending] by <sortKey>",
        "sortKey<,dir", (XTECommandImp)&_cmd_sort},
    {"sort <bkgnd> [`dir``asc`ascending|`des`descending] by <sortKey>",
        "sortKey<,dir,bkgnd", (XTECommandImp)&_cmd_sort},
    
    {"visual [effect] {`effect``0`cut|`1`dissolve|`2`wipe left|`3`wipe right|`4`wipe up|`5`wipe down} "
        "[`speed``-2`very slowly|`-2`very slow|`-1`slowly|`-1`slow|`0`normal|`2`very fast|`1`fast] "
        "[to {`dest``0`card|`1`black|`2`white|`3`grey|`3`gray}]",
        "effect,speed,dest",
        (XTECommandImp)&_cmd_visual},
    
    /* must use this to capture cases where [card] is missing on the end, and prev & next which aren't
     handled via ordinal referencing */
    //"go [to] {`which``-2`next|`-1`prev|`-1`previous|`1`first|`2`second|`3`third|`4`forth|`5`fifth|"
    //"`6`sixth|`7`seventh|`8`eighth|`9`ninth|`10`tenth|`-3`middle|`-4`any|`-5`last} [[`marked``1`marked] card]
    // "ANY" is missing from this syntax, because in 1.0 we can't be bothered with the machinery that
    // would be require to make this work; for starters, the go command would have to be implemented
    // more on the thread side of things, to make use of the xtalk engine to resolve the random() need
    // all this really means is that you must say go any card, rather than just saying go any.
    //"
    {"go [to] {`which``-2`next|`-1`prev|`-1`previous|`1`first|`2`second|`3`third|`4`forth|`5`fifth|"
        "`6`sixth|`7`seventh|`8`eighth|`9`ninth|`10`tenth|`-3`middle|`-5`last} [[`marked``1`marked] card]",
        "which,marked",
        (XTECommandImp)&_cmd_go_which}, // this definition is causing a leak ***
    {"go [to] <where>", "where", (XTECommandImp)&_cmd_go_where},
    
    {"answer file <prompt> [of type <type1> [or <type2> [or <type3>]]]", "prompt,type1,type2,type3", (XTECommandImp)&_cmd_answer_file},
    {"answer folder <prompt>", "prompt", (XTECommandImp)&_cmd_answer_folder},
    {"answer <prompt> [with <btn1> [or <btn2> [or <btn3>]]]", "prompt,btn1,btn2,btn3", (XTECommandImp)&_cmd_answer_choice},
    
    {"ask file <prompt> [with <defaultFilename>]", "prompt,defaultFilename", (XTECommandImp)&_cmd_ask_file},
    {"ask [`password``1`password] <prompt> [with <response>]", "prompt,response,password", (XTECommandImp)&_cmd_ask_text},
    
    {"open file <filepath>", "filepath", (XTECommandImp)&_cmd_file_open},
    {"close file <filepath>", "filepath", (XTECommandImp)&_cmd_file_close},
    {"read line from file <filepath>", "filepath", (XTECommandImp)&_cmd_file_read_line},
    {"read from file <filepath> [at <charBegin>] {for <charCount> | until {end|eof|<charEnd>}}", "filepath,charBegin,charEnd,charCount", (XTECommandImp)&_cmd_file_read_chars},
    {"write <text> to file <filepath> [at {end|eof|<charBegin>}]", "filepath,text,charBegin", (XTECommandImp)&_cmd_file_write_chars},
    
    NULL,
};





