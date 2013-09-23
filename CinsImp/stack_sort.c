//
//  stack_sort.c
//  CinsImp
//
//  Created by Joshua Hawcroft on 15/07/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#include "stack_int.h"
#include "sorter.h"

/*
 
 TODO to make this work:
 
 -  clone AST in xtalk_variant.c, _xte_ast_wrap()
    OR don't even bother, just wrap it and set something up later to prevent it being stored and reused outside
    of the current invokation - avoids a whole heap of issues
 -  new entry point to evaluate a variant-wrapped AST in the xtalk engine and return a variant value
 -  finishing stack_sort() below & enhancing ID tables accordingly
 -  writing the comparator, _compare_cards() below
 
 */


struct CardSortKey
{
    long card_id;
    XTEVariant *key;
};


struct SortContext
{
    long card_count;
    struct CardSortKey *sort_keys;
    XTE *xtalk;
};


static int _compare_cards(Sorter *in_sorter, struct SortContext *in_context, struct CardSortKey *in_item1, struct CardSortKey *in_item2)
{
    return xte_compare_variants(in_context->xtalk, in_item1->key, in_item2->key);
}


long stack_sorting_card(Stack *in_stack)
{
    if (in_stack->sort_card_id < 1) return STACK_NO_OBJECT;
    return in_stack->sort_card_id;
}


void _stack_card_table_flush(Stack *in_stack);



/*
 *  stack_sort
 *  ---------------------------------------------------------------------------------------------
 *  Sorts the cards of the stack using the specified XTE sort key.
 */
void stack_sort(Stack *in_stack, XTE *in_xtalk, long in_bkgnd_id, int in_marked,
                XTEVariant *in_sort_key, StackSortMode in_mode)
{
    //printf("SORTING bkgnd id:%ld\n", in_bkgnd_id);
    
    /* grab ID table for cards to be sorted */
    IDTable *idtable = in_stack->stack_card_table;
    
    /* create and configure a sorter */
    struct SortContext context;
    Sorter *sorter = sorter_create((SorterComparator)&_compare_cards, &context);
    if (!sorter) return app_out_of_memory_void();
    
    long *slot_indicies = NULL;
    
    /* construct a list of card IDs, indicies and sort keys;
     the sorter will swap IDs around, but leave the indicies stationary,
     allowing us to write the sorted list interleaved in the correct
     places between cards of other backgrounds at the end. */
    if (in_bkgnd_id != STACK_NO_OBJECT)
    {
        /* enumerate only cards in background */
        context.card_count = stack_bkgnd_card_count(in_stack, in_bkgnd_id);
        context.sort_keys = _stack_calloc(context.card_count, sizeof(struct CardSortKey));
        context.xtalk = in_xtalk;
        if (!context.sort_keys) return app_out_of_memory_void();
        
        slot_indicies = _stack_calloc(context.card_count, sizeof(long));
        if (!slot_indicies)
        {
            _stack_free(context.sort_keys);
            sorter_destroy(sorter);
            return;
        }
        
        long stack_card_count = idtable_size(idtable);
        int slot_index = 0;
        for (long card_index = 0; card_index < stack_card_count; card_index++)
        {
            /* grab next card in stack */
            long card_id = idtable_id_for_index(idtable, card_index);
            
            /* check if the card is in the background */
            long cards_bkgnd = _cards_bkgnd(in_stack, card_id);
            if (cards_bkgnd == in_bkgnd_id)
            {
                /* it is, so add it to the sort list */
                in_stack->sort_card_id = context.sort_keys[slot_index].card_id = card_id;
                    
                context.sort_keys[slot_index].key = xte_variant_value(in_xtalk, xte_evaluate_delayed_param(in_xtalk, in_sort_key));
                if (xte_has_error(in_xtalk))
                {
                    /* if there was a problem evaluating the expression,
                     stop and cleanup */
                    for (card_index = 0; card_index < context.card_count; card_index++)
                        xte_variant_release(context.sort_keys[card_index].key);
                    _stack_free(context.sort_keys);
                    sorter_destroy(sorter);
                    return;
                }
                
                sorter_add(sorter, &(context.sort_keys[slot_index]));
                slot_indicies[slot_index++] = card_index;
            }
        }
    }
    else
    {
        /* enumerate all cards in stack */
        context.card_count = idtable_size(idtable);
        context.sort_keys = _stack_calloc(context.card_count, sizeof(struct CardSortKey));
        context.xtalk = in_xtalk;
        if (!context.sort_keys) return app_out_of_memory_void();
        for (long card_index = 0; card_index < context.card_count; card_index++)
        {
            in_stack->sort_card_id = context.sort_keys[card_index].card_id = idtable_id_for_index(idtable, card_index);
            context.sort_keys[card_index].key = xte_variant_value(in_xtalk, xte_evaluate_delayed_param(in_xtalk, in_sort_key));
            if (xte_has_error(in_xtalk))
            {
                /* if there was a problem evaluating the expression,
                 stop and cleanup */
                for (card_index = 0; card_index < context.card_count; card_index++)
                    xte_variant_release(context.sort_keys[card_index].key);
                _stack_free(context.sort_keys);
                sorter_destroy(sorter);
                return;
            }
            
            sorter_add(sorter, &(context.sort_keys[card_index]));
        }
    }
    assert(sorter_count(sorter) == context.card_count);
    
    /* perform the sort */
    sorter_sort(sorter, (SorterDirection)in_mode);
    in_stack->sort_card_id = STACK_NO_OBJECT;
    
    if (slot_indicies)
    {
        /* mutate the ID table with the sorted sequence;
         only change the cards at the specified slot indicies */
        for (long card_index = 0; card_index < context.card_count; card_index++)
        {
            struct CardSortKey *card = sorter_item(sorter, card_index);
            idtable_mutate_slot(idtable, slot_indicies[card_index], card->card_id);
            
            xte_variant_release(card->key);
        }
        
        _stack_free(slot_indicies);
    }
    else
    {
        /* rebuild the ID table with the sorted sequence;
         and cleanup */
        idtable_clear(idtable);
        for (long card_index = 0; card_index < context.card_count; card_index++)
        {
            struct CardSortKey *card = sorter_item(sorter, card_index);
            idtable_append(idtable, card->card_id);
            
            xte_variant_release(card->key);
        }
    }
    
    /* cleanup */
    _stack_free(context.sort_keys);
    sorter_destroy(sorter);
    
    /* write new order of cards to disk */
    _stack_card_table_flush(in_stack);
}




