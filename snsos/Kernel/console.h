#ifndef _H_CONSOLE_
#define _H_CONSOLE_

void Con_Init(); // initialize console subsystem

// console kernel mode API

void Con_Print(const char *format, ...); // print a formatted string with arguments to the console, TODO: figure out var args shit

#endif