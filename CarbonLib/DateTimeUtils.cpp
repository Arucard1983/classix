//
// DateTimeUtils.cpp
// Classix
//
// Copyright (C) 2013 Félix Cloutier
//
// This file is part of Classix.
//
// Classix is free software: you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later
// version.
//
// Classix is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
// A PARTICULAR PURPOSE. See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along with
// Classix. If not, see http://www.gnu.org/licenses/.
//

#include <CoreFoundation/CoreFoundation.h>
#include "Prototypes.h"
#include "CarbonLib.h"
#include "NotImplementedException.h"

//Time constants for Mac1984
constexpr CFAbsoluteTime kMacClassicEpochOffset = 3061152000.0;

struct MacDateTimeRec {int16_t year; int16_t month; int16_t day; int16_t hour; int16_t minute; int16_t second; int16_t dayOfWeek;};

static uint32_t GetCurrentMacSeconds(){
CFAbsoluteTime time = CFAbsoluteTimeGetCurrent();
CFAbsoluteTime since1904 = time + kMacClassicEpochOffset;
return static_cast<uint32_t>(since1904);}

void CarbonLib_DateToSeconds(CarbonLib::Globals* globals, MachineState* state)
{
	// state->r3:pointer for origin DateTimeRec struct
        // state->r4: pointer for target uint32_t
        auto* inDate = globals->allocator.ToPointer(state->r3);
        CFGregorianDate gregDate;
        gregDate.year   = inDate->year;
        gregDate.month  = inDate->month;
        gregDate.day    = inDate->day;
        gregDate.hour   = inDate->hour;
        gregDate.minute = inDate->minute;
        gregDate.second = inDate->second;
        CFTimeZoneRef tz = CFTimeZoneCopyDefault();
        CFAbsoluteTime cfSeconds = CFGregorianDateGetAbsoluteTime(gregDate, tz);
        uint32_t macSeconds = static_cast<uint32_t>(cfSeconds + kMacClassicEpochOffset);
        *globals->allocator.ToPointer<Common::UInt32>(state->r4) = Common::UInt32(macSeconds);
        if (tz) CFRelease(tz);
}

void CarbonLib_GetDateTime(CarbonLib::Globals* globals, MachineState* state)
{
        uint32_t seconds = GetCurrentMacSeconds();
        *globals->allocator.ToPointer<Common::UInt32>(state->r3) = Common::UInt32(seconds);
        state->r3 = 0;
}

void CarbonLib_GetTime(CarbonLib::Globals* globals, MachineState* state)
{
	// Use Core Graphics to retrieve the host clock
        CFTimeZoneRef tz = CFTimeZoneCopyDefault();
        CFAbsoluteTime now = CFAbsoluteTimeGetCurrent();
        CFGregorianDate gregDate = CFAbsoluteTimeGetGregorianDate(now, tz);
        auto* outDate = globals->allocator.ToPointer<MacDateTimeRec>(state->r3);
        outDate->year   = Common::Int16(static_cast<int16_t>(gregDate.year));
        outDate->month  = Common::Int16(static_cast<int16_t>(gregDate.month));
        outDate->day    = Common::Int16(static_cast<int16_t>(gregDate.day));
        outDate->hour   = Common::Int16(static_cast<int16_t>(gregDate.hour));
        outDate->minute = Common::Int16(static_cast<int16_t>(gregDate.minute));
        outDate->second = Common::Int16(static_cast<int16_t>(gregDate.second));

        //Notice that on Classic Mac OS, Sunday is day 1 of the week!
        CFAbsoluteTime weekTime = CFAbsoluteTimeGetCurrent();
        int32_t dayOfWeek = CFAbsoluteTimeGetDayOfWeek(weekTime, tz);
        outDate->dayOfWeek = Common::Int16(static_cast<int16_t>(dayOfWeek));
        if (tz) CFRelease(tz);
}

void CarbonLib_InitDateCache(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0; // Dummy Clearance.
}

void CarbonLib_IUDatePString(CarbonLib::Globals* globals, MachineState* state)
{
    // Foward function for date
    CarbonLib_IUDateString(globals, state);
}

void CarbonLib_IUDateString(CarbonLib::Globals* globals, MachineState* state)
{
    // r3: uint32_t counting Mac OS seconds (since 1904)
    // r4: int16_t longFlag (0 = short, 1 = long, 2 = abbrev)
    // r5: Pointer to the target Str255 buffer (Pascal String)
    
    uint32_t macSeconds = state->r3;
    int16_t longFlag = static_cast<int16_t>(state->r4);
    
    // Conver to the Core Foundation absolute time (2001)
    CFAbsoluteTime cfTime = static_cast<CFAbsoluteTime>(macSeconds) - kMacClassicEpochOffset;
    
    // Mapping the selectors of Mac OS Classic to the Core Foundation styles
    CFDateFormatterStyle dateStyle;
    switch (longFlag) {
        case 0:  dateStyle = kCFDateFormatterShortStyle;  break; // "shortDate"
        case 1:  dateStyle = kCFDateFormatterLongStyle;   break; // "longDate"
        case 2:  dateStyle = kCFDateFormatterMediumStyle; break; // "abbrevDate"
        default: dateStyle = kCFDateFormatterShortStyle;  break;
    }
    
    // Create the string style respecting the locale defined by the host (Linux)
    CFDateFormatterRef formatter = CFDateFormatterCreate(
        kCFAllocatorDefault, 
        CFLocaleCopyCurrent(), 
        dateStyle, 
        kCFDateFormatterNoStyle // No hour
    );

   CFStringRef cfDateString = CFDateFormatterCreateStringWithAbsoluteTime(kCFAllocatorDefault, formatter, cfTime);
    
    // Convert the CFString created to classi Pascal String expected by the application
    auto* outPascalStr = globals->allocator.ToPointer<uint8_t>(state->r5);
    
    char cStr[256];
    if (CFStringGetCString(cfDateString, cStr, sizeof(cStr), kCFStringEncodingMacRoman)) {
        size_t len = strlen(cStr);
        if (len > 255) len = 255;
        
        outPascalStr[0] = static_cast<uint8_t>(len);  // First byte store the size
        std::memcpy(&outPascalStr[1], cStr, len);      // Follow the characters
    } else {
        outPascalStr[0] = 0; // Empty string in case of encoding failure
    }
    
    // Clear Core Foundation objects
    if (cfDateString) CFRelease(cfDateString);
    if (formatter) CFRelease(formatter);
}

void CarbonLib_IULDateString(CarbonLib::Globals* globals, MachineState* state)
{
    // state->r3: Pointer to LongDateTime (64-bit int) on emulated memory
    // state->r4: longFlag
    // state->r5: result buffer (Str255)
    
    // 1. Read the 64 bits value from memory using the Big-Endian macro
    auto* longTimePtr = globals->allocator.ToPointer<Common::SInt64>(state->r3);
    int64_t fullSeconds = *longTimePtr; //  BigEndianInt explcit conversion manage the swap
    
    // 2. Since the current year is 2026, the value fill perfectly on an uint32_t.
    // Managing the original registers
    uint32_t orig_r3 = state->r3;
    uint32_t orig_r4 = state->r4;
    uint32_t orig_r5 = state->r5;
    
    // 3. Apply the converted values to simulate a 32 bits call
    state->r3 = static_cast<uint32_t>(fullSeconds); // Pass the time by value
    state->r4 = orig_r4;                            // Maintain the longFlag
    state->r5 = orig_r5;                            // Mantain the target flag
    
    // 4. Apply the 32-bit logic as defined
    CarbonLib_IUDateString(globals, state);
    
    // 5. Restore the original registers, by emulator courtesy.
    state->r3 = orig_r3;
}

void CarbonLib_IULTimeString(CarbonLib::Globals* globals, MachineState* state)
{
    // state->r3: Pointer to LongDateTime (64-bit int)
    // state->r4: wantSeconds
    // state->r5: result buffer (Str255)
    
    auto* longTimePtr = globals->allocator.ToPointer<Common::SInt64>(state->r3);
    int64_t fullSeconds = *longTimePtr;
    
    uint32_t orig_r3 = state->r3;
    uint32_t orig_r4 = state->r4;
    uint32_t orig_r5 = state->r5;
    
    state->r3 = static_cast<uint32_t>(fullSeconds);
    state->r4 = orig_r4;
    state->r5 = orig_r5;
    
    CarbonLib_IUTimeString(globals, state);
    
    state->r3 = orig_r3;
}

void CarbonLib_IUTimePString(CarbonLib::Globals* globals, MachineState* state)
{
	// Forward function for time 
        CarbonLib_IUTimeString(globals, state);
}

void CarbonLib_IUTimeString(CarbonLib::Globals* globals, MachineState* state)
{
    // r3: uint32_t containing the number of seconds on Mac OS (since 1904)
    // r4: Boolean (wantSeconds) - If the seconds should or not be placed on string
    // r5: Pointer to the target buffer (Str255 / Pascal String)
    
    uint32_t macSeconds = state->r3;
    bool wantSeconds = (state->r4 != 0);
    
    // Convert back to Core Foundation epoch
    CFAbsoluteTime cfTime = static_cast<CFAbsoluteTime>(macSeconds) - kMacClassicEpochOffset;
    
    // Create a hur/date Core Foundation format
    CFDateFormatterRef formatter = CFDateFormatterCreate(
        kCFAllocatorDefault, 
        CFLocaleCopyCurrent(), 
        kCFDateFormatterNoStyle, // No data
        wantSeconds ? kCFDateFormatterLongStyle : kCFDateFormatterShortStyle // Hour format
    );

   CFStringRef cfTimeString = CFDateFormatterCreateStringWithAbsoluteTime(kCFAllocatorDefault, formatter, cfTime);
    
    // Convert CFString to a tradiitonal Pascal String from Macintosh (Str255)
    auto* outPascalStr = globals->allocator.ToPointer<uint8_t>(state->r5);
    
    // Obtain the local parameters in C string format first
    char cStr[64];
    if (CFStringGetCString(cfTimeString, cStr, sizeof(cStr), kCFStringEncodingMacRoman)) {
        size_t len = strlen(cStr);
        if (len > 255) len = 255;
        
        outPascalStr[0] = static_cast<uint8_t>(len); // First byte is size (Pascal style)
        std::memcpy(&outPascalStr[1], cStr, len);    // Copy the Characters
    } else {
        outPascalStr[0] = 0; // Empty string in case ofcatastrophic failure
    }
    
    // Clieaning Core Foundation memory
    if (cfTimeString) CFRelease(cfTimeString);
    if (formatter) CFRelease(formatter);
    
    state->r3 = 0;
}

void CarbonLib_LongDateToSeconds(CarbonLib::Globals* globals, MachineState* state)
{
    // r3: Pointer to LongDateRec host
    // r4: Pointer to LongDateTime (64-bit int host)
    
    uint32_t orig_r3 = state->r3;
    uint32_t orig_r4 = state->r4;
    
    // Create a temporary buffer to obatin the 32-bit result
    // Note: Since LongDateRec share the initial DateTimeRec allignement , 
    // the 32 bits functtion can read the base field perfectly.
    uint32_t temporaryDest32 = 0;
    
    // Simualte the target as local variable before handle to 64 bits
    // To simplify, just call the internal logical of the 32-bit function
    CarbonLib_DateToSeconds(globals, state);
    
    // Recover the 32-bit results returned by the original target(r4)
    uint32_t result32 = *globals->allocator.ToPointer<Common::UInt32>(orig_r4);
    
    // Now write the 64-bit results using the corrrect pointer eequest by the appplication
    auto* dest64 = globals->allocator.ToPointer<Common::SInt64>(orig_r4);
    *dest64 = static_cast<int64_t>(result32);

    state->r3 = 0;
}

void CarbonLib_LongSecondsToDate(CarbonLib::Globals* globals, MachineState* state)
{
    // r3: Pointer to LongDateTime (64-bit int de origem)
    // r4: Pointer to target LongDateRec
    
    // 1. Read 64 bits using Big-Endian
    auto* longTimePtr = globals->allocator.ToPointer<Common::SInt64>(state->r3);
    int64_t fullSeconds = *longTimePtr;
    
    // 2. managing register
    uint32_t orig_r3 = state->r3;
    uint32_t orig_r4 = state->r4;
    
    // 3. Prepare the 32 bits function (SecondsToDate)
    state->r3 = static_cast<uint32_t>(fullSeconds); // Pass the value r3
    state->r4 = orig_r4;                            // Maintain the pointer to the strucre
    
    CarbonLib_SecondsToDate(globals, state);
    
    // 4. Restore the original pointer
    state->r3 = orig_r3;
}

void CarbonLib_ReadDateTime(CarbonLib::Globals* globals, MachineState* state)
{
	uint32_t seconds = GetCurrentMacSeconds();
        *globals->allocator.ToPointer<Common::UInt32>(state->r3) = Common::UInt32(seconds);
        state->r3 = 0;
}

void CarbonLib_SecondsToDate(CarbonLib::Globals* globals, MachineState* state)
{
	//r3 is the number of seconds
        //r4 is the pointer to Datetimerec Struct of destination
        uint32_t seconds = state->r3;
        CFAbsoluteTime cfSeconds = static_cast(seconds) - kMacClassicEpochOffset;
        CFTimeZoneRef tz = CFTimeZoneCopyDefault();
        CFGregorianDate gregDate = CFAbsoluteTimeGetGregorianDate(cfSeconds, tz);
        auto* outDate = globals->allocator.ToPointer<MacDateTimeRec>(state->r4);
        outDate->year   = Common::Int16(static_cast<int16_t>(gregDate.year));
        outDate->month  = Common::Int16(static_cast<int16_t>(gregDate.month));
        outDate->day    = Common::Int16(static_cast<int16_t>(gregDate.day));
        outDate->hour   = Common::Int16(static_cast<int16_t>(gregDate.hour));
        outDate->minute = Common::Int16(static_cast<int16_t>(gregDate.minute));
        outDate->second = Common::Int16(static_cast<int16_t>(gregDate.second));
        int32_t dayOfWeek = CFAbsoluteTimeGetDayOfWeek(cfSeconds, tz);
        outDate->dayOfWeek = Common::Int16(static_cast<int16_t>(dayOfWeek));
        if (tz) CFRelease(tz);
}

void CarbonLib_SetDateTime(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = -50; //Guest programs cannot change host clock!
}

void CarbonLib_SetTime(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = -50; //Guest programs cannot change host clock!
}

void CarbonLib_StringToDate(CarbonLib::Globals* globals, MachineState* state)
{
	constexpr uint32_t fatalDateTimeStringErr = 0x80000000;
        state->r3 = fatalDateTimeStringErr;
}

void CarbonLib_StringToTime(CarbonLib::Globals* globals, MachineState* state)
{
	constexpr uint32_t fatalDateTimeStringErr = 0x80000000;
        state->r3 = fatalDateTimeStringErr;
}

void CarbonLib_ToggleDate(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = -50; //Exclusive Finder operation, that is not present on Darling!
}

void CarbonLib_ValidDate(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 1; //Return true
}

