//*************************************************************************************
//  TR4_mstr.hpp
//      Here's the interface for the master scheduler class.  The master scheduler is
//      the object which controls the running of all the tasks in each process while
//      the timer object keeps time.
//
//  Copyright (c) 1994-1996, D.M.Auslander and J.R.Ridgely
//      May be used and distributed for any non-commercial purposes as long as this
//      copyright notice is included.
//      11-01-94  DMA  Original program
//      11-21-94  JR   New class structure
//      12-15-94  JR   Spun off as separate implementation file
//       1-29-95  JR   CSim simulation stuff moved into this file from timer file
//       5-20-95  JR   Inserted into Tranlog3 project with necessary name changes
//      12-21-96  JR   Changed to TranRun4 flexible-scheduling version
//*************************************************************************************

#ifndef TR4_MSTR_HPP
    #define  TR3_MSTR_HPP                   //  Variable to prevent multiple inclusions

//=====================================================================================
//  Class: CMaster
//      This is the scheduler that runs everything.  Just one master may exist; it is
//      instantiated in TRANRUN3.CPP and a pointer to it is called TheMaster.  
//=====================================================================================

//  This enum represents possible states of the process scheduler 
enum MasterState 
    {
    GOING,              //  Scheduler running normally 
    STOPPED             //  Scheduler paused, able to resume
    };

class CMaster: public CBasicList
    {
    private:
        MasterState Status;                 //  State of master scheduler 
        CDataLogger* TraceLogger;           //  Transition logic data logger object
        real_time StopTime;                 //  Time when master will shut off
        CString* ExitMessage;               //  Message displayed when master stops

    public:
        CMaster (void);                     //  Default constructor
        ~CMaster (void);                    //  and destructor as usual

        CProcess* AddProcess (char*);       //  Create a process and put it in list
        CProcess* InsertProcess             //  If you've already created a process,
            (CProcess*);                    //    use this to insert it in the list
        void SetTickTime (real_time);       //  Set time between timer object ticks
        void SetStopTime (real_time);       //  Set time at which control will stop
        void Go (void);                     //  Start scheduler up
        void Stop (void);                   //  Halt the scheduler
        void RunBackground (void);          //  Run the tasks not called by ISR's
        void RunForeground (void);          //  Run pre-emptive scheduler (one pass)
        void DumpTrace (const char*);       //  Dump transition logic trace to a file
        void DumpTraceNumbers (const char*);//  Dump trace in numbers form for Matlab
        void ProfileOn (void);              //  Turn profiling on for all processes 
        void ProfileOff (void);             //  Turn profiling back off again
        void DumpProfiles (const char*);    //  Dump execution-time profiles of tasks
        void SetExitMessage (char* aMsg)    //  Set a new exit message, usually a
            { *ExitMessage = aMsg; }        //    complaint causing an error exit
        friend void TL_TraceLine            //  These two TL_TraceLine functions are
            (void*, void*, void*);          //    called by tasks to record state
        friend void TL_TraceLine            //    transitions.  The first is for
            (void*, long, long);            //    TranRun3, the second for CTL_EXEC.

        //  Friend function complains and exits when serious errors occur
        friend void TR_Exit (const char*, ...);
        };


//-------------------------------------------------------------------------------------
//  Global pointer:  TheMaster
//      Make a _global_ pointer to "the" master object which is shared by all modules 
//      (It's instantiated in CTL_MSTR.CPP)

extern CMaster* TheMaster;

#endif      //  End multiple-inclusion protection

