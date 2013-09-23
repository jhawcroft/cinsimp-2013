/*
 
 xTalk Engine Synonym Parsing Unit
 xtalk_syn.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Responsible for parsing synonyms
 
 *************************************************************************************************
 */

#include "xtalk_internal.h"


/* 
 synonynm replacement will store the original word phrase and replacement length
 as a child LIST of the first node in the phrase;
 replacement nodes will be flagged to prevent further recursion.
 
 later the original phrases could be restored prior to further parsing;
 assuming those phrases were not understood by some other parsing routine.
 
 eg. "put 3 into cd"
 would currently be expanded to "put 3 into card",
 which is fine except that "card" might eventually be an allowable variable
 name when not used within a valid reference expression!
 */


/*********
 Parsing
 */

/* expands a specific synonym within the expression */
static void _xte_convert_synonym(XTE *in_engine, XTEAST *in_stmt, int in_offset, struct XTESynonymInt *in_syn)
{
    /* before we begin the expansion, make sure it's not already in an expanded (multi-word) form,
     eg. "message" -> "message box", so we don't end up with: "message box box" */
    int i = in_offset, w;
    for (w = 0; w < in_syn->rep_word_count; w++)
    {
        XTEAST *word = _xte_ast_list_child(in_stmt, i++);
        if (!word) break;
        if ( (word->type != XTE_AST_WORD) || (word->flags & XTE_AST_FLAG_SYNREP) ||
            (_xte_compare_cstr(in_syn->rep_words[w], word->value.string) != 0) ) break;
    }
    if (w == in_syn->rep_word_count) return;
    
    /* create a list to hold the current source terminology */
    XTEAST *list = _xte_ast_create(in_engine, XTE_AST_LIST);
    assert(list != NULL);
    
    /* move the existing terminology to the list */
    for (int i = 0; i < in_syn->word_count; i++)
        _xte_ast_list_append(list, _xte_ast_list_remove(in_stmt, in_offset));

    /* insert the synonym replacement words */
    for (int i = 0; i < in_syn->rep_word_count; i++)
    {
        _xte_ast_list_insert(in_stmt, in_offset + i, _xte_ast_create_word(in_engine, in_syn->rep_words[i]));
        in_stmt->children[in_offset + i]->flags |= XTE_AST_FLAG_SYNREP;
    }
    
    /* add the list of the source terminology to the first replacement */
    _xte_ast_list_append(in_stmt->children[in_offset], list);
}


/*
 *  _xte_parse_convert_synonyms
 *  ---------------------------------------------------------------------------------------------
 *  Looks for and expands known synonyms, according to the terminology dictionary.
 *
 *  ! Mutates the input token stream.
 */
void _xte_parse_convert_synonyms(XTE *in_engine, XTEAST *in_stmt)
{
    assert(in_engine != NULL);
    assert(in_stmt != NULL);
    
    /* iterate backwards through statement words */
    for (int i = in_stmt->children_count - 1; i >= 0; i--)
    {
        /* look for words that are not the result of a previous synonym expansion */
        XTEAST *child = in_stmt->children[i];
        if ( (child->type == XTE_AST_WORD) && (!(child->flags & XTE_AST_FLAG_SYNREP)) )
        {
            /* iterate through synonyms */
            for (int j = in_engine->syns_count-1; j >= 0; j--)
            {
                /* compare synonym against statement */
                struct XTESynonymInt *syn = &(in_engine->syns[j]);
                if (i + syn->word_count >= in_stmt->children_count + 1) continue;
                
                int w;
                for (w = 0; w < syn->word_count; w++)
                {
                    XTEAST *word = in_stmt->children[i + w];
                    if ((word->type != XTE_AST_WORD) || (word->flags & XTE_AST_FLAG_SYNREP) ||
                        (_xte_compare_cstr(syn->words[w], word->value.string) != 0))
                        break;
                }
                if (syn->word_count == w)
                {
                    /* found matching synonym; convert it */
                    _xte_convert_synonym(in_engine, in_stmt, i, syn);
                    break;
                }
            }
        }
    }
}


/* reverse synonym expansion; not currently implemented,
 might be implemented later (see notes at top of file) */
void _xte_parse_reverse_synonyms(XTE *in_engine, XTEAST *in_stmt)
{
    return _xte_panic_void(in_engine, XTE_ERROR_UNIMPLEMENTED, NULL);
}



/*********
 Dictionary
 */


/* adds a single synonym to the internal synyonym dictionary;
 ** TODO ** to be done correctly, synonyms should actually be listed from longest to shortest 
 (on word count), but we can do this later as I don't think it'll cause much harm here.
 */
static void _xte_synonym_add(XTE *in_engine, struct XTESynonymDef *in_def)
{
    /* add an entry to the synonym table */
    struct XTESynonymInt *new_table = realloc(in_engine->syns, sizeof(struct XTESynonymInt) * (in_engine->syns_count + 1));
    if (!new_table) return _xte_panic_void(in_engine, XTE_ERROR_MEMORY, NULL);
    in_engine->syns = new_table;
    struct XTESynonymInt *new_synonym = &(new_table[in_engine->syns_count++]);
    
    /* configure the synonym */
    new_synonym->name = _xte_clone_cstr(in_engine, in_def->name);
    _xte_itemize_cstr(in_engine, in_def->synonym, " ", &(new_synonym->words), &(new_synonym->word_count));
    _xte_itemize_cstr(in_engine, in_def->name, " ", &(new_synonym->rep_words), &(new_synonym->rep_word_count));
}


/* add one or more synonyms with one call */
void _xte_synonyms_add(XTE *in_engine, struct XTESynonymDef *in_defs)
{
    if (!in_defs) return;
    struct XTESynonymDef *the_syn = in_defs;
    while (the_syn->name)
    {
        _xte_synonym_add(in_engine, the_syn);
        the_syn++;
    }
}



/*********
 Built-ins
 */

static struct XTESynonymDef bi_syns[] = {
    {"char", "character"},
    {"chars", "characters"},
    NULL,
};

/* add built-in language synonyms */
void _xte_synonyms_add_builtins(XTE *in_engine)
{
    _xte_synonyms_add(in_engine, bi_syns);
}



