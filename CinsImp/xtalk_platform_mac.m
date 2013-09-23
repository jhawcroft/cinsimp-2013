/*
 
 xTalk Engine MacOS X Specific Glue
 xtalk_platform_mac.m
 
 CinsImp
 Copyright (c) 2010-2013 Joshua Hawcroft
 <www.joshhawcroft.com/CinsImp/>
 
 Mac OS X glue code
 
 */

#import <Foundation/Foundation.h>
#include "xtalk_platform.h"
#include <stdlib.h>



#define XTALK_TIMESTAMP_EPOCH_DATE @"1983-09-19 16:30:00 +0930"


/*********
 Date/Time
 */

@interface XTEDateTimeOSContext : NSObject
{
@public
    NSDate *_working_date;
    NSDate *_reference_date;
    NSDataDetector *_detector;
    
    char *_result;
}
@end
@implementation XTEDateTimeOSContext
@end


void _xte_os_current_datetime(XTEDateTimeOSContext *in_context)
{
    @autoreleasepool {
        in_context->_working_date = [NSDate date];
    }
}


void* _xte_os_date_init(void)
{
    @autoreleasepool {
        XTEDateTimeOSContext *context = [[XTEDateTimeOSContext alloc] init];
        
        context->_reference_date = [NSDate dateWithString:XTALK_TIMESTAMP_EPOCH_DATE];
        _xte_os_current_datetime(context);
        
        NSError *error = nil;
        context->_detector = [NSDataDetector dataDetectorWithTypes:NSTextCheckingTypeDate error:&error];
        
        context->_result = NULL;
        
        return (void*)CFBridgingRetain(context);
    }
}


void _xte_os_date_deinit(XTEDateTimeOSContext *in_context)
{
    @autoreleasepool {
        if (in_context->_result) free(in_context->_result);
        CFBridgingRelease((__bridge CFTypeRef)(in_context));
    }
}


double _xte_os_timestamp(XTEDateTimeOSContext *in_context)
{
    @autoreleasepool {
        return [in_context->_working_date timeIntervalSinceDate:in_context->_reference_date];
    }
}


void _xte_os_conv_timestamp(XTEDateTimeOSContext *in_context, double in_timestamp)
{
    @autoreleasepool {
        in_context->_working_date = [NSDate dateWithTimeInterval:in_timestamp sinceDate:in_context->_reference_date];
    }
}


void _xte_os_dateitems(XTEDateTimeOSContext *in_context, int *out_year, int *out_month, int *out_dayOfMonth,
                       int *out_hour24, int *out_minute, int *out_second, int *out_dayOfWeek)
{
    @autoreleasepool {
        NSCalendar *gregorian = [[NSCalendar alloc] initWithCalendarIdentifier:NSGregorianCalendar];
        NSDateComponents *parts = [gregorian components:NSYearCalendarUnit | NSMonthCalendarUnit |
                                   NSDayCalendarUnit | NSHourCalendarUnit | NSMinuteCalendarUnit |
                                   NSSecondCalendarUnit | NSWeekdayCalendarUnit fromDate:in_context->_working_date];
        *out_year = (int)[parts year];
        *out_month = (int)[parts month];
        *out_dayOfMonth = (int)[parts day];
        
        *out_hour24 = (int)[parts hour];
        *out_minute = (int)[parts minute];
        *out_second = (int)[parts second];
        
        *out_dayOfWeek = (int)[parts weekday];
    }
}


void _xte_os_conv_dateitems(XTEDateTimeOSContext *in_context, int in_year, int in_month, int in_dayOfMonth,
                            int in_hour24, int in_minute, int in_second, int in_dayOfWeek)
{
    @autoreleasepool {
        NSDateComponents *parts = [[NSDateComponents alloc] init];
        [parts setCalendar:[[NSCalendar alloc] initWithCalendarIdentifier:NSGregorianCalendar]];
        
        [parts setYear:in_year];
        [parts setMonth:in_month];
        [parts setDay:in_dayOfMonth];
        
        [parts setHour:in_hour24];
        [parts setMinute:in_minute];
        [parts setSecond:in_second];
        
        [parts setWeekday:in_dayOfWeek];
        
        in_context->_working_date = [parts date];
    }
}


char const* _xte_os_date_string(XTEDateTimeOSContext *in_context, int in_format)
{
    @autoreleasepool {
        NSDateFormatter *formatter = [[NSDateFormatter alloc] init];
        switch (in_format)
        {
            case XTE_DATE_SHORT:
                [formatter setDateStyle:NSDateFormatterShortStyle];
                [formatter setTimeStyle:NSDateFormatterNoStyle];
                break;
            case XTE_DATE_ABBREVIATED:
                [formatter setDateStyle:NSDateFormatterLongStyle];
                [formatter setTimeStyle:NSDateFormatterNoStyle];
                break;
            case XTE_DATE_LONG:
                [formatter setDateStyle:NSDateFormatterFullStyle];
                [formatter setTimeStyle:NSDateFormatterNoStyle];
                break;
            case XTE_TIME_SHORT:
                [formatter setDateStyle:NSDateFormatterNoStyle];
                [formatter setTimeStyle:NSDateFormatterShortStyle];
                break;
            case XTE_TIME_LONG:
                [formatter setDateStyle:NSDateFormatterNoStyle];
                [formatter setTimeStyle:NSDateFormatterMediumStyle];
                break;
            case XTE_MONTH_SHORT:
                [formatter setDateFormat:@"MMM"];
                break;
            case XTE_MONTH_LONG:
                [formatter setDateFormat:@"MMMM"];
                break;
            case XTE_WEEKDAY_SHORT:
                [formatter setDateFormat:@"EEE"];
                break;
            case XTE_WEEKDAY_LONG:
                [formatter setDateFormat:@"EEEE"];
                break;
        }
        
        NSString *desc = [formatter stringFromDate:in_context->_working_date];
        char const *utf8_str = [desc UTF8String];
        
        if (in_context->_result) free(in_context->_result);
        in_context->_result = malloc(strlen(utf8_str) + 1);
        strcpy(in_context->_result, utf8_str);
        
        return in_context->_result;
    }
}


void _xte_os_parse_date(XTEDateTimeOSContext *in_context, char const *in_date)
{
    @autoreleasepool {
        NSString *string = [NSString stringWithCString:in_date encoding:NSUTF8StringEncoding];
        NSArray *matches = [in_context->_detector matchesInString:string options:0 range:NSMakeRange(0, [string length])];
        for (NSTextCheckingResult *match in matches) {
            if (match.date)
            {
                in_context->_working_date = match.date;
                return;
            }
        }
    }
}






/* string */

int utf8_compare(const char *in_string1, const char *in_string2)
{
    @autoreleasepool {
        NSString *string1 = [NSString stringWithCString:in_string1 encoding:NSUTF8StringEncoding];
        NSString *string2 = [NSString stringWithCString:in_string2 encoding:NSUTF8StringEncoding];
        return [string1 localizedCaseInsensitiveCompare:string2];
    }
}


int utf8_contains(const char *in_string1, const char *in_string2)
{
    @autoreleasepool {
        NSString *string1 = [NSString stringWithCString:in_string1 encoding:NSUTF8StringEncoding];
        NSString *string2 = [NSString stringWithCString:in_string2 encoding:NSUTF8StringEncoding];
        return ([string1 rangeOfString:string2 options:NSCaseInsensitiveSearch].location != NSNotFound);
    }
}

/*
static NSString *utf8_string = NULL;
static NSArray *utf8_words = NULL;
static NSArray *utf8_lines = NULL;
static NSArray *utf8_items = NULL;
static NSString *utf8_itemdelim = @",";

void _xte_platform_utf8_begin(const char *in_string)
{
    @autoreleasepool {
    utf8_string = [NSString stringWithCString:in_string encoding:NSUTF8StringEncoding];
    utf8_words = NULL;
    utf8_lines = NULL;
    utf8_items = NULL;
    }
}

int _xte_platform_utf8_length(void)
{
    return (int)[utf8_string length];
}

int _xte_platform_utf8_character(int in_index)
{
    @try {
        return [utf8_string characterAtIndex:in_index];
    }
    @catch (NSException *exception) {
        return 0;
    }
}

const char* _xte_platform_utf8_substring(int in_offset, int in_length)
{
    @autoreleasepool {
    if (in_offset < 0) in_offset = 0;
    if (in_offset > [utf8_string length]) in_offset = (int)[utf8_string length];
    if (in_offset + in_length > [utf8_string length])
        in_length = (int)[utf8_string length] - in_offset;
    NSRange subrange = NSMakeRange(in_offset, in_length);
    return [[utf8_string substringWithRange:subrange] UTF8String];
    }
}


static void _utf8_get_words(void)
{
    @autoreleasepool {
    if (!utf8_words)
    {
        NSCharacterSet *delimiterCharacterSet = [NSCharacterSet whitespaceAndNewlineCharacterSet];
        utf8_words = [utf8_string componentsSeparatedByCharactersInSet:delimiterCharacterSet];
    }
    }
}

int _xte_platform_utf8_word_count(void)
{
    _utf8_get_words();
    return (int)[utf8_words count];
}

const char* _xte_platform_utf8_subwords(int in_offset, int in_length)
{
    @autoreleasepool {
    _utf8_get_words();
    if (in_offset < 0) in_offset = 0;
    if (in_offset > [utf8_words count]) in_offset = (int)[utf8_words count];
    if (in_offset + in_length > [utf8_words count])
        in_length = (int)[utf8_words count] - in_offset;
    NSRange subrange = NSMakeRange(in_offset, in_length);
    return [[[utf8_words subarrayWithRange:subrange] componentsJoinedByString:@" "] UTF8String];
    }
}


static void _utf8_get_lines(void)
{
    @autoreleasepool {
    if (!utf8_lines)
    {
        NSCharacterSet *delimiterCharacterSet = [NSCharacterSet newlineCharacterSet];
        utf8_lines = [utf8_string componentsSeparatedByCharactersInSet:delimiterCharacterSet];
    }
    }
}

int _xte_platform_utf8_line_count(void)
{
    _utf8_get_lines();
    return (int)[utf8_lines count];
}

const char* _xte_platform_utf8_sublines(int in_offset, int in_length)
{
    @autoreleasepool {
    _utf8_get_lines();
    if (in_offset < 0) in_offset = 0;
    if (in_offset > [utf8_lines count]) in_offset = (int)[utf8_lines count];
    if (in_offset + in_length > [utf8_lines count])
        in_length = (int)[utf8_lines count] - in_offset;
    NSRange subrange = NSMakeRange(in_offset, in_length);
    return [[[utf8_lines subarrayWithRange:subrange] componentsJoinedByString:@"\n"] UTF8String];
    }
}


static void _utf8_get_items(void)
{
    @autoreleasepool {
    if (!utf8_items)
    {
        utf8_items = [utf8_string componentsSeparatedByString:utf8_itemdelim];
    }
    }
}

void _xte_platform_utf8_set_itemdelimiter(const char *in_delim)
{
    @autoreleasepool {
    utf8_itemdelim = [NSString stringWithCString:in_delim encoding:NSUTF8StringEncoding];
    }
}

int _xte_platform_utf8_item_count(void)
{
    _utf8_get_items();
    return (int)[utf8_items count];
}

const char* _xte_platform_utf8_subitems(int in_offset, int in_length)
{
    @autoreleasepool {
    _utf8_get_items();
    if (in_offset < 0) in_offset = 0;
    if (in_offset > [utf8_items count]) in_offset = (int)[utf8_items count];
    if (in_offset + in_length > [utf8_items count])
        in_length = (int)[utf8_items count] - in_offset;
    NSRange subrange = NSMakeRange(in_offset, in_length);
    return [[[utf8_items subarrayWithRange:subrange] componentsJoinedByString:utf8_itemdelim] UTF8String];
    }
}
*/

/* host */

void _xte_platform_sys_version(int *out_major, int *out_minor, int *out_bugfix)
{
    @autoreleasepool {
        int mMajor = 10;
        int mMinor = 8;
        int mBugfix = 0;
        
        NSString* versionString = [[NSDictionary dictionaryWithContentsOfFile:@"/System/Library/CoreServices/SystemVersion.plist"] objectForKey:@"ProductVersion"];
        NSArray* versions = [versionString componentsSeparatedByString:@"."];
        if (versions.count < 2) return;
        if ( versions.count >= 1 ) {
            mMajor = (int)[versions[0] integerValue];
        }
        if ( versions.count >= 2 ) {
            mMinor = (int)[versions[1] integerValue];
        }
        if ( versions.count >= 3 ) {
            mBugfix = (int)[versions[2] integerValue];
        }
        
        *out_major = mMajor;
        *out_minor = mMinor;
        *out_bugfix = mBugfix;
    }
}


const char* _xte_platform_sys(void)
{
    return "MacOS X";
}



