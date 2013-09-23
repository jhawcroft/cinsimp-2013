/*
 
 Application Control Unit - Handle Manager
 acu_mem.c
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Manages 'handles' which provide access to CinsImp related resources, such as Stacks, Cards,
 Buttons, Fields, Files and anything else that can be represented within the CinsImp environment
 
 Handles are representative of resources that may/may not exist, that may go out of scope, etc.
 
 Memory management is via reference counting, using _retain and _release calls.  There is an
 auto-release mechanism to make the glue code between the platform specific UI and the ACU a 
 little easier to write.
 
 ! Access by the UI to resources via a handle is guaranteed not to crash, even if the original
   resources are no longer available, for example, if a stack is closed.
 
 *************************************************************************************************
 */

#include "acu_int.h"


/******************
 Configuration
 */

#define _AUTO_RELEASE_ALLOC 10




/******************
 Internal Handle API
 */

/////// NON REENTRANT CRAPPY POOL NEEDS REPLACING ***!


static void _autoreleasor(StackHandle in_handle)
{
    acu_release(in_handle);
}


void _stackmgr_autorelease(HandleDef *in_handle)
{
    if (_stackmgr_has_fatal()) return;
    
    _acu_thread_mutex_lock(&(_g_acu.ar_pool_mutex));
    _acu_autorelease(_g_acu.ar_pool, in_handle, (ACUAutoreleasePoolReleasor)_autoreleasor);
    _acu_thread_mutex_unlock(&(_g_acu.ar_pool_mutex));
    
    /*if (_g_acu.auto_release_pool_count == _g_acu.auto_release_pool_alloc)
    {
        HandleDef **new_pool = realloc(_g_acu.auto_release_pool,
                                       sizeof(HandleDef*) * (_g_acu.auto_release_pool_alloc + _AUTO_RELEASE_ALLOC));
        if (!new_pool) return _stackmgr_raise_error(STACKMGR_ERROR_MEMORY);
        _g_acu.auto_release_pool = new_pool;
        _g_acu.auto_release_pool_alloc += _AUTO_RELEASE_ALLOC;
    }
    
    _g_acu.auto_release_pool[_g_acu.auto_release_pool_count++] = in_handle;*/
}


int _stackmgr_handle_is_stack(HandleDef *in_handle)
{
    if (!in_handle) return STACKMGR_FALSE;
    return (memcmp(in_handle->struct_id, _STACK_STRUCT_ID, sizeof(_STACK_STRUCT_ID)) == 0);
}


HandleDef* _stackmgr_handle_create(int in_flags)
{
    if (_stackmgr_has_fatal()) return NULL;
    
    HandleDef *result = calloc(1, sizeof(HandleDef));
    
    if (!_acu_thread_mutex_create(&(result->mutex)))
    {
        _acu_raise_error(XTE_ERROR_MEMORY);
        return NULL;
    }
    
    memcpy(result->struct_id, _STACKMGR_HANDLE_STRUCT_ID, sizeof(_STACKMGR_HANDLE_STRUCT_ID));
    result->ref_count = 1;
    if (in_flags & STACKMGR_FLAG_AUTO_RELEASE) _stackmgr_autorelease(result);
    assert(IS_STACKHANDLE(result));
    
    return result;
}


static void _stackmgr_handle_dispose(HandleDef *in_handle)
{
    assert(IS_STACKHANDLE(in_handle));
    
    _acu_thread_mutex_dispose(&(in_handle->mutex));
    free(in_handle);
}


void _stackmgr_handle_release(HandleDef *in_handle)
{
    //if (_stackmgr_has_fatal()) return;
    
    if (_stackmgr_handle_is_stack(in_handle)) return;
    assert(IS_STACKHANDLE(in_handle));
    
    _acu_thread_mutex_lock(&(in_handle->mutex));
    
    if (in_handle->ref_count > 0) in_handle->ref_count--;
    if (in_handle->ref_count == 0)
    {
        _acu_thread_mutex_unlock(&(in_handle->mutex));
        _stackmgr_handle_dispose(in_handle);
        return;
    }
    
    _acu_thread_mutex_unlock(&(in_handle->mutex));
}


void _stackmgr_handle_retain(HandleDef *in_handle)
{
    if (_stackmgr_has_fatal()) return;
    
    if (_stackmgr_handle_is_stack(in_handle)) return;
    assert(IS_STACKHANDLE(in_handle));
    
    _acu_thread_mutex_lock(&(in_handle->mutex));
    in_handle->ref_count++;
    _acu_thread_mutex_unlock(&(in_handle->mutex));
}

/* make sure we don't do drain while stacks have scripts running - or else - multithread the handle manager;
 which would probably be a good idea anyway **TODO** */

void stackmgr_arp_drain(void)
{
    if (_stackmgr_has_fatal()) return;
    
    _acu_thread_mutex_lock(&(_g_acu.ar_pool_mutex));
    _acu_autorelease_pool_drain(_g_acu.ar_pool);
    _acu_thread_mutex_unlock(&(_g_acu.ar_pool_mutex));
    
    /*
    for (int i = 0; i < _g_acu.auto_release_pool_count; i++)
    {
        _stackmgr_handle_release(_g_acu.auto_release_pool[i]);
    }
    _g_acu.auto_release_pool_count = 0;
     */
}


static int _stackmgr_handles_same(HandleDef *in_handle_1, HandleDef *in_handle_2)
{
    if ((!in_handle_1) || (!in_handle_2)) return STACKMGR_FALSE;
    if (IS_STACK(in_handle_1) && IS_STACK(in_handle_2) && (in_handle_1 == in_handle_2)) return STACKMGR_TRUE;
    if (!IS_STACKHANDLE(in_handle_1)) return STACKMGR_FALSE;
    if (!IS_STACKHANDLE(in_handle_2)) return STACKMGR_FALSE;
    return (memcmp(&(in_handle_1->reference), &(in_handle_2->reference), sizeof(struct HandleRef)) == 0);
}


static void _stackmgr_build_description(HandleDef *in_handle, int in_flags)
{
    if (!in_handle)
    {
        strcpy(_g_acu.result_handle_desc, "INVALID HANDLE");
        return;
    }
    
    if (_stackmgr_handle_is_stack(in_handle))
    {
        sprintf(_g_acu.result_handle_desc, "stack \"%s\"", stack_name((Stack*)in_handle));
        return;
    }
    
    if (_stackmgr_stack((StackHandle)in_handle) == NULL)
    {
        strcpy(_g_acu.result_handle_desc, "INVALID HANDLE");
        return;
    }
    
    switch (in_handle->reference.type)
    {
        case STACKMGR_TYPE_BKGND:
        {
            char const *name;
            name = stack_prop_get_string(in_handle->reference.register_entry->stack,
                                         STACK_NO_OBJECT,
                                         in_handle->reference.layer_id,
                                         PROPERTY_NAME);
            if (name && (name[0] != 0))
                sprintf(_g_acu.result_handle_desc, "bkgnd id %ld = \"%s\"",
                        in_handle->reference.layer_id, name);
            else
                sprintf(_g_acu.result_handle_desc, "bkgnd id %ld",
                        in_handle->reference.layer_id);
            break;
        }
        case STACKMGR_TYPE_CARD:
        {
            char const *name;
            name = stack_prop_get_string(in_handle->reference.register_entry->stack,
                                         in_handle->reference.layer_id,
                                         STACK_NO_OBJECT,
                                         PROPERTY_NAME);
            if (name && (name[0] != 0))
                sprintf(_g_acu.result_handle_desc, "card id %ld = \"%s\"",
                        in_handle->reference.layer_id, name);
            else
                sprintf(_g_acu.result_handle_desc, "card id %ld",
                        in_handle->reference.layer_id);
            break;
        }
        case STACKMGR_TYPE_FIELD:
        {
            char const *name;
            if (stack_widget_is_card(in_handle->reference.register_entry->stack, in_handle->reference.widget_id))
            {
                name = stack_widget_prop_get_string(in_handle->reference.register_entry->stack,
                                                    in_handle->reference.widget_id,
                                                    in_handle->reference.layer_id,
                                                    PROPERTY_NAME);
                if (name && (name[0] != 0))
                    sprintf(_g_acu.result_handle_desc, "card field id %ld = \"%s\"",
                            in_handle->reference.widget_id, name);
                else
                    sprintf(_g_acu.result_handle_desc, "card field id %ld",
                            in_handle->reference.widget_id);
            }
            else
            {
                name = stack_widget_prop_get_string(in_handle->reference.register_entry->stack,
                                                    in_handle->reference.widget_id,
                                                    STACK_NO_OBJECT,
                                                    PROPERTY_NAME);
                if (name && (name[0] != 0))
                    sprintf(_g_acu.result_handle_desc, "bkgnd field id %ld = \"%s\"",
                            in_handle->reference.widget_id, name);
                else
                    sprintf(_g_acu.result_handle_desc, "bkgnd field id %ld",
                            in_handle->reference.widget_id);
            }
            break;
        }
        case STACKMGR_TYPE_BUTTON:
        {
            char const *name;
            if (stack_widget_is_card(in_handle->reference.register_entry->stack, in_handle->reference.widget_id))
            {
                name = stack_widget_prop_get_string(in_handle->reference.register_entry->stack,
                                                    in_handle->reference.widget_id,
                                                    in_handle->reference.layer_id,
                                                    PROPERTY_NAME);
                if (name && (name[0] != 0))
                    sprintf(_g_acu.result_handle_desc, "card button id %ld = \"%s\"",
                            in_handle->reference.widget_id, name);
                else
                    sprintf(_g_acu.result_handle_desc, "card button id %ld",
                            in_handle->reference.widget_id);
            }
            else
            {
                name = stack_widget_prop_get_string(in_handle->reference.register_entry->stack,
                                                    in_handle->reference.widget_id,
                                                    STACK_NO_OBJECT,
                                                    PROPERTY_NAME);
                if (name && (name[0] != 0))
                    sprintf(_g_acu.result_handle_desc, "bkgnd button id %ld = \"%s\"",
                            in_handle->reference.widget_id, name);
                else
                    sprintf(_g_acu.result_handle_desc, "bkgnd button id %ld",
                            in_handle->reference.widget_id);
            }
            break;
        }
    }
}



/******************
 Public Handle API
 */

int stackmgr_handles_equivalent(StackHandle in_handle_1, StackHandle in_handle_2)
{
    return _stackmgr_handles_same((HandleDef*)in_handle_1, (HandleDef*)in_handle_2);
}


char const* stackmgr_handle_description(StackHandle in_handle, int in_flags)
{
    if (_stackmgr_has_fatal()) return NULL;
    _stackmgr_build_description((HandleDef*)in_handle, in_flags);
    return _g_acu.result_handle_desc;
}



StackHandle acu_handle_for_card_id(int in_flags, StackHandle in_stack, long in_card_id)
{
    if (_stackmgr_has_fatal()) return NULL;
    
    HandleDef *handle = _stackmgr_handle_create(in_flags);
    
    handle->reference.type = STACKMGR_TYPE_CARD;
    handle->reference.register_entry = _stackmgr_stack(in_stack);
    handle->reference.session_id = (handle->reference.register_entry ? handle->reference.register_entry->session_id : 0);
    //handle->reference.stack = (Stack*)in_stack;
    handle->reference.layer_id = in_card_id;
    handle->reference.layer_is_card = STACKMGR_TRUE;
    
    return (StackHandle)handle;
}


StackHandle stackmgr_handle_for_bkgnd_id(int in_flags, StackHandle in_stack, long in_bkgnd_id)
{
    if (_stackmgr_has_fatal()) return NULL;
    
    HandleDef *handle = _stackmgr_handle_create(in_flags);
    
    handle->reference.type = STACKMGR_TYPE_BKGND;
    handle->reference.register_entry = _stackmgr_stack(in_stack);
    handle->reference.session_id = (handle->reference.register_entry ? handle->reference.register_entry->session_id : 0);
    //handle->reference.stack = (Stack*)in_stack;
    handle->reference.layer_id = in_bkgnd_id;
    handle->reference.layer_is_card = STACKMGR_FALSE;
    
    return (StackHandle)handle;
}


StackHandle acu_handle_for_field_id(int in_flags, StackHandle in_owner, long in_field_id, StackHandle in_card)
{
    if (_stackmgr_has_fatal()) return NULL;
    
    //HandleDef *owner = (HandleDef*)in_owner;
    HandleDef *handle = _stackmgr_handle_create(in_flags);
    
    handle->reference.type = STACKMGR_TYPE_FIELD;
    handle->reference.register_entry = _stackmgr_stack(in_owner);
    handle->reference.session_id = (handle->reference.register_entry ? handle->reference.register_entry->session_id : 0);
    
    handle->reference.widget_id = in_field_id;
    
    if (in_card)
    {
        HDEF(handle).layer_id = HDEF(in_card).layer_id;
        HDEF(handle).layer_is_card = HDEF(in_card).layer_is_card;
    }
    //handle->reference.layer_id = owner->reference.layer_id;
    //handle->reference.layer_is_card = owner->reference.layer_is_card;
    
    
    return (StackHandle)handle;
}


StackHandle acu_handle_for_button_id(int in_flags, StackHandle in_owner, long in_button_id)
{
    if (_stackmgr_has_fatal()) return NULL;
    
    HandleDef *owner = (HandleDef*)in_owner;
    HandleDef *handle = _stackmgr_handle_create(in_flags);
    
    handle->reference.type = STACKMGR_TYPE_BUTTON;
    handle->reference.register_entry = _stackmgr_stack(in_owner);
    handle->reference.session_id = (handle->reference.register_entry ? handle->reference.register_entry->session_id : 0);
    //handle->reference.stack = owner->reference.stack;
    handle->reference.layer_id = owner->reference.layer_id;
    handle->reference.layer_is_card = owner->reference.layer_is_card;
    handle->reference.widget_id = in_button_id;
    
    return (StackHandle)handle;
}






StackHandle stackmgr_handle_retain(StackHandle in_handle)
{
    if (!in_handle) return NULL;
    _stackmgr_handle_retain((HandleDef*)in_handle);
    return in_handle;
}


StackHandle stackmgr_handle_release(StackHandle in_handle)
{
    if (!in_handle) return NULL;
    _stackmgr_handle_release((HandleDef*)in_handle);
    return in_handle;
}


StackHandle stackmgr_autorelease(StackHandle in_handle)
{
    if (!in_handle) return NULL;
    _stackmgr_autorelease((HandleDef*)in_handle);
    return in_handle;
}


StackHandle acu_autorelease(StackHandle in_handle)
{
    return stackmgr_autorelease(in_handle);
}


StackHandle acu_retain(StackHandle in_handle)
{
    return stackmgr_handle_retain(in_handle);
}


StackHandle acu_release(StackHandle in_handle)
{
    return stackmgr_handle_release(in_handle);
}





StackHandle acu_handle_for_current_card(int in_flags, StackHandle in_stack)
{
    StackMgrStack *the_stack = _stackmgr_stack(in_stack);
    assert(the_stack != NULL);
    
    HandleDef* handle = _stackmgr_handle_create(in_flags);
    handle->reference.register_entry = _stackmgr_stack(in_stack);
    handle->reference.session_id = (handle->reference.register_entry ? handle->reference.register_entry->session_id : 0);
    handle->reference.type = STACKMGR_TYPE_CARD;
    handle->reference.layer_is_card = ACU_TRUE;
    handle->reference.layer_id = _stackmgr_current_card_id(the_stack);
    
    return (StackHandle)handle;
}


StackHandle acu_handle_for_current_bkgnd(int in_flags, StackHandle in_stack)
{
    StackMgrStack *the_stack = _stackmgr_stack(in_stack);
    assert(the_stack != NULL);
    
    HandleDef* handle = _stackmgr_handle_create(in_flags);
    handle->reference.register_entry = _stackmgr_stack(in_stack);
    handle->reference.session_id = (handle->reference.register_entry ? handle->reference.register_entry->session_id : 0);
    handle->reference.type = STACKMGR_TYPE_BKGND;
    handle->reference.layer_is_card = ACU_FALSE;
    handle->reference.layer_id = _stackmgr_current_bkgnd_id(the_stack);
    
    return (StackHandle)handle;
}




