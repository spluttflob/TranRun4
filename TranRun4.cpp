//*************************************************************************************
//  TranRun4.cpp
//      This is the "main" file for the TranRun 4 scheduler.  It contains the main ()
//      function (the first one called after the program starts up):  if the environ-
//      ment is DOS or a 'synthetic DOS' like QuickWin, we just use the regular C
//      function main(); if it's instead a Windows GUI application, the Windows appli-
//      cation makes a call to SchedulerMain() from within its InitInstance() func-
//      tion.  The user must include this CPP file in his/her/its project.
//
//  Version
//       1-03-95  JRR  Original version
//       1-26-95  JRR  All environment-dependent code from CTL_EXEC moved in here to
//                     permit the rest of the scheduler to be environment independent
//       5-21-95  JRR  Changed from CTL_EXEC scheduler to TranRun3 project names
//      12-21-96  JRR  Evolved into TranRun4 this evening  
//*************************************************************************************

#include <stdlib.h>
#include <conio.h>              //  Screen I/O functions - gotoxy(), etc.
#include <stdarg.h>             //  Variable argument lists 

#include <TranRun4.hpp>


//-------------------------------------------------------------------------------------
//  Global Data
//      This stuff is global in the project and is declared extern in a HPP file.

CMaster* TheMaster;             //  Global pointers to master scheduler and timer
CRealTimer* TheTimer;           //  objects
CProcess* MainProcess;          //  Pointer to the main process object
boolean TR_DoProfile = FALSE;   //  Set to TRUE when execution profiling is to be done


//-------------------------------------------------------------------------------------
//  Function:  The Main Routine, by whatever name
//      This is the main function which is called when the program first runs.   It
//      creates the master scheduler and then calls UserMain(), which is what the user
//      has written instead of the normal C function main().  If this is a DOS app.,
//      its name is main() so it runs at startup time.  If we're making a Windows
//      application (including Borland EasyWin), this function is named WinMain and it
//      gets a handle to the program instance (which is needed for a Matlab interface).
//      The idea of all this is that the user uses one UserMain() for any environment.

#if defined (TR_MATLAB)
    #include <windows.h>
    int pascal WinMain (HANDLE hInstance, HANDLE hPrevInst, LPSTR CPar, int CShow)
        {
        _InitEasyWin ();
#else
    int main (int argc, char* argv[])
        {
#endif
        int ReturnValue = 0;                        //  Value returned to DOS

        //  Create objects for the master scheduler, timer, and one process
        TheMaster = new CMaster ();
        MainProcess = TheMaster->AddProcess ("Main Process");
        TheTimer = new CRealTimer ();

        ReturnValue = UserMain (argc, argv);        //  Run and save return value

        delete (TheMaster);                         //  Close down those objects
        delete (TheTimer);                          //  (this also closes files)

        return (ReturnValue);
        }


//-------------------------------------------------------------------------------------
//  Function: TR_Message
//      This function echos a line of warning text to the user.  It is meant to be
//      called when a pretty serious but not fatal error happens.  If the problem is
//      really horrible and requires a hasty program exit, use TR_Exit().
//          Note:  If you add a new operator interface or real-time environment,
//          please add some code to this function so it will work for your interface.

void TR_Message (const char *aFormat, ...)
    {
    va_list Arguments;

    //  Begin the variable-argument processing stuff
    va_start (Arguments, aFormat);

    //  If using standard C output, use stderr for complaints
    #if defined (STD_C)
        vfprintf (stderr, aFormat, Arguments);
    //  If using the DOS operator window, write the message at top of screen
    #else
        gotoxy (1, 1);
        vfprintf (stderr, aFormat, Arguments);
    #endif

    //  Clean up after the variable-argument processing
    va_end (Arguments);
    }


