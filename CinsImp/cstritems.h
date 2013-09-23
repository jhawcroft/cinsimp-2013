/*
 
 C-String Items
 cstritems.h
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Provides comma-delimited itemizing and item list manipulation
 
 (currently employed by the properties inspector palette to work with certain string properties,
 such as the TextStyle property of fields)
 
 */

#ifndef listedStyles_cstritems_h
#define listedStyles_cstritems_h


int cstr_has_item(const char *in_string, const char *in_item);
const char* cstr_item_add(const char *in_string, const char *in_item);
const char* cstr_item_remove(const char *in_string, const char *in_item);


#endif
