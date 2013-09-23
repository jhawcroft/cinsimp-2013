/*
 
 Application Control Unit - Icon Manager UI
 acu_iconmgr.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Icon Manager; underlying mechanism behind the UI
 
 Note:  xTalk scripts will access icons via a different mechanism within the ACU;
 This mechanism is provided as a convenience to the Cocoa UI code which wraps around it.
 
 *************************************************************************************************
 */

#include "acu_int.h"


/* TODO:  ! this needs to lock access to the stack while it's doing the recaching,
 halting xTalk scripts,
 
 the problem with this is it's trying to cache image data, which is going to require
 a large amount of memory, unless we cache it to a scrap file which seems silly.
 
 probably we should just prevent access to the icon manager while the xtalk interpreter
 is running - and use that mechanism we're already using to do so for other bits
 of the UI, eg. in validation
 
 or even, not bother, just block access if a script tries to mutate an icon.  is that
 even possible?
 */


/*
 *  PNG_ICON_RES_TYPE
 *  ---------------------------------------------------------------------------------------------
 *  This is the resource type used to represent icons within the Stack unit.
 *  Icons are presently stored as PNG format data.
 */
#define PNG_ICON_RES_TYPE "PNGICON"


/* need to build a cache of the icons
 specifically for the UI; that way if a script changes things
 it wont bugger the UI up,
 we must remember to invalidate the cache internally
 if stuff is changed and send a callback to
 have the actual UI reload */

void _acu_iconmgr_dispose(StackMgrStack *in_stack)
{
    assert(in_stack != NULL);
    if (in_stack->iconmgr_cache)
    {
        for (int i = 0; i < in_stack->iconmgr_cache_size; i++)
        {
            ACUCachedIcon *icon = &(in_stack->iconmgr_cache[i]);
            if (icon->the_name) free(icon->the_name);
            if (icon->the_data) free(icon->the_data);
        }
        free(in_stack->iconmgr_cache);
    }
    in_stack->iconmgr_cache = NULL;
    in_stack->iconmgr_cache_size = 0;
}


void _acu_iconmgr_recache(StackMgrStack *in_stack)
{
    assert(in_stack != NULL);
    
    _acu_iconmgr_dispose(in_stack);
    
    int builtin_count = stack_res_count(_g_acu.builtin_resources, PNG_ICON_RES_TYPE);
    int local_count = stack_res_count(in_stack->stack, PNG_ICON_RES_TYPE);
    
    in_stack->iconmgr_cache = calloc(local_count + builtin_count, sizeof(ACUCachedIcon));
    if (!in_stack->iconmgr_cache)
    {
        _acu_raise_error(ACU_ERROR_MEMORY);
        return;
    }
    
    int icon_count = 0;
    
    for (int i = 1; i <= builtin_count; i++)
    {
        ACUCachedIcon *icon = &(in_stack->iconmgr_cache[icon_count++]);
        icon->the_id = stack_res_n(_g_acu.builtin_resources, PNG_ICON_RES_TYPE, i);
        icon->the_stack = _g_acu.builtin_resources;
        char *temp_name;
        if (stack_res_get(_g_acu.builtin_resources, icon->the_id, PNG_ICON_RES_TYPE, &temp_name, NULL, NULL))
            icon->the_name = _acu_clone_cstr(temp_name);
    }
    
    for (int i = 1; i <= local_count; i++)
    {
        int the_id = stack_res_n(in_stack->stack, PNG_ICON_RES_TYPE, i);
        
        ACUCachedIcon *icon = NULL;
        for (int j = 0; j < icon_count; j++)
        {
            icon = &(in_stack->iconmgr_cache[j]);
            if (icon->the_id == the_id) break;
            icon = NULL;
        }
        if (icon == NULL) icon = &(in_stack->iconmgr_cache[icon_count++]);
        
        if (icon->the_name) free(icon->the_name);
        icon->the_name = NULL;
        
        icon->the_stack = in_stack->stack;
        icon->the_id = the_id;
        
        char *temp_name;
        if (stack_res_get(in_stack->stack, icon->the_id, PNG_ICON_RES_TYPE, &temp_name, NULL, NULL))
            icon->the_name = _acu_clone_cstr(temp_name);
    }
    
    in_stack->iconmgr_cache_size = icon_count;
}


/* update the cache so only the bitmaps in the specified range are loaded */
void acu_iconmgr_preview_range(StackHandle in_stack, int in_from_number, int in_to_number)
{
    assert(in_stack != NULL);
    StackMgrStack *stack = _stackmgr_stack(in_stack);
    assert(stack != NULL);
    
    //printf("Range: %d to %d (inclusive)\n", in_from_number, in_to_number);
    
    for (int i = 0; i < stack->iconmgr_cache_size; i++)
    {
        ACUCachedIcon *icon = &(stack->iconmgr_cache[i]);
        int in_range = ((i + 1 >= in_from_number) && (i + 1 <= in_to_number));
        
        if ((!in_range) && icon->the_data)
        {
            free(icon->the_data);
            icon->the_data = NULL;
            icon->the_size = 0;
        }
        
        else if (in_range && (!icon->the_data))
        {
            void *temp_data;
            long temp_size;
            if (stack_res_get(icon->the_stack, icon->the_id, PNG_ICON_RES_TYPE, NULL, &temp_data, &temp_size))
            {
                icon->the_data = malloc(temp_size + 1);
                if (icon->the_data)
                {
                    memcpy(icon->the_data, temp_data, temp_size);
                    icon->the_size = temp_size;
                }
            }
        }
    }
}




int acu_iconmgr_count(StackHandle in_stack)
{
    assert(in_stack != NULL);
    StackMgrStack *stack = _stackmgr_stack(in_stack);
    assert(stack != NULL);
    if (!stack->iconmgr_cache) _acu_iconmgr_recache(stack);
    return stack->iconmgr_cache_size;
}


void acu_iconmgr_icon_n(StackHandle in_stack, int in_index, int *out_id, char **out_name, void **out_data, long *out_size)
{
    assert(in_stack != NULL);
    StackMgrStack *stack = _stackmgr_stack(in_stack);
    assert(stack != NULL);
    if (!stack->iconmgr_cache) _acu_iconmgr_recache(stack);
    
    if (out_id) *out_id = 0;
    if (out_name) *out_name = "";
    if (out_data) *out_data = "";
    if (out_size) *out_size = 0;
    if ((in_index < 0) || (in_index >= stack->iconmgr_cache_size)) return;
    
    ACUCachedIcon *icon = &(stack->iconmgr_cache[in_index]);
    if (out_id) *out_id = icon->the_id;
    if (out_name) *out_name = icon->the_name;
    if (out_data) *out_data = icon->the_data;
    if (out_size) *out_size = icon->the_size;
}


void acu_icon_data(StackHandle in_stack, int in_id, void **out_data, long *out_size)
{
    assert(in_stack != NULL);
    StackMgrStack *stack = _stackmgr_stack(in_stack);
    assert(stack != NULL);
    
    if (!stack_res_get(stack->stack, in_id, PNG_ICON_RES_TYPE, NULL, out_data, out_size))
    {
        stack_res_get(_g_acu.builtin_resources, in_id, PNG_ICON_RES_TYPE, NULL, out_data, out_size);
    }
}



/*
void acu_iconmgr_filter(StackHandle in_stack, char const *in_filter)
{
    assert(in_stack != NULL);
    StackMgrStack *stack = _stackmgr_stack(in_stack);
    assert(stack != NULL);
    
    if (!stack->iconmgr_cache) _acu_iconmgr_recache(stack);
    
    
}
*/

int acu_iconmgr_create(StackHandle in_stack)
{
    assert(in_stack != NULL);
    StackMgrStack *stack = _stackmgr_stack(in_stack);
    assert(stack != NULL);
    int result = stack_res_create(stack->stack, STACK_NO_OBJECT, PNG_ICON_RES_TYPE, "");
    if (result) _acu_iconmgr_recache(stack);
    return result;
}


int acu_iconmgr_mutate(StackHandle in_stack, int in_id, int const *in_new_id, char const *in_new_name, void const *in_data, long in_size)
{
    assert(in_stack != NULL);
    StackMgrStack *stack = _stackmgr_stack(in_stack);
    assert(stack != NULL);
    int result = stack_res_set(stack->stack, in_id, PNG_ICON_RES_TYPE, in_new_id, in_new_name, in_data, in_size);
    if (result) _acu_iconmgr_recache(stack);
    return result;
}


int acu_iconmgr_delete(StackHandle in_stack, int in_id)
{
    assert(in_stack != NULL);
    StackMgrStack *stack = _stackmgr_stack(in_stack);
    assert(stack != NULL);
    int result = stack_res_delete(stack->stack, in_id, PNG_ICON_RES_TYPE);
    if (result) _acu_iconmgr_recache(stack);
    return result;
}






