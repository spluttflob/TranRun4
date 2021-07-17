//*************************************************************************************
//  TR4_stat.hpp
//      This is the header for the T.L. state classes used in the TranRun4 project.
//
//  Copyright (c) 1994-1996, D.M.Auslander and J.R.Ridgely
//      May be used and distributed for any non-commercial purposes as long as this
//      copyright notice is included.
//*************************************************************************************

#ifndef TR4_STAT_HPP
    #define  TR4_STAT_HPP                   //  Variable to prevent multiple inclusions

//  Forward declaration of CTask (needed because states keep pointers to parent tasks)
class CTask;


//=====================================================================================
//  Class: CState
//      This class implements a transition logic state.  A task contains a list of
//      states; each state must have at least one of the following:
//        - Entry function, run when the task enters the given state
//        - Action function, run again and again while the task remains in a state
//        - Test function(s) which determine if task will enter a new state
//=====================================================================================

class CState
    {
    private:
        char* Name;                     //  String holds the name of this state
        int NonOverloadedCount;         //  Count of functions not yet overloaded
        boolean FirstTimeRun;           //  Is this the first time this state's run?
        boolean EnteringThisState;      //  True when entering this state
        int SerialNumber;               //  Number of this state in task's state list
        boolean DoProfile;              //  True if we are keeping run duration data
        CProfiler* EntryProfiler;       //  These are the execution time profiler
        CProfiler* ActionProfiler;      //  objects for the entry, action, and tran-
        CProfiler* TestProfiler;        //  sition test functions

    protected:
        CTask* pParent;                 //  Pointer to parent task

    public:
        //  Constructor is given a name for the new state
        CState (const char*);
        virtual ~CState (void);

        //  This function returns a pointer to the state's name, which is in a string
        const char* GetName (void) { return (Name); }

        //  Entry function is called when the parent task enters this state; Action
        //  function runs repeatedly while remaining in this state; Transition Test
        //  function is called at end of every scan and returns the value of a new
        //  state to be entered next scan (or it returns NULL for no transition).
        //  Schedule() is the function which calls these functions.
        virtual void Entry (void);
        virtual void Action (void);
        virtual CState* TransitionTest (void);
        CState* Schedule (void);

        void SetSampleTime (real_time);     //  Function resets interval between runs
        real_time GetSampleTime (void);     //  To get sample time of parent task
        int GetPriority (void);             //  Function gets priority of parent task
        int GetSerialNumber (void)          //  This function returns the serial num-
            { return (SerialNumber); }      //    ber of this state in the task's list
        void SetSerialNumber (int aNumber)  //  The task calls this function to set
            { SerialNumber = aNumber; }     //    the number of this state in its list
        void Idle (void);                   //  Parent task idles 'til next sample time
        void Deactivate (void);             //  Task stops running until re-activated
        void Reactivate (void);             //  To start a de-activated task up again
        void SetParent (CTask* aPtr)        //  Function called by the task object to
            { pParent = aPtr; }             //    set pointer to parent task object
        void ProfileOff (void)              //  Function to turn execution time pro-
            { DoProfile = FALSE; }          //    filing back off
        void DumpProfile (FILE*,            //  Function writes information about how
                            const char*);   //    long state functions took to run

        //  Write histogram tables of run times and latencies to the given files
        void DumpDurations (const char*, const char*, const char*);
        void DumpLatencies (const char*, const char*, const char*);

        //  Turn execution-time profiling on with default and user-given parameters
        void ProfileOn (void) { DoProfile = TRUE; }
        void ProfileOn (int, real_time, real_time);
    };

#endif      //  End of multiple-inclusion protection

