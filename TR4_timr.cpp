//*************************************************************************************
//  TR4_TIMR.CPP
//      This is the implementation file for the real-time timer object.  This object
//      is called by the master scheduler to keep track of real time.  Only one timer
//      object is to be created.  It uses one of several modes, each defined with a
//      preprocessor variable named TR_TIME_*, from the real-time mode header file. 
//      Windows 3 Timer:  If using TR_TIME_FREE under Windows 3.1, the Virtual Timer 
//      Device will be utilized, and wintimer.obj (from wintimer.asm) must be present
//
//  Copyright (c) 1994-1996, D.M.Auslander, J.C.Jones, M.Lemkin, and J.R.Ridgely
//      May be used and distributed for any non-commercial purposes as long as this
//      copyright notice is included.
//      11-01-94  DMA    Original program 
//      12-15-94  JR     Spun off as separate source file
//      12-15-94  JJ/JR  Added mode CX_TIME_FREE, which uses PC free running timer
//       1-26-95  JR     Removed interrupt-driven updates of free running timer
//       2-02-95  JR     Moved interrupt handling stuff to CTL_INTR.CPP
//       5-20-95  JR     Changed to the TranRun project names
//       3-16-96  JR     Added Windows 3.1 virtual timer mode; it needs WINTIMER.ASM
//       8-18-96  JR     Set up Windows NT timekeeping with QueryPerformanceCounter()
//      12-21-96  JRR    Ported over to TranRun4
//       1-14-96  JRR    Added Prevent/AllowPreemption(), took out TR_THREAD_TIMER
//*************************************************************************************

//  If this is a Windows 32-bit (95/NT) console application, we'll need windows.h
#if defined (__WIN32__)
        #include <windows.h>        //  For Win32, we use QueryPerformanceCounter()
        #include <winbase.h>
#endif
#if !defined (__WIN32__)
        #include <conio.h>          //  Included for prototypes of _outp() and _inp()
        #include <dos.h>            //  Has _enable(), _disable(), _dos_setvect(), etc.
#endif

#include <float.h>
#include <stdlib.h>
#include <TranRun4.hpp>
#include <string.h>


//  The LARGE_INTEGER data structure is treated oddly by this compiler - we don't know
//  what the names for its parts are going to be.  So use funny macros to get parts.
//  Note that this construction may not be portable to other-than-x86 computers.
#define  GetLowPart(X)   (*(DWORD*)&(X))
#define  GetHighPart(X)  (*((LONG*)&(X)+1))


//-------------------------------------------------------------------------------------
//  INFORMATION ABOUT FREE-RUNNING TIMER IN IBM-PC, used by TR_TIME_FREE mode in DOS
//      The following are the port addresses for the timers and the control register.
//      The port number selects the proper address (a1 and a0 on 8253 chip) and read
//      and write flags depending on if written or read, i.e.
//          COMMAND                 RD'     WR'     A1      A0
//           inp(TIMER0)            0       1       0       0
//           inp(TIMER1)            0       1       0       1
//           inp(TIMER2)            0       1       1       0
//           outp(TIMER0)           1       0       0       0
//           outp(TIMER1)           1       0       0       1
//           outp(TIMER2)           1       0       1       0
//           outp(TIMER_CTL)        1       0       1       1
//
//  To set up each timer you must write to the control word register:
//      D7      D6      D5      D4      D3      D2      D1      D0
//      SC1     SC0     RL1     RL0     M2      M1      M0      BCD
//
//  Where:                 SC1    SC0                                 RL1     RL0
//      Select Timer 0     0      0         Counter Latch for Read    0       0
//      Select Timer 1     0      1         Read/Load MSB only        1       0
//      Select Timer 2     1      0         Read/Load LSB only        0       1
//                                          Read/Load LSB, THEN MSB   1       1
//      M2, M1, M0 control mode:  Mode 0: Interrupt on terminal count
//                                Mode 1: Programmable one-shot
//                                Mode 2: Rate generator
//                                Mode 3: Square wave generator (Counts by twos)
//                                Mode 4: Software triggered strobe
//                                Mode 5: Hardware triggered strobe
//      BCD: 0 - Binary counter (16 bits); 1 - Binary coded decimal counter (4 decades)
//
//  Notes: - In order to set the gate high for timer 2 (sound timer), which enables
//           counting, use the command: _outp (0x61, _inp(0x61) | (0x01));
//         - All counters count down to zero.
//         - More info is in Intel's Microprocessor and Peripheral Handbook Vol. 2

//  These #definitions are used by TR_TIME_FREE mode and the interrupt service routine
#if defined (TR_TIME_FREE) && !defined (_Windows)
    static unsigned short LastTime = 0;        //  Time read in previous GetTimeNow()
#endif
#if defined (TR_TIME_FREE) && defined (__WIN32__)
    LARGE_INTEGER StartPerfTicks;              //  Hold ticks at start for Win-95 or NT
#endif
#if defined (TR_TIME_FREE) && defined (_Windows) && !defined (__WIN32__)
    static unsigned short VTD_Addr_High;       //  These numbers store address of the
    static unsigned short VTD_Addr_Low;        //  Windows virtual timer device
    const real_time SEC_PER_OV = 3599.597124;  //  Seconds per 32-bit clock overflow
    unsigned long TARRAY[2];                   //  Array holds time data from Windows
    extern "C" void GETAPI (void);             //  Functions ask Windows where timer
    extern "C" void GETTICKS (void);           //  device is, get number of time ticks
#endif
#if defined (TR_TIME_FREE) && !defined (__WIN32__)
    const real_time CK_FREQ = 1193180.0;       //  Frequency of PC clock in Hz
#endif
#if defined (TR_TIME_FREE) && defined (TR_THREAD_MULTI)
    const real_time MAX_PER = 0.0549;       //  Maximum possible timer period, sec.
#endif
const unsigned TIMER0 = 0x40;               //  Timer 0 port
const unsigned TIMER1 = 0x41;               //  Timer 1 port
const unsigned TIMER2 = 0x42;               //  Timer 2 port
const unsigned TIMER_CTL = 0x43;            //  8253 Timer control port
const unsigned TMREOI = 0x60;               //  Timer-specific EOI signal
const unsigned INTCTL0 = 0x20;              //  Port 0 (interrupt control) of 8259
const unsigned CK_VECTOR = 8;               //  Interrupt vector number of PC timer

//  Pick a timer, #0 which is used for timer interrupts, or #2 which runs the speaker
//  ...current version is using Timer 0 because Timer 2 causes problems on some Intel
//  486 or Pentium machines by getting mixed up with the typematic timer (we think)
#if defined TR_THREAD_MULTI
    #define TR_USE_TIMER_2
#else
    #define TR_USE_TIMER_0
#endif

#if defined (TR_THREAD_MULTI)
    //  This buffer holds FPU's state, which is saved whenever reentrance occurs.
    //  It acts like a second stack just for FPU saves, but it's in the data segment
    static far char FPU_Buffer[94 * MAX_REENTER];

    //  This counter is used to prevent pre-emption of protected sections of code
    static int PreemptionCounter = 0;
#endif

//  If building a Windows app, use virtual timer device, NOT timers 0 or 2
#if defined (_Windows)
    #undef TR_USE_TIMER_0
    #undef TR_USE_TIMER_2
#endif


//-------------------------------------------------------------------------------------
//  File-scope global data
//      In order to allow efficient communication between the ISR and background, it
//      is necessary to use these global objects, even if it is lousy C++ style.

const unsigned Delta_Limit = 0xB000;        //  Clock overflow error checking limit

#if defined (TR_THREAD_MULTI)
    static int TR_ReentryDepth = 0;         //  This int measures ISR reentrance depth
    static int TR_MaxDepth = 0;             //  Maximum depth which has been reached
    static boolean TR_ExitFlag = FALSE;     //  Prevents calling TR_Exit() twice
    static long TR_ISR_Runs = 0;            //  Number of times the ISR has run
#endif

#if defined (TR_TIME_FREE) && defined (TR_USE_TIMER_0)
    static unsigned IntRateDivisor = 0xFFFF;    //  Interrupt clock rate divisor
#endif

#if defined (TR_TIME_INT) || defined (TR_THREAD_MULTI) || defined (TR_THREAD_TIMER)
    unsigned long TR_TimerTicks;            //  Number of times timer ISR has run
#endif


//-------------------------------------------------------------------------------------
//  Interrupt Handler: TimerISR
//      This is the timer interrupt service routine which is called by the timer hard-
//      ware once every (whatever) milliseconds.
//
//      The TR_THREAD_MULTI multithreading version of the function runs a re-entrant
//      scheduler.  The depth of re-entrance (how many times the function has been
//      entered without its running to completion and leaving) is checked, and if that
//      depth becomes too great (which can cause stack trouble) the program refuses to
//      call the scheduler again and causes an error.  TheMaster->RunForeground() will
//      run timer interrupt and sample time tasks.
//
//      The TR_THREAD_TIMER version, which runs only timer-interrupt tasks in a fore-
//      ground thread, also uses multithreading logic, but TheMaster->RunForeground()
//      will only call timer_interrupt tasks in this version.  These tasks are not to
//      be considered re-entrant; they should complete quickly in under one tick.
//
//      There is also a TR_TIME_INT version which is used with single-threading sched-
//      ulers that run all tasks in the background thread but use the interrupts to
//      measure real time.

#if defined (TR_THREAD_MULTI) || defined (TR_THREAD_TIMER)
    ISR_FUNCTIONDEF (TimerISR)
        {
        //  Save the state of the floating point unit to the FPU "stack" in the data
        //  segment.  Calling fsave [DI] causes all the FPU's internal registers to be
        //  saved to a memory location pointed to by DI and referenced by the segment
        //  register DS.  Therefore, before calling fsave we must set DS to the data
        //  segment of the FPU "stack" space, as defining FPU_Buffer as a file-scope
        //  global has put it in a data segment which may be different from the one
        //  we're in when this ISR runs.  DI gets the offset into that data segment at
        //  which the FPU state will go
        unsigned Save_DI = FP_OFF (FPU_Buffer) + (94 * TR_ReentryDepth);
        unsigned Save_DS = FP_SEG (FPU_Buffer);

        asm     push    DS              //  Push the data segment in which the ISR is
        _DI = Save_DI;                  //  intended by the compiler to run; then get
        _DS = Save_DS;                  //  DI and DS from the local, automatic vars.
        asm     fsave   [DI]            //  in which they were saved and use them to
        asm     fwait                   //  save the FPU state.  Then pop DS because
        asm     pop     DS              //  it's needed for dealing with other data

        TR_ISR_Runs++;                      //  Re-entrance:  If this interrupt has
        TR_ReentryDepth++;                  //  interrupted another instance of itself
        if (TR_ReentryDepth > TR_MaxDepth)  //  increment the maximum reentry level.
            TR_MaxDepth = TR_ReentryDepth;  //  Also save the maximum reentrance depth

        //  If the re-entrance depth is too great, refuse to run foreground tasks and
        //  cause error exit; also don't decrement ReentryDepth, so it won't run the
        //  foreground again.  ExitFlag == TRUE means depth was too great, so exit
        if (TR_ExitFlag == TRUE)
            {
            outp (INTCTL0, TMREOI);
            }
        else if (TR_ReentryDepth > MAX_REENTER)
            {
            TR_Exit ("Re-entry limit exceeded!  Depth is %d", TR_ReentryDepth);
            TR_ExitFlag = TRUE;
            outp (INTCTL0, TMREOI);
            }
        else
            {
            //  If we're using the free running timer to keep time, call it now to make
            //  sure that it keeps updated and doesn't have overflow problems.  If
            //  we're using interrupts to keep time, increment the time now.
            #endif
            #if (defined (TR_THREAD_MULTI) || defined (TR_THREAD_TIMER))\
                                           && defined (TR_TIME_FREE)
                GetTimeNowUnprotected ();
            #elif (defined (TR_THREAD_MULTI) || defined (TR_THREAD_TIMER))\
                                             && defined (TR_TIME_INT)
                TR_TimerTicks++;            //  Increment time counter
            #endif
            #if defined (TR_THREAD_MULTI) || defined (TR_THREAD_TIMER)
            outp (INTCTL0, TMREOI);         //  Re-enable hardware timer alarm

            //  If not in a protected section, run interrupt driven tasks now
            if (PreemptionCounter == 0)  TheMaster->RunForeground ();
            }
        TR_ReentryDepth--;                  //  We're exiting one level of re-entry

        asm     push    DS
        _DI = Save_DI;                  //  Get the data segment and offset at which
        _DS = Save_DS;                  //  we saved the FPU state; use 'em to restore
        asm     frstor  [DI]            //  the FPU state.  Wait until FPU is fully
        asm     fwait                   //  restored before going back to background
        asm     pop     DS
        }
#endif

//  This version of the function is used only to keep time, and all it does is
//  increment the global timer.  Re-entrance of this function is not permitted.
//  The macro definition of the function is for compatibility between compilers
//  see file TR3_COMP.HPP for the definitions of this macro
#if defined (TR_TIME_INT) && defined (TR_THREAD_SINGLE)
    ISR_FUNCTIONDEF (TimerISR)
        {
        TR_TimerTicks++;                    //  Increment time counter
        outp (INTCTL0, TMREOI);             //  Send signal to interrupt controller
        }
#endif


//=====================================================================================
//  Class: CRealTimer
//      This class implements a real-time timer which keeps track of "real" time in
//      whatever way corresponds to the operational mode of the program.  Real time
//      modes include
//      - Simulated time, in which time is incremented by the master class when a scan
//        has been completed
//      - Simulated-real-time, in which timer interrupts run this clock but not the
//        actual calling of task functions
//      - Full interrupt real time, where timer interrupts run almost all timed stuff
//=====================================================================================

//-------------------------------------------------------------------------------------
//  Constructor: CRealTimer
//      This constructor creates a new Real Time Timer object.

CRealTimer::CRealTimer (void)
    {
    TheTime = (real_time)0.0;           //  The timer hasn't been set up, so all
    DeltaTime = (real_time)0.0;         //  times returned will be zeros
    DumpFile = NULL;                    //  Default file object for dumps is stdout 
    InterObj = NULL;                    //  Default timer-interrupt object: null 

    #if defined (TR_TIME_INT) || defined (TR_THREAD_MULTI) || defined (TR_THREAD_TIMER)
        //  Create an object to hold that timer interrupt configuration
        InterObj = new CInterruptObj (CK_VECTOR, TimerISR);
    #endif

    #if defined (TR_TIME_INT)
        //  Set the number-of-interrupts counter to zero
        TR_TimerTicks = 0L;
    #endif
    }


//-------------------------------------------------------------------------------------
//  Destructor: CRealTimer
//      This destructor frees up some memory which was used by the timer object.  

CRealTimer::~CRealTimer (void)
    {
    if (InterObj != NULL)
        delete (InterObj);
    if (DumpFile != NULL)
        delete (DumpFile);
    }


//-------------------------------------------------------------------------------------
//  Function: Setup
//      This function initializes the timer so it can actually DO something.

void CRealTimer::Setup (real_time DeltaT)
    {
    DeltaTime = DeltaT;                     //  Save parameters into member data fields

    //  If an interrupt handler is used, change rate of hardware interrupt timer
    #if defined (TR_TIME_INT) || defined (TR_THREAD_MULTI) || defined (TR_THREAD_TIMER)
        InterObj->SetAlarm (DeltaTime);
    #endif
    }


//-------------------------------------------------------------------------------------
//  Function: Go
//      This function turns the timer "on" and resets the time count to 0.  It also
//      initializes the software or hardware of the timer, depending on the mode.

void CRealTimer::Go (void)
    {
    TheTime = (real_time)0.0;

    //  If using _ftime(), save starting time as a zero reference for real-time time
    #if defined (TR_TIME_FTIME)
        ftime (&Start_ftime);

    //  If compiling a Windows 3.1 application which uses a free-running timer, we'll 
    //  use Windows's built-in Virtual Timer Device.  It's initialized here 
    #elif defined (TR_TIME_FREE) && defined (_Windows) && !defined (__WIN32__)
        GETAPI ();                          //  Call assembly function to get address
        VTD_Addr_High = _DX;                //  of virtual timer, then store it in 
        VTD_Addr_Low = _AX;                 //  a pair of 16-bit numbers 

        _DX = VTD_Addr_High;                //  Restore addresses in case they've been
        _AX = VTD_Addr_Low;                 //  messed up; then get the number of 
        GETTICKS ();                        //  timer ticks since Windows was started

        //  Compute the starting-up time and save it for calculating intervals later
        TheTimer->StartTime =
                        (real_time)((TARRAY[1] * SEC_PER_OV) + (TARRAY[0] / CK_FREQ));

    //  This is for Windows NT or 95.  It uses the NT "Performance Counter" device
    //  We use an array of longs because the documentation for LARGE_INTEGER is wrong
    //  and ignore the suspicious pointer warnings...it works, we'll fix it later
    #elif defined (TR_TIME_FREE) && defined (_Windows) && defined (__WIN32__)
        //  First find the frequency at which the performance counter runs
        QueryPerformanceFrequency (&StartPerfTicks);
        PerfFreq = (double)((GetHighPart (StartPerfTicks) * 4294967296.0)
                            + GetLowPart (StartPerfTicks));

        //  Then get the numbers in the counter as we start the program up 
        QueryPerformanceCounter (&StartPerfTicks);

    //  If using free-running timer #0, we must set up the timer hardware here.  This
    //  block is for when we're not using interrupt scheduling
    #elif defined (TR_TIME_FREE) && defined (TR_USE_TIMER_0) \
          && !(defined (TR_THREAD_TIMER) || defined (TR_THREAD_MULTI))
        //  Set count down mode, r/w mode, and binary mode for timer 0
        outp (TIMER_CTL, 0x34);

        //  Send LSB then MSB of counter value. 0xFFFF sets the 8253 to the maximum
        //  rollover of 65535 counts corresponding to 54.9 ms.  Then latch current 
        //  timer value and read it in so it will serve as first 'previous' reading
        outp (TIMER0, 0xFF);
        outp (TIMER0, 0xFF);
        outp (TIMER_CTL, 0x04);
        LastTime = (((unsigned short) inp (TIMER0)) | 
                    (((unsigned short) inp (TIMER0)) << 8));
    #endif

    //  This block runs when we are using interrupt scheduling, and the interrupts are
    //  driven from the same timer as the "free" running timer so we adjust its period
    #if defined (TR_TIME_FREE) && defined (TR_USE_TIMER_0) \
          && (defined (TR_THREAD_TIMER) || defined (TR_THREAD_MULTI))
        //  Make sure the time interval given is valid; if so, compute number of ticks
        if ((DeltaTime > 0.0) && (DeltaTime < MAX_PER))
            IntRateDivisor = (unsigned)((CK_FREQ * DeltaTime) + 0.5);

        //  Set count down mode, r/w mode, and binary mode for timer 0
        outp (TIMER_CTL, 0x34);

        //  Send LSB then MSB of counter value... 0xFFFF sets the 8253 to the maximum
        //  rollover of 65535 counts.  This corresponds to 54.9 ms.  Also save LastTime
        outp (TIMER0, (int)(IntRateDivisor & 0xFF));
        outp (TIMER0, (int)((IntRateDivisor >> 8) & 0xFF));
        outp (TIMER_CTL, 0x04);
        LastTime = inp (TIMER0) | (inp (TIMER0) << 8);
    #endif

    //  If using free-running timer #2, we must set up the timer hardware here
    #if defined (TR_TIME_FREE) && defined (TR_USE_TIMER_2)
        //  Set count down mode, r/w mode, and binary mode for timer 2
        outp (TIMER_CTL, 0xB4);

        //  Send LSB then MSB of counter value... 0xFFFF sets the 8253 to the maximum
        //  rollover of 65535 counts.  This corresponds to 54.9 ms
        outp (TIMER2, 0xFF);
        outp (TIMER2, 0xFF);

        //  Enable counting by bringing gate to timer2 high (on 8253 chip)
        outp (0x61, inp (0x61) | 0x01);
    #endif

    //  If using interrupts to keep time and/or run the foreground, install the ISR now
    #if defined (TR_TIME_INT) || defined (TR_THREAD_MULTI) || defined (TR_THREAD_TIMER)
        //  Install the clock interrupt service routine which calls the scheduler
        InterObj->InstallISR ();
    #endif

    //  If the free-running timer mode hasn't done so, set interrupt timer frequency
    #if defined (TR_TIME_INT)
        if (InterObj->SetAlarm (DeltaTime) == FALSE)
            TR_Exit ("Unable to set clock interrupt period to given rate");
    #endif
    }


//-------------------------------------------------------------------------------------
//  Function: Stop
//      This function turns the timer "off."  In some modes, this requires nothing.
//      In free-running-timer mode, this means disable the free running timer's gate.
//      In modes which use interrupts, it means removing the ISR.  

void CRealTimer::Stop (void)
    {
    #if defined (TR_TIME_FREE) && defined (TR_USE_TIMER_2)
        outp (0x61, inp (0x61) & (0xFC));       //  Disable gate of free-running timer
    #endif

    //  If using interrupts to keep time, remove the ISR now
    #if defined (TR_TIME_INT) || defined (TR_THREAD_MULTI) || defined (TR_THREAD_TIMER)
        InterObj->RemoveISR ();

        //  If it's a timer interrupt, set clock rate back to the DOS normal speed
        if (InterObj->SetAlarm ((real_time)(-1.0)) == TRUE)
            TR_Exit ("Unable to restore interrupt timer to DOS default");
    #endif
    }


//-------------------------------------------------------------------------------------
//  Function: Increment
//      Here's a function which causes time to advance a little.  Actually it incre-
//      ments the system time.  It's called in simulation mode and by the ISR in an
//      interrupt mode.

void CRealTimer::Increment (void)
    {
    TheTime += DeltaTime;
    }


//-------------------------------------------------------------------------------------
//  Function: DumpStatus
//      This function sends a timer status dump to the file object specified in the
//      timer object's member data.  If no dump file has been set up, it does nothing.
//      Note that TR_MaxDepth is a global item in this file

void CRealTimer::DumpStatus (const char* aFileName)
    {
    FILE *DumpFile;                         //  Handle of the file to which to dump


    if (aFileName == NULL)                  //  If no valid file name was given,
        return;                             //  dump out now

    //  Create a new file object to which to write the dump
    if ((DumpFile = fopen (aFileName, "w")) != NULL)
        {
        fprintf (DumpFile, "Status Dump for Timer Object at time %lg\n",
                 (double) GetTimeNow ());
        #if defined (TR_TIME_SIM)
            fprintf (DumpFile, "Mode:  Simulated Time \n");
        #elif defined (TR_TIME_FREE)
            fprintf (DumpFile, "Mode:  Free-Running Timer \n");
        #elif defined (TR_TIME_INT)
            fprintf (DumpFile, "Mode:  Interrupt Timing \n");
        #elif defined (TR_TIME_FTIME)
            fprintf (DumpFile, "Mode:  ftime() Timing \n");
        #else
            fprintf (DumpFile, "Timing mode unknown\n");
        #endif
        fprintf (DumpFile, "    Time Increment: %lg sec.\n", (double)DeltaTime);
        #if defined (TR_THREAD_MULTI)
            fprintf (DumpFile, "    Maximum Re-entry depth: %d\n", TR_MaxDepth);
        #endif
        #if defined (TR_THREAD_MULTI) || defined (TR_THREAD_TIMER)
        fprintf (DumpFile, "    The ISR ran %d times\n\n", TR_ISR_Runs);
        #endif

        fclose (DumpFile);
        }
    }


//-------------------------------------------------------------------------------------
//  Function: GetTimeString
//      Returns a pointer to the current time in a character string.

const char *CRealTimer::GetTimeString (void)
    {
    static char aBuf[64];                   //  Big enough buffer for any time string 

    sprintf (aBuf, REAL_TIME_FORMAT, GetTimeNow ());
    return (aBuf);
    }


//-------------------------------------------------------------------------------------
//  Function: GetTimeNow
//      This version of the time-getting function is interrupt protected when used in
//      multithreading modes.  If not using multithreading modes, we don't need the
//      interrupt protection, so just use non-interrupt-protected version with a macro

#if defined (TR_THREAD_MULTI) || defined (TR_THREAD_TIMER) || defined (TR_TIME_INT)
real_time GetTimeNow (void)
    {
    DisableInterrupts ();
    real_time TimeRightNow = GetTimeNowUnprotected ();
    EnableInterrupts ();
    return (TimeRightNow);
    }
#endif


//-------------------------------------------------------------------------------------
//  Function: GetTimeNowUnprotected
//      This is a friend function of CTimer which returns the current time as kept by
//      the timer's 'real-time' clock.  It was made a friend and not member function
//      so that it could be called from anywhere in the project with minimal overhead.
//      It's unprotected in that interrupts aren't disabled and enabled around it; the
//      protected version supplied to users is a macro called 'GetTimeNow()'.

real_time GetTimeNowUnprotected (void)
    {
    //  If using ftime() function to get "real" time, get it in a wierd _timeb
    //  structure, subtract the time in the starting time structure, and convert from
    //  _timeb structure (integers containing sec. and ms.) to precision floating point
    #if defined (TR_TIME_FTIME)
        timeb TimeNow;                              //  Current time in funny struct
        int msec;                                   //  Delta-time's milliseconds part

        ftime (&TimeNow);
        msec = (int)TimeNow.millitm - (int)((TheTimer->Start_ftime).millitm);
        if (msec < 0) msec += 1000;
        TheTimer->TheTime = (real_time)msec / (real_time)1000.0;
        TheTimer->TheTime += (real_time)(TimeNow.time - (TheTimer->Start_ftime).time);

        return (TheTimer->TheTime);
    #endif

    //  If using free-running timer under Windows 3.1, ask virtual timer device for
    //  ticks, convert the number to seconds, and subtract starting time
    #if defined (TR_TIME_FREE) && defined (_Windows) && !defined (__WIN32__)
        real_time TheTimeNow;

        _DX = VTD_Addr_High;
        _AX = VTD_Addr_Low;
        GETTICKS ();
        TheTimeNow = (real_time)((TARRAY[1] * SEC_PER_OV) + (TARRAY[0] / CK_FREQ));
        TheTimer->TheTime = TheTimeNow - TheTimer->StartTime;
        return (TheTimer->TheTime);

    //  If using free-running timer in Windows NT, ask the performance counter for a
    //  tick count; subtract tick count at program start; and divide by tick frequency
    #elif defined (TR_TIME_FREE) && defined (_Windows) && defined (__WIN32__)
        LARGE_INTEGER PerfTicks;
        real_time TheTimeNow;

        QueryPerformanceCounter (&PerfTicks);
        TheTimeNow = (real_time)(((GetHighPart (PerfTicks)
                     - GetHighPart (StartPerfTicks))
                     * 4294967296 / TheTimer->PerfFreq) + ((GetLowPart (PerfTicks)
                     - GetLowPart (StartPerfTicks)) / TheTimer->PerfFreq));
        TheTimer->TheTime = TheTimeNow;
        return TheTimeNow;
    #endif

    //  If using free-running timer under DOS, read the timer hardware directly
    #if defined (TR_TIME_FREE) && !defined (_Windows) && !defined (__WIN32__)
        unsigned short Current;                     //  Current ticks in 8253 chip
        unsigned char msb, lsb;                     //  Hold bytes read from hardware
        unsigned short Delta;                       //  Change in time since last read
    #endif

    //  This section runs when we're using timer 0 (may be shared with interrupts)
    #if defined (TR_TIME_FREE) && defined (TR_USE_TIMER_0)
        outp (TIMER_CTL, 0x04);                     //  Latch current timer value
        lsb = (unsigned char) inp (TIMER0);         //  Read LSB of word in timer
        msb = (unsigned char) inp (TIMER0);         //  first, and then MSB
        Current = (short)lsb | ((short)msb  << 8);  //  Concatenate bytes together
        if (Current < LastTime)                     //  If timer hasn't overflowed
            Delta = LastTime - Current;             //  calculate delta.  If it has,
        else                                        //  we must correct for period
            Delta = IntRateDivisor - Current + LastTime;
    #endif

    //  This is the section for timer 2, which is never shared with interrupts
    #if defined (TR_TIME_FREE) && defined (TR_USE_TIMER_2)
        outp (TIMER_CTL, 0x84);                    //  Latch current timer value
        lsb = (unsigned char) inp (TIMER2);        //  Read LSB of word in timer first
        msb = (unsigned char) inp (TIMER2);        //  and then MSB
        Current = (short)lsb | ((short)msb  << 8); //  Concatenate bytes together
        Delta = LastTime - Current;                //  Calculate ticks since last read
    #endif

    #if defined (TR_TIME_FREE) && !defined (_Windows) && !defined (__WIN32__)
        LastTime = Current;                        //  Save current count for later

        //  If the number of ticks is greater than Delta_Limit, consider this an over-
        //  flow.  This is a statistical error, since an overflow of certain values
        //  will not be detected.  
        if (Delta > Delta_Limit)
            TR_Exit ("Free-running timer overflow (over %4.1lf ms) at time %.3lf", 
                     (double)Delta_Limit / CK_FREQ * 1000.0, TheTimer->TheTime);

        //  Calculate and save the time from the number of ticks since last measurement
        TheTimer->TheTime += ((real_time)Delta) / (real_time)CK_FREQ;
        return (TheTimer->TheTime);
    #endif

    //  If in interrupt timing mode, calculate the time from the number of ISR ticks
    #if defined (TR_TIME_INT)
        return (TheTimer->DeltaTime * (real_time)(TR_TimerTicks));
    #endif

    //  Time must have been incremented by simulation; just grab a copy
    #if defined (TR_TIME_SIM)
        return (TheTimer->TheTime);
    #endif
    }


//-------------------------------------------------------------------------------------
//  Functions:  PreventPreemption and AllowPreemption
//      These functions are used to prevent another task, even one of higher priority,
//      from pre-empting the current task.  This is usually done to prevent the cor-
//      ruption of data which is being transferred.  Preemption is prevented by the
//      timer ISR (above in this file) which checks the prevention counter; if the
//      counter is greater than zero, no functions are called by the timer ISR.
//      These functions are only needed, and defined, when in multithreading mode.      

#if defined (TR_THREAD_MULTI)

    void PreventPreemption (void)
        {
        PreemptionCounter++;
        }

    void AllowPreemption (void)
        {
        if (PreemptionCounter > 0)  PreemptionCounter--;
        }

#endif  //  TR_THREAD_MULTI

