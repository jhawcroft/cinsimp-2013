/*
 
 Stack File Format
 stack_find.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Find text in stack functionality
 
 (see header file for module description)
 
 */

#include "stack_int.h"
#include "xtalk_engine.h"



/********
 Startup
 */


static int _is_card_searchable(Stack *in_stack, long in_card_id)
{
    // be sure to check background settings too
    int searchable = 1;
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(in_stack->db, "SELECT card.dontsearch, bkgnd.dontsearch "
                       "FROM card JOIN bkgnd ON card.bkgndid=bkgnd.bkgndid WHERE card.cardid=?1", -1,
                       &stmt, NULL);
    sqlite3_bind_int(stmt, 1, (int)in_card_id);
    sqlite3_step(stmt);
    if (sqlite3_column_int(stmt, 0) || sqlite3_column_int(stmt, 1))
        searchable = 0;
    sqlite3_finalize(stmt);
    return searchable;
}


static int _is_field_searchable(Stack *in_stack, long in_field_id)
{
    int searchable = 1;
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(in_stack->db, "SELECT dontsearch,type FROM widget WHERE widgetid=?1", -1,
                       &stmt, NULL);
    sqlite3_bind_int(stmt, 1, (int)in_field_id);
    sqlite3_step(stmt);
    if (sqlite3_column_int(stmt, 0)) searchable = 0;
    switch (sqlite3_column_int(stmt, 1))
    {
        case WIDGET_BUTTON_PUSH:
        case WIDGET_BUTTON_TRANSPARENT:
            searchable = 0;
            break;
        default:
            break;
    }
    sqlite3_finalize(stmt);
    return searchable;
}


static void _get_searchable_fields(Stack *in_stack, long in_card_id, long in_field_id, long **out_fields, int *out_count)
{
    *out_count = 0;
    
    // if field_id is set, we check only that field and return a list of 1
    if (in_field_id != 0)
    {
        if (_is_field_searchable(in_stack, in_field_id))
        {
            *out_fields = _stack_malloc(sizeof(long));
            if (!*out_fields) return app_out_of_memory_void();
            (*out_fields)[0] = in_field_id;
        }
        else
        {
            *out_fields = NULL;
        }
        return;
    }
    
    // obtain the sequence of fields on the card
    IDTable * bkgnd_table = _stack_widget_seq_get(in_stack, 0, _cards_bkgnd(in_stack, in_card_id));
    IDTable *card_table = _stack_widget_seq_get(in_stack, in_card_id, 0);
    
    // create an array adequate to hold the maximum number of fields
    *out_fields = _stack_malloc( sizeof(long) * (idtable_size(card_table) + idtable_size(bkgnd_table)) );
    if (!*out_fields) return app_out_of_memory_void();
    
    // check background fields are searchable
    for (int i = 0; i < idtable_size(bkgnd_table); i++)
    {
        long field_id = idtable_id_for_index(bkgnd_table, i);
        if (_is_field_searchable(in_stack, field_id))
            (*out_fields)[(*out_count)++] = field_id;
    }
    
    // check card fields are searchable
    for (int i = 0; i < idtable_size(card_table); i++)
    {
        long field_id = idtable_id_for_index(card_table, i);
        if (_is_field_searchable(in_stack, field_id))
            (*out_fields)[(*out_count)++] = field_id;
    }
}


static char* _field_content(Stack *in_stack, long in_field_id, long in_card_id)
{
    char *searchable;
    stack_widget_content_get(in_stack, in_field_id, in_card_id, 0, &searchable, NULL, NULL, NULL);
    return _stack_clone_cstr(searchable);
}


////

static void _find_get_next_card(Stack *in_stack)
{
   
   
    
    //find_stop_card if set =1 we exit here
    if (in_stack->find_stop_card)
    {
        in_stack->find_state = _FIND_FINISH;
        return;
    }
    
    // get next card in sequence (in_stack->find_state_card_id)
    IDTable *table = NULL;
    if (!in_stack->find_marked)
        table = in_stack->stack_card_table;
    else
        ; // marked cards will be implemented in a future version
    long seq = idtable_index_for_id(table, in_stack->find_state_card_id) + 1;
    if (seq >= idtable_size(table)) seq = 0;
    in_stack->find_state_card_id = idtable_id_for_index(table, seq);
    
    // check for end
    if (in_stack->find_state_card_id == in_stack->find_card_id)
    {
        if (in_stack->found_match_this_search)
        {
            in_stack->found_match_this_search = 0;
        }
        else
        {
            in_stack->find_state = _FIND_FINISH;
            return;
        }
    }
    
    // need to check find_restrict_bkgnd
    if (in_stack->find_restrict_bkgnd)
    {
        // this card must belong to this bkgnd, otherwise we exit and try again
        if (_cards_bkgnd(in_stack, in_stack->find_state_card_id) != in_stack->find_restrict_bkgnd)
            return;
    }
    
    // is card searchable?
    if (!_is_card_searchable(in_stack, in_stack->find_state_card_id))
        return;
    
    
    
    
    in_stack->find_state = _FIND_CARD_SETUP;
}


static void _find_card_setup(Stack *in_stack)
{
    // reset search fields
    in_stack->find_state_field_count = 0;
    if (in_stack->find_state_fields)
        _stack_free(in_stack->find_state_fields);
    
    // get list of fields for card (in_stack->find_state_card_id)
    _get_searchable_fields(in_stack, in_stack->find_state_card_id, in_stack->find_field_id,
                           &(in_stack->find_state_fields), &(in_stack->find_state_field_count));
    in_stack->find_state_field_index = 0;
    in_stack->find_state_field_offset = 0;
    in_stack->find_state_field_length = 0;
    
    
        
    if (in_stack->find_state_field_count == 0)
    {
        in_stack->find_state = _FIND_GET_NEXT_CARD;
        return;
    }

    // grab the content of the first field here - cache in stack
    if (in_stack->find_state_field_content) _stack_free(in_stack->find_state_field_content);
    in_stack->find_state_field_content = _field_content(in_stack, in_stack->find_state_fields[0], in_stack->find_state_card_id);
    
    
    
    
    // do the split of the first field here and save to stack context
    if ((in_stack->find_mode == FIND_WHOLE_WORDS) ||
        (in_stack->find_mode == FIND_WORDS_BEGINNING) ||
        (in_stack->find_mode == FIND_WORDS_CONTAINING) ||
        (in_stack->find_mode == FIND_WORD_PHRASE))
    {
        if (in_stack->find_state_field_words)
            _stack_items_free(in_stack->find_state_field_words, in_stack->find_state_field_word_count);
        if (in_stack->find_state_field_offsets)
            _stack_free(in_stack->find_state_field_offsets);
        _stack_wordize(in_stack->find_state_field_content,
                       &(in_stack->find_state_field_words),
                       &(in_stack->find_state_field_offsets),
                       &(in_stack->find_state_field_word_count));
        in_stack->find_state_field_word_index = 0;
    }
    else if (in_stack->find_mode == FIND_CHAR_PHRASE)
    {
        in_stack->find_state_field_offset = 0;
        in_stack->find_state_field_length = strlen(in_stack->find_state_field_content);
    }
    
    
   
    
    
    if (in_stack->find_mode == FIND_WORD_PHRASE)
        in_stack->find_state = _FIND_WORD_PHRASE;
    else if (in_stack->find_mode == FIND_CHAR_PHRASE)
        in_stack->find_state = _FIND_CHAR_PHRASE;
    else
        in_stack->find_state = _FIND_WORDS_FIRST;
}




/********
 Noncontiguous Word Matching
 */

/* inplace, strip of prefix and postfix punctuation */
void _stack_word_strip_prepost_punct(char *io_word, int *out_prefix_count, int *out_postfix_count)
{
    assert(io_word != NULL);
    
    char *end = io_word;
    while (*end) end++; /* iterate until we find the end of the word */
    
    char *ptr = io_word;
    while ( *ptr && ((*ptr < 0x80) && ispunct(*ptr)) )
    {
        ptr++; /* iterate until we reach non-punctuation */
    }
    if (out_prefix_count)
        *out_prefix_count = (int)(ptr - io_word);
    memmove(io_word, ptr, end - ptr + 1);
    
    end = io_word + (end - ptr);
    if (io_word == end) return;
    
    ptr = end;
    do
    {
        ptr--; /* iterate until we reach non-punctuation */
    } while ( (ptr != io_word) && ((*ptr < 0x80) && ispunct(*ptr)) );
    *(++ptr) = 0;
    if (out_postfix_count)
        *out_postfix_count = (int)(end - ptr);
}


/* specific matching for words;
 later we can make this split both words into 3-components: prefix, main, suffix,
 and consider main first, then match as much as possible of prefix & suffix (or none at all).
 it'll have to return the actual offset & length of the match.
 
 **TODO** these functions have a bug in that the offsets they output are in bytes,
 we'll almost certainly need character offsets.
 
 Not a high priority, as long as the algorithm works, we can catch it before 1.0
 and once this file is a lot neater - also punctuation is less likely to cause an incorrect offset
 as it's generally 'ASCII' anyway...
 
 */
static int _find_words_equal(char const *in_search_word, char const *in_content_word, long *out_match_offset, long *out_match_bytes)
{
    char *search_word, *content_word;
    int prefix_punc_bytes, postfix_punc_bytes;
    int result;
    
    /* get versions of words without punctuation */
    search_word = _stack_clone_cstr((char*)in_search_word);
    _stack_word_strip_prepost_punct(search_word, NULL, NULL);
    content_word = _stack_clone_cstr((char*)in_content_word);
    _stack_word_strip_prepost_punct(content_word, &prefix_punc_bytes, &postfix_punc_bytes);
    
    /* run initial case-insensitive comparison */
    result = xte_cstrings_equal(search_word, content_word);
    
    
    /* attempt to match prefix and postfix punctuation;
     **TODO*** LOW priority *****/
    
    
    /* calculate offsets */
    if ((result) && (out_match_offset))
    {
        *out_match_offset = prefix_punc_bytes;
        *out_match_bytes = strlen(search_word);
    }
    
    /* cleanup */
    _stack_free(search_word);
    _stack_free(content_word);
    
    return result;
}


/* similar to above; more notes needed in this comment .... 
 does a begins search */
static int _find_words_beginning(char const *in_search_word, char const *in_content_word, long *out_match_offset, long *out_match_bytes)
{
    char *search_word, *content_word;
    int prefix_punc_bytes;
    int result;
    
    /* get versions of words without punctuation */
    search_word = _stack_clone_cstr((char*)in_search_word);
    _stack_word_strip_prepost_punct(search_word, NULL, NULL);
    content_word = _stack_clone_cstr((char*)in_content_word);
    _stack_word_strip_prepost_punct(content_word, &prefix_punc_bytes, NULL);
    
    /* run initial case-insensitive comparison */
    long search_bytes, content_bytes;
    search_bytes = strlen(search_word);
    content_bytes = strlen(content_word);
    if (search_bytes > content_bytes) result = 0;
    else
    {
        content_word[search_bytes] = 0; /* trim the words to the same length */
        result = xte_cstrings_equal(search_word, content_word);
    }
    
    
    /* attempt to match prefix punctuation (no postfix);
     **TODO*** LOW priority *****/
    
    
    /* calculate offsets */
    if ((result) && (out_match_offset))
    {
        *out_match_offset = prefix_punc_bytes;
        *out_match_bytes = strlen(search_word);
    }
    
    /* cleanup */
    _stack_free(search_word);
    _stack_free(content_word);
    
    return result;
}



static int _find_word_containing(char const *in_search_word, char const *in_content_word, long *out_match_offset, long *out_match_bytes)
{
    int result = 0;
    
    /* run contains match */
    long search_bytes, content_bytes;
    long offset_start = 0;
    search_bytes = strlen(in_search_word);
    content_bytes = strlen(in_content_word);
    if (search_bytes > content_bytes) result = 0;
    else
    {
        while (offset_start + search_bytes <= content_bytes)
        {
            if (xte_cstrings_szd_equal(in_search_word, search_bytes, in_content_word + offset_start, search_bytes))
            {
                result = 1;
                break;
            }
            offset_start++;
        }
    }
    
    /* calculate offsets */
    if ((result) && (out_match_offset))
    {
        *out_match_offset = offset_start;
        *out_match_bytes = strlen(in_search_word);
    }
    
    return result;
}



static void _find_words_first(Stack *in_stack)
{
    // find the first word of the search words in any of the card's fields
    int matched_word = 0;
    while (in_stack->find_state_field_index < in_stack->find_state_field_count)
    {
        // continue search of field here
        for (int word_index = in_stack->find_state_field_word_index;
             word_index < in_stack->find_state_field_word_count;
             word_index++)
        {
            long match_offset, match_bytes;
            int is_match = 0;
            switch (in_stack->find_mode)
            {
                case FIND_WORDS_BEGINNING:
                    is_match = _find_words_beginning(in_stack->find_words[0],
                                                     in_stack->find_state_field_words[word_index],
                                                     &match_offset,
                                                     &match_bytes);
                    break;
                case FIND_WORDS_CONTAINING:
                    is_match = _find_word_containing(in_stack->find_words[0],
                                                     in_stack->find_state_field_words[word_index],
                                                     &match_offset,
                                                     &match_bytes);
                    break;
                case FIND_WHOLE_WORDS:
                    is_match = _find_words_equal(in_stack->find_words[0],
                                                 in_stack->find_state_field_words[word_index],
                                                 &match_offset,
                                                 &match_bytes);
                    break;
                default:
                    assert(0);
            }
            if (is_match)
            {
                // found a matching word
                matched_word = 1;
                in_stack->find_state_field_word_index = word_index + 1;
                
                // store match information in case we match soon
                in_stack->found_offset = in_stack->find_state_field_offsets[word_index] + match_offset;
                in_stack->found_length = match_bytes;//strlen(in_stack->find_words[0]);
                in_stack->found_field_id = in_stack->find_state_fields[in_stack->find_state_field_index];
                in_stack->found_card_id = in_stack->find_state_card_id;
                in_stack->found_text = _stack_clone_cstr(in_stack->find_state_field_words[word_index]);
                break;
            }
        }
        if (matched_word) break;
        
        
        
        // look at next field
        in_stack->find_state_field_index++;
        if (in_stack->find_state_field_index < in_stack->find_state_field_count)
        {
            // reset some state variables
            in_stack->find_state_field_offset = 0;
            in_stack->find_state_field_length = 0;
            
            // grab the content of the next field here
            if (in_stack->find_state_field_content) _stack_free(in_stack->find_state_field_content);
            in_stack->find_state_field_content = _field_content(in_stack,
                                                                in_stack->find_state_fields[in_stack->find_state_field_index],
                                                                in_stack->find_state_card_id);
            
            
            
            // split the next field and save to stack contents here
            if (in_stack->find_state_field_words)
                _stack_items_free(in_stack->find_state_field_words, in_stack->find_state_field_word_count);
            if (in_stack->find_state_field_offsets)
                _stack_free(in_stack->find_state_field_offsets);
            _stack_wordize(in_stack->find_state_field_content,
                           &(in_stack->find_state_field_words),
                           &(in_stack->find_state_field_offsets),
                           &(in_stack->find_state_field_word_count));
            
            
            
        }
    }
    
    if (matched_word) in_stack->find_state = _FIND_WORDS_OTHER;
    else in_stack->find_state = _FIND_GET_NEXT_CARD;
}


static void _find_words_other(Stack *in_stack)
{
    
    
    
    int matched_word;
    // check the other words 1..N all match to any field
    // but don't care where or which field
    int matching_other_words = 0;
    for (int search_word_index = 1;
         search_word_index < in_stack->find_word_count;
         search_word_index++)
    {
        char *search_word = in_stack->find_words[search_word_index];
        
        matched_word = 0;
        for (int field_index = 0;
             field_index < in_stack->find_state_field_count;
             field_index++)
        {
            // get field content and split into words
            char *field_content = _field_content(in_stack, in_stack->find_state_fields[field_index], in_stack->find_state_card_id);
            char **field_words;
            long *field_offsets;
            int field_word_count;
            _stack_wordize(field_content,
                           &field_words,
                           &field_offsets,
                           &field_word_count);
            _stack_free(field_content);
            
            // find a matching word in the field
            for (int field_word_index = 0;
                 field_word_index < field_word_count;
                 field_word_index++)
            {
                int is_match = 0;
                switch (in_stack->find_mode)
                {
                    case FIND_WORDS_BEGINNING:
                        is_match = _find_words_beginning(search_word, field_words[field_word_index], NULL, NULL);
                        break;
                    case FIND_WORDS_CONTAINING:
                        is_match = _find_word_containing(search_word, field_words[field_word_index], NULL, NULL);
                        break;
                    case FIND_WHOLE_WORDS:
                        is_match = _find_words_equal(search_word, field_words[field_word_index], NULL, NULL);
                        break;
                    default:
                        assert(0);
                }
                if (is_match)
                {
                    // found a matching word
                    matched_word = 1;
                    break;
                }
            }
            
            /* cleanup */
            if (field_words)
                _stack_items_free(field_words, field_word_count);
            if (field_offsets)
                _stack_free(field_offsets);
            
            if (matched_word) break;
        }
        
        if (matched_word) matching_other_words++;
    }
    if (matching_other_words == in_stack->find_word_count-1)
    {
        // found a match!
        //match_found = 1;
        in_stack->found_match_this_search = 1;
        in_stack->matches_this_search++;
        
        // ??? maybe... nothing else...
        
        in_stack->find_state = _FIND_FINISH;
    }
    else
    {
        in_stack->find_state = _FIND_GET_NEXT_CARD;
    }
}



/********
 Word Phrase Matching
 */

static void _find_word_phrase(Stack *in_stack)
{
    int matched_word = 0;
    while (in_stack->find_state_field_index < in_stack->find_state_field_count)
    {
        // continue search of field here
        for (int word_index = in_stack->find_state_field_word_index;
             word_index < in_stack->find_state_field_word_count;
             word_index++)
        {
            /* attempt to match words of search terms with consecutive content words at this location */
            long matching_words = 0, match_offset, match_chars;
            for (int find_word_index = 0;
                 (find_word_index < in_stack->find_word_count) &&
                 (word_index + find_word_index < in_stack->find_state_field_word_count);
                 find_word_index++)
            {
                long offset, chars;
                if (!_find_words_equal(in_stack->find_words[find_word_index],
                                       in_stack->find_state_field_words[word_index + find_word_index],
                                       &offset,
                                       &chars)) break;
                if (find_word_index == 0)
                    match_offset = offset;
                if (find_word_index + 1 == in_stack->find_word_count)
                    match_chars = in_stack->find_state_field_offsets[word_index + find_word_index] -
                    in_stack->find_state_field_offsets[word_index] + _xte_utf8_strlen(in_stack->find_state_field_words[word_index + find_word_index]) - match_offset;
                matching_words++;
            }
            
            /* if all search words matched, we have a match! */
            if (matching_words == in_stack->find_word_count)
            {
                // found a matching word
                matched_word = 1;
                in_stack->find_state_field_word_index = word_index + 1; /* so we continue from the next word next search time... */
                
                // store match information in case we match soon
                in_stack->found_offset = in_stack->find_state_field_offsets[word_index] + match_offset;
                in_stack->found_length = match_chars;
                in_stack->found_field_id = in_stack->find_state_fields[in_stack->find_state_field_index];
                in_stack->found_card_id = in_stack->find_state_card_id;
                in_stack->found_text = _stack_clone_cstr(in_stack->find_state_field_words[word_index]);
                break;
            }
        }
        if (matched_word) break;
        
        // look at next field
        in_stack->find_state_field_index++;
        if (in_stack->find_state_field_index < in_stack->find_state_field_count)
        {
            // reset some state variables
            in_stack->find_state_field_offset = 0;
            in_stack->find_state_field_length = 0;
            
            // grab the content of the next field here
            if (in_stack->find_state_field_content) _stack_free(in_stack->find_state_field_content);
            in_stack->find_state_field_content = _field_content(in_stack,
                                                                in_stack->find_state_fields[in_stack->find_state_field_index],
                                                                in_stack->find_state_card_id);
            
            // split the next field and save to stack contents here
            if (in_stack->find_state_field_words)
                _stack_items_free(in_stack->find_state_field_words, in_stack->find_state_field_word_count);
            if (in_stack->find_state_field_offsets)
                _stack_free(in_stack->find_state_field_offsets);
            _stack_wordize(in_stack->find_state_field_content,
                           &(in_stack->find_state_field_words),
                           &(in_stack->find_state_field_offsets),
                           &(in_stack->find_state_field_word_count));
        }
    }
    
    if (!matched_word)
        in_stack->find_state = _FIND_GET_NEXT_CARD;
    else
    {
        in_stack->found_match_this_search = 1;
        in_stack->matches_this_search++;
         in_stack->find_state = _FIND_FINISH;
    }
}



/********
 Character Phrase Matching
 */

static void _find_char_phrase(Stack *in_stack)
{
    int matched_phrase = 0;
    while (in_stack->find_state_field_index < in_stack->find_state_field_count)
    {
        // continue search of field here
        for (;
             (in_stack->find_state_field_offset < in_stack->find_state_field_length) &&
             (in_stack->find_state_field_offset + in_stack->find_search_bytes <= in_stack->find_state_field_length);
             in_stack->find_state_field_offset++)
        {
            if (xte_cstrings_szd_equal(in_stack->find_search, in_stack->find_search_bytes,
                                       in_stack->find_state_field_content + in_stack->find_state_field_offset, in_stack->find_search_bytes))
            {
                matched_phrase = 1;
                in_stack->find_state_field_offset++; /* so we continue from the next phrase next search time... */
                
                // store match information in case we match soon
                in_stack->found_offset = xte_cstring_chars_between(in_stack->find_state_field_content,
                                                                   in_stack->find_state_field_content + in_stack->find_state_field_offset) - 1;
                in_stack->found_length = _xte_utf8_strlen(in_stack->find_search);
                in_stack->found_field_id = in_stack->find_state_fields[in_stack->find_state_field_index];
                in_stack->found_card_id = in_stack->find_state_card_id;
                in_stack->found_text = _stack_clone_cstr(in_stack->find_search);
                break;
            }
        }
        if (matched_phrase) break;
        
        // look at next field
        in_stack->find_state_field_index++;
        if (in_stack->find_state_field_index < in_stack->find_state_field_count)
        {
            // reset some state variables
            in_stack->find_state_field_offset = 0;
            in_stack->find_state_field_length = 0;
            
            // grab the content of the next field here
            if (in_stack->find_state_field_content) _stack_free(in_stack->find_state_field_content);
            in_stack->find_state_field_content = _field_content(in_stack,
                                                                in_stack->find_state_fields[in_stack->find_state_field_index],
                                                                in_stack->find_state_card_id);
            
            in_stack->find_state_field_offset = 0;
            in_stack->find_state_field_length = strlen(in_stack->find_state_field_content);
        }
    }
    
    if (!matched_phrase)
        in_stack->find_state = _FIND_GET_NEXT_CARD;
    else
    {
        in_stack->found_match_this_search = 1;
        in_stack->matches_this_search++;
        in_stack->find_state = _FIND_FINISH;
    }
}





/********
 Shutdown
 */

static void _find_check_any_result(Stack *in_stack)
{
    // chance here to check /
    //if (in_stack->found_match_this_search)
    // dont need because tihs is already available to the outside ayway....
    
    // change state to _FIND_FINISH
    in_stack->find_state = _FIND_FINISH;
}




/********
 Control and API
 */


// card at a time
static void _find_again(Stack *in_stack)
{
    if (in_stack->found_text)
        _stack_free(in_stack->found_text);
    in_stack->found_text = NULL;
    
    // branch to an appropriate state
    switch (in_stack->find_state)
    {
        case _FIND_GET_NEXT_CARD:
            _find_get_next_card(in_stack);
            break;
        case _FIND_CARD_SETUP:
            _find_card_setup(in_stack);
            break;
        //case _FIND_GET_CARD_FIELDS:
            //_find_get_card_fields(in_stack);
            //break;
        case _FIND_WORDS_FIRST:
            _find_words_first(in_stack);
            break;
        case _FIND_WORDS_OTHER:
            _find_words_other(in_stack);
            break;
            
        case _FIND_WORD_PHRASE:
            _find_word_phrase(in_stack);
            break;
        case _FIND_CHAR_PHRASE:
            _find_char_phrase(in_stack);
            break;
            
        case _FIND_CHECK_ANY_RESULT:
            _find_check_any_result(in_stack);
            break;
        case _FIND_FINISH:
            break;
            
        default:
            break;
    }
}


// our UI should probably put up a window with a determinate progress indicator
// while performing a search, anytime the search takes longer than 0.1 of a second

// could be a sheet?  how would this work with scripts?  scripts could disable the
// progress bar i suppose

// maybe it shouldn't be determinate - since scripts could potentially be performing
// many finds

// then again, the preferred mechanism for OS X applications is to display an abortable
// progress window for long processes, not simply change the cursor

// we could do this for long scripts too.  and provide an "Abort" button
// to be more intuitive than Command-Period .


// so find can be easily threaded using a platform specific threading mechanism
// or implemented pseudo by the UI
int stack_find_step(Stack *in_stack)
{
    if (in_stack->find_state == _FIND_FINISH) return 0;
    _find_again(in_stack);
    return 1;
}


void stack_find(Stack *in_stack, long in_card_id, enum FindMode in_mode, char *in_search, long in_field_id, int in_marked)
{
    if ((in_stack->find_mode != in_mode) ||
        (!in_stack->find_search) || 
        (in_stack->find_search && (strcmp(in_stack->find_search, in_search) != 0)) ||
        (in_stack->find_field_id != in_field_id) ||
        (in_stack->find_marked != in_marked))
    {
        printf("Starting a new find\n");
        stack_reset_find(in_stack);
        
        if (in_field_id > 0)
        {
            // if field specified, check if it's card/bkgnd
            // if it's card, we only search 1 card
            // set find_stop_card = 1 otherwise 0
            long card_id, bkgnd_id;
            stack_widget_owner(in_stack, in_field_id, &card_id, &bkgnd_id);
            in_stack->find_stop_card = (card_id != 0);
            
            // if it's bkgnd, we precheck that each card being searched belongs to the bkgnd during the search steps
            // set find_restrict_bkgnd = , otherwise 0
            in_stack->find_restrict_bkgnd = bkgnd_id;
        }
        else
        {
            // there are no card/bkgnd/field restrictions...
            in_stack->find_stop_card = 0;
            in_stack->find_restrict_bkgnd = 0;
        }
        
        in_stack->find_card_id = in_card_id;
        in_stack->find_state_card_id = in_card_id;
        in_stack->find_mode = in_mode;
        if (in_stack->find_search) _stack_free(in_stack->find_search);
        in_stack->find_search = _stack_clone_cstr(in_search);
        in_stack->find_search_bytes = strlen(in_stack->find_search);
        in_stack->find_field_id = in_field_id;
        in_stack->find_marked = in_marked;
        
        if ((in_stack->find_mode == FIND_WHOLE_WORDS) ||
            (in_stack->find_mode == FIND_WORDS_BEGINNING) ||
            (in_stack->find_mode == FIND_WORDS_CONTAINING) ||
            (in_stack->find_mode == FIND_WORD_PHRASE))
        {
            _stack_wordize(in_stack->find_search, &(in_stack->find_words), NULL, &(in_stack->find_word_count));
            // check for empty search terms
            if (in_stack->find_word_count == 0)
            {
                in_stack->find_state = _FIND_FINISH;
            }
        }
        else
        {
            // check for empty search terms
            if (in_stack->find_search_bytes == 0)
                in_stack->find_state = _FIND_FINISH;
        }
            
        
        
        
        
        in_stack->find_state = _FIND_CARD_SETUP;
        
        
    }
    else
    {
        if (in_stack->find_mode == FIND_WORD_PHRASE)
            in_stack->find_state = _FIND_WORD_PHRASE;
        else if (in_stack->find_mode == FIND_CHAR_PHRASE)
            in_stack->find_state = _FIND_CHAR_PHRASE;
        else
            in_stack->find_state = _FIND_WORDS_FIRST;
    }
        
}


/* the whole purpose of this function is to completely reset the find;
 and start configuring a find from scratch */

void stack_reset_find(Stack *in_stack)
{
    if (in_stack->found_text) _stack_free(in_stack->found_text);
    in_stack->found_text = NULL;
    if (in_stack->find_search) _stack_free(in_stack->find_search);
    in_stack->find_search = NULL;
    if (in_stack->find_state_fields) _stack_free(in_stack->find_state_fields);
    in_stack->find_state_fields = NULL;
    if (in_stack->find_state_field_content) _stack_free(in_stack->find_state_field_content);
    in_stack->find_state_field_content = NULL;
    if (in_stack->find_state_field_offsets) _stack_free(in_stack->find_state_field_offsets);
    in_stack->find_state_field_offsets = NULL;
    _stack_items_free(in_stack->find_words, in_stack->find_word_count);
    in_stack->find_words = NULL;
    _stack_items_free(in_stack->find_state_field_words, in_stack->find_state_field_word_count);
    in_stack->find_state_field_words = NULL;
    in_stack->matches_this_search = 0;

    /*in_stack->found_card_id = 0;
    if (in_stack->found_text) _stack_free(in_stack->found_text);
    in_stack->found_text = NULL;
    
    in_stack->find_state_card_id = 0;

    if (in_stack->find_state_fields)
        _stack_free(in_stack->find_state_fields);
    in_stack->find_state_fields = NULL;
    in_stack->find_state_field_count = 0;
    
    in_stack->found_match_this_search = 0;*/
}





/********
 Results Access
 */

int stack_find_result(Stack *in_stack, long *out_card_id, long *out_field_id, long *out_offset, long *out_length, char **out_text)
{
    if (!in_stack->found_match_this_search) return 0;
    
    *out_card_id = in_stack->found_card_id;
    *out_field_id = in_stack->found_field_id;
    *out_offset = in_stack->found_offset;
    *out_length = in_stack->found_length;
    *out_text = in_stack->found_text;
    
    return 1;
}


const char* stack_find_terms(Stack *in_stack)
{
    if (!in_stack->find_search) return "";
    return in_stack->find_search;
}



