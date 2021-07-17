//*************************************************************************************
//  TranRun4.hpp
//      This is the header for version 4 of Prof. Auslander's TranRun real-time
//      programming environment.  The current version does not use a code generator.
//      In TranRun4, object oriented programming under C++ is used as extensively
//      as practicable and the slight additional overhead is tolerated.
//
//  Real-time modes
//      The following #defines specify timekeeping modes:
//
//        TR_TIME_SIM    - Pure simulated time.  A "real time" count is incremented
//                         once per sweep of the task functions.  This mode may be
//                         "calibrated" by adjusting the tick time to reflect the
//                         average time taken by the scheduler for a task sweep.
//        TR_TIME_FREE   - Time is read from the free-running timer, a chip in the PC.
//                         When running under DOS, we read the hardware; under Windows
//                         we call a Windows API function to get the fime 
//        TR_TIME_INT    - Timer interrupts are used to keep time, but not to control
//                         tasks.  The timer interrupt's period may be set by the user
//                         to a reasonable number (>0.2) of milliseconds.
//        TR_TIME_FTIME  - The PC's internal timer is used to measure time.  The
//                         recursive (but not reentrant) scheduler is used.  PC's have
//                         a pretty lousy timer with a resolution of ~55 ms, so this
//                         mode is useful only for slow-running systems.
//        TR_TIME_EXTSIM - (*Not ready*) An external simulation module keeps time.
//
//      These next #defines control single or multithreading modes.
//
//        TR_THREAD_MULTI  - Timer interrupts call the scheduler, which may preempt
//                           a running task.  Continuous and sample-time tasks run in
//                           the background.
//        TR_THREAD_SINGLE - Only one thread runs, and interrupts are used only to
//                           increment the real-time clock (if at all).  Each task
//                           function runs only when the previous one has finished.
//
//      Sequential operation in which task functions all run in order is has less
//      overhead.  Minimum-latency mode has higher overhead, but the latency is less
//      because fast tasks don't have to wait until all the slower tasks have run.
//
//        TR_EXEC_MIN - After every task function run, the scheduler checks for the
//                      highest priority task and runs its function
//        TR_EXEC_SEQ - All task functions run sequentially regardless of priority
//
//  Copyright (c) 1994-1997 by D.M.Auslander and J.R.Ridgely
//      May be used and distributed for any non-commercial purposes as long as this
//      copyright notice is included.
//      Original program 11-94 by DMA
//      11-21-94  JR   New class structure
//      12-04-94  JR   Interrupt support added
//      12-14-94  JR   Merged task_obj.hpp, #included base_obj.hpp into this file
//      12-20-94  JR   Removed C type tasks, left C++ task interface only
//       1-02-95  DMA  Added CSim class to support simulation timed by scheduler
//       2-02-95  JR   Split header files into sets by class, made this main header
//       5-20-95  JR   Changed from CTL_EXEC to Tranlog3 project and file names
//       8-24-95  JR   Took CSim back out again... who needs it
//      12-21-96  JR   Evolved from TranRun3 to TranRun4 this evening 
//*************************************************************************************

#ifndef TRANRUN4_HPP
    #define TRANRUN4_HPP                    //  Variable to prevent multiple inclusions

//-------------------------------------------------------------------------------------
//  Handy Macros
//      - Real time format is defined here so the user can change to, say, long double
//        if need be for more precision over longer times
//      - Late time fraction is a tolerance for tasks running on time
//      - Maximum re-entrance controls how backed up multithreading mode can get with
//        functions which should run but can't find the time before it gives up

//  Define a type to be used for storing time - generally high precision floating
//  point.  Also define the format which is to be used by printf() and scanf() to
//  read and write variables of that type.  These two must be compatible.
#define  real_time           double
#define  REAL_TIME_FORMAT    "%lg"

//  This is the percent of a sample time by which a function may run late before the
//  task's tardiness causes a run-time error.  For example, 0.5 means that if the task
//  has a sample time of 1 ms, running 0.4 ms late is OK but 0.6 ms late is not.
//  A very high number means we're allowing tasks to run very late; this may be
//  necessary to run under Windows 3.1 or NT, or run under the Borland IDE.
#define  LATE_TIME_FRACTION  123.4

//  Define the maximum number of re-entry levels you'll tolerate in multithreading
//  mode.  More times than this, and the program will refuse to re-enter again
#define  MAX_REENTER         8

//  Define the width of pages on which timing and diagnostic printouts will go
#define  PAGE_WIDTH          78


//-------------------------------------------------------------------------------------
//  Global Function Prototypes
//      A minimum of globally accessible utility functions are used.  In addition to
//      the error exit functions prototyped here, the timer object has a global-scope
//      friend function called GetTimeNow().

void TR_Message (const char *, ...);        //  Write a one-line message to user
real_time GetTimeNow (void);                //  Returns time in the real-time clock
int UserMain (int, char**);                 //  User's main function acts like main()


//-------------------------------------------------------------------------------------
//  Project Header File List
//      These header files are the ones which must be included to make TranRun 3
//      projects.  The user must edit 'tr3_conf.hpp' manually, as it is used to define
//      the operating environment (scheduler mode, timekeeping method, task or state
//      based project, and so on).  

#include "TR4_conf.hpp"         //  Configuration file with preprocessor variables

//  We always seem to need standard header files
#include <stdio.h>
#include <stdlib.h>

//  If using a multithreading mode or interrupt timekeeping, #include interrupt defs.
#if defined (TR_THREAD_MULTI) || defined (TR_TIME_INT)
    #include <dos.h>
#endif

#include <TR4_comp.hpp>         //  Compatibility file for use with different compilers
#include <base_obj.hpp>         //  Basic node, list, string, and file classes
#include <data_lgr.hpp>         //  Data logger class
#include <TR4_intr.hpp>         //  Interrupt-handler class
#include <TR4_prof.hpp>         //  Execution-time profiling utility
#include <TR4_stat.hpp>         //  States and state transitions
#include <TR4_task.hpp>         //  Task and task list classes
//#include <TR4_shar.hpp>         //  Shared variable classes (not ready yet)
#include <TR4_proc.hpp>         //  Class for process, set of tasks on one computer
#include <TR4_mstr.hpp>         //  Master scheduler class holds it all together
#include <TR4_timr.hpp>         //  Class which handles real-time timekeeping

#endif          //  End of multiple inclusion protection

