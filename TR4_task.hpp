//*************************************************************************************
//  TR4_Task.hpp
//      This is the header for the task and task list classes used in the TranRun4
//      scheduler project.
//
//  Copyright (c) 1994-1996, D.M.Auslander and J.R.Ridgely
//      May be used and distributed for any non-commercial purposes as long as this 
//      copyright notice is included.
//
//  Version
//      11-01-94  DMA  Original file
//      12-21-96  JRR  Changed to TranRun4 version  
//*************************************************************************************

#ifndef  TR4_TASK_HPP                       //  Variable to prevent multiple inclusions
    #define  TR4_TASK_HPP

//=====================================================================================
//  Class: CTask
//      Here we implement the basic behavior which is common to all kinds of tasks:
//        - The task has a name and a status
//        - The task knows where to find its 'action' function and run it
//        - Keeps track of priority (OK, continuous tasks don't need this)
//        - Knows how long between running and when the next time to run is (ditto)
//        - The task can dump its status for diagnostics
//      Added for TranRun3, 5-22-95:
//        - A task keeps a list of CState objects which represent T.L. states   
//=====================================================================================

//  Enumeration of task types.  Note that the task type names in an array in the file
//  CTL_TASK.CPP *must* match and be in the same order as the types in this list.
enum TaskType
    {
    HARDWARE_INT,       //  Real hardware interrupts trigger this type of task
    TIMER_INT,          //  Task runs on soft timer interrupts
    PREEMPTIBLE,        //  Task run in timer ISR, but can be pre-empted
    SAMPLE_TIME,        //  This runs once every sample-time interval
    EVENT,              //  Here's a task which runs on flag in event table
    CONTINUOUS          //  Task which runs in the background - lowest priority
    };

//  Enumeration of valid task status values, used to check what a task's up to
enum TaskStatus
    {
    TS_IDLE,            //  Task won't run again 'til next clock tick
    TS_READY,           //  Task is ready to be run again; also code for "it has run"
    TS_PENDING,         //  Event or sample time task's run-pending flag has been set
    TS_RUNNING,         //  The task is currently running
    TS_PREEMPTED,       //  Running, but has been pre-empted by another task
    TS_DEACTIVATED      //  Put to sleep; won't run again until re-activated
    };

class CTask : public CBasicList
    {
    private:
        TaskType TheType;                   //  Task type - continuous, timer, etc.
        TaskStatus Status;                  //  Status of the task at a given time
        real_time TimeInterval;             //  Interval between runs of task function
        real_time NextTime;                 //  Next time at which task func. will run
        double TimingTolerance;             //  How far can we miss assigned run time?
        long TimesRun;                      //  How many times has this task been run?
        int MyPriority;                     //  Key used for sorting is priority
        int SerialNumber;                   //  Serial number of this task in list
        boolean Do_TL_Trace;                //  Do we record state transitions?
        CState* pInitialState;              //  First state to be run by scheduler
        CState* pCurrentState;              //  State currently being run by this task
        boolean FirstTimeRun;               //  Is TRUE first time we run this task
        int InsertStateCounter;             //  Creates serial numbers for the states
        boolean DoProfile;                  //  TRUE if we're keeping run duration data
        CProfiler* RunProfiler;             //  Pointer to profiler object for Run()

        //  Configure method:  The constructors call this to initialize the task
        void Configure (const char*, TaskType, int, real_time);

    protected:
        char* Name;                         //  Name of this task, as char. string
        long State;                         //  The TL state in which this task is now

    public:
        CTask (const char*, TaskType);      //  Constructor for continuous tasks
        CTask (const char*, TaskType,       //  Here's the constructor for the timer
               real_time);                  //    interrupt tasks
        CTask (const char*, TaskType,       //  This constructor is for event tasks
               int);                        //
        CTask (const char*, TaskType,       //  Constructor for sample time tasks
               int, real_time);             //
        virtual ~CTask (void);              //  Destructor

        //  For state-based scheduling, Run() must be overridden by the user; for task
        //  based scheduling it calls states' Entry(), Action(), and TransitionTest()
        virtual void Run (void);

        TaskStatus Schedule (void);         //  Decide which state functions to run
        void TraceOff (void)                //  User calls this function to deactivate
            { Do_TL_Trace = FALSE; }        //    tracing of state transitions
        void SetSampleTime (real_time);     //  Function resets interval between runs
        real_time GetSampleTime (void)      //  Function allows anyone to find out the
            { return (TimeInterval); }      //    sample time of this task
        int GetPriority (void)              //  Function returns task priority, which
            { return (MyPriority); }        //    is sort key in sample time task list
        const char* GetName (void)          //  Function returns a pointer to task's
            { return (Name); }              //    name (a character string)
        int GetSerialNumber (void)          //  This function returns the serial num-
            { return (SerialNumber); }      //    ber of this task in process's list
        void SetSerialNumber (int aNumber)  //  The process calls this function to set
            { SerialNumber = aNumber; }     //    the number of this task in its list
        void DumpStatus (FILE*);            //  Write status dump to the file
        void DumpConfiguration (FILE*);     //  Write to configuration dump file
        void Idle (void);                   //  Task stops until next sample time
        boolean TriggerEvent (void);        //  Called to make event task pending
        void Deactivate (void);             //  Task stops running until re-activated
        void Reactivate (void);             //  To start a de-activated task up again
        void InsertState (CState*);         //  Insert a new state into the state list
        void SetInitialState (CState*);     //  Specify the state in which we start up
        void DumpProfile (const char*);     //  Write function timing information
        void DumpProfile (FILE*);           //  Same function taking file pointer
        void DumpDurations (const char*);   //  Function to print execution time table
        void DumpLatencies (const char*);   //  Function prints table of latencies 
        void ProfileOn (void);              //  Turn profiling on for task or states
        void ProfileOff (void);             //  Function to turn profiling off again

        //  Call this function to set tolerance (how long before task is "late")
        void SetTimingTolerance (double aFrac)
            { TimingTolerance = aFrac * (double)TimeInterval; }

    //  These classes need to access private or protected data, but do so minimally
    friend class CProcess;
    };

#endif                                      //  End of multiple inclusion protection

