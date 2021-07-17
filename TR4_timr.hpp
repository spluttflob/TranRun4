//*************************************************************************************
//  TR4_TIMR.HPP
//      This is the header file which contains the interface for the real-time keeping
//      object in the TranRun3 scheduler.
//  
//  Copyright (c) 1994-1997 by D.M.Auslander and J.R.Ridgely
//      May be used and distributed for any non-commercial purposes as long as this
//      copyright notice is included.
//*************************************************************************************

#ifndef TR4_TIMR_HPP
    #define TR4_TIMR_HPP                    //  Variable to prevent multiple inclusions

//=====================================================================================
//  Class: CRealTimer
//      This class implements a real-time timer which keeps track of "real" time in
//      whatever way corresponds to the operational mode of the program.  Real time
//      modes include
//        - Simulated time, in which time is incremented by the master class when a  
//          scan has been completed 
//        - Single-thread real time, in which the timer keeps a clock but interrupts 
//          are not used to control program flow 
//        - Full multithreading real time, where interrupts run non-continuous tasks
//=====================================================================================

#include <sys\timeb.h>          //  Has _ftime() structure; should be ANSI compatible

class CRealTimer : public CTaskList
    {
    private:
        real_time TheTime;                  //  Time in seconds since program started 
        real_time DeltaTime;                //  How much to increment time every step 
        timeb Start_ftime;                  //  Starting time used in _ftime mode
        real_time StartTime;                //  Start time for Windows virtual timer 
        CFileObj* DumpFile;                 //  File for status dump destination 
        CInterruptObj* InterObj;            //  Object holds timer interrupt vector
        #if defined (__WIN32__)             //  Use "Performance Counter" for 32-bit:
            double PerfFreq;                //  Frequency of performance counter ticks
        #endif

    public:
        CRealTimer (void);                  //  Constructor takes pointer to stack
        ~CRealTimer (void);                 //  space for 80x87 states

        void Setup (real_time);             //  Function to set up timer's tick time 
        void Go (void);                     //  Start timer running, setting clock to 0
        void Stop (void);                   //  Stop timer, removing interrupts etc.  
        void Increment (void);              //  Add one clock tick to the current time
        real_time GetDeltaTime (void)       //  Function to return the tick time
            { return (DeltaTime); }
        void DumpStatus (const char*);      //  Print status dump for timer object
        const char *GetTimeString (void);   //  Returns current time in a string

        //  GetTimeNowUnprotected() returns the real time without interrupt protection;
        //  GetTimeNow() is the user-callable interrupt-protected version (unless a
        //  non-interrupt mode is being used, in which case there's no difference).
        friend real_time GetTimeNowUnprotected (void);
        #if defined (TR_THREAD_MULTI) || defined (TR_TIME_INT)\
                                      || defined (TR_THREAD_TIMER)
            friend real_time GetTimeNow (void);
        #else
            #define  GetTimeNow  GetTimeNowUnprotected
        #endif

        //  Timer ISR is defined as friend function only if using an interrupt-driven
        //  mode.  It's friend so it can directly access timer's reentry control vars.
        #if defined (TR_THREAD_MULTI) || defined (TR_TIME_INT)
            friend void TimerISR (void);
        #endif
    };

//  Prototypes of functions to define a "critical section" of code within which other
//  tasks may not pre-empt the currently executing task.  These sections are usually
//  used to prevent corruption of data which is being transferred.  The names
//  CriticalSectionBegin() and ...End() are used for compatibility with the template
//  type scheduler
#define  CriticalSectionBegin()  PreventPreemption()
#define  CriticalSectionEnd()    AllowPreemption()
#if defined(TR_THREAD_MULTI)
    void PreventPreemption (void);
    void AllowPreemption (void);
#else
    #define  PreventPreemption()
    #define  AllowPreemption()
#endif

//  Make a _global_ pointer to "the" timer object which is shared by everyone
extern CRealTimer *TheTimer;

#endif      //  End multiple inclusion protection

