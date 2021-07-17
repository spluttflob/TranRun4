//*************************************************************************************
//  TR4_task.cpp
//      This is the implementation file for task object classes.  These objects imple-
//      ment tasks for the ctl_exec scheduler; the task objects know how (and when)
//      to run themselves, dump their status, etc. etc.
//
//  Tasks and States
//      Version 3 - when compiled in state-based mode, tasks must be given lists of
//                  state objects.  States are defined in TR3_stat.hpp/cpp.
//      Version 4 - tasks may or may not have lists of states.  If you do not use a
//                  list of states for a given task, you must overload the Run()
//                  function for that task.  
//
//  Copyright (c) 1994-1996, D.M.Auslander and J.R.Ridgely
//      May be used and distributed for any non-commercial purposes as long as this 
//      copyright notice is included.  
//      Original program 11-94 by DMA 
//      11-21-94  JR   New class structure
//       1-15-95  JR   Event type tasks added
//       3-05-95  JR   Scheduler works for food
//       3-08-95  JR   Added stuff for profiler
//       5-20-95  JR   Changed to TranRun3 project
//       6-01-95  JR   Added TaskData pointer and handlers
//       6-09-95  JR   Removed TaskData pointer; now it's inherited by CStates
//       8-17-95  JR   Timer Interrupt tasks get multiple scans and must call Idle()
//       9-02-95  JR   Interrupt protection for task list manipulations
//       9-18-95  JR   Merged CTL_EXEC code into this file with #if defined's etc.
//      12-21-96  JR   Changed to TranRun4, to allow mixing of state and task based   
//*************************************************************************************

#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <TranRun4.hpp>


//-------------------------------------------------------------------------------------
//  Task type Names and Status Names:  These arrays of character strings hold some
//  names corresponding to task types and statuses given above.  These arrays must
//  match the enumerations, i.e. be in the same order.  They're used in the status
//  dumping functions.

const char *TaskTypeNames[6] = {"Hardware", "Timer Int.", "Preemptible",
                                "Sample Time", "Event", "Continuous"};
const char *TaskStatusNames[6] = {"Idle", "Ready", "Pending", "Running", "Pre-empted",
                                  "Deactivated"};


//=====================================================================================
//  Class: CTask
//      Here we implement the basic behavior which is common to all kinds of tasks.
//      Tasks can run themselves (they know how to check if it's time to do so), give
//      out pointers to their names, dump their status, and suspend themselves.
//=====================================================================================

//-------------------------------------------------------------------------------------
//  Constructor: CTask (Version for timer interrupt type tasks)
//      A new C++ style task object is created and the fields are filled in.  No task
//      priority is given, as this is a timer interrupt task which doesn't need it.

CTask::CTask (const char *aName, TaskType aType, real_time aTimeInt) : CBasicList ()
    {
    if (aType != TIMER_INT)
        TR_Exit ("Task \"%s\" constructed with wrong parameters", aName);

    //  Call configure function for the task; give it a null task function pointer 
    Configure (aName, aType, 0, aTimeInt);
    }


//-------------------------------------------------------------------------------------
//  Constructor: CTask (Version for sample time tasks)
//      A new task object is created and the fields are filled in with the name, task 
//      function, etc.  It makes no sense to call this when constructing another type 
//      of task, as other types don't have priority; if the user tries to, complain.  

CTask::CTask (const char *aName, TaskType aType, int aPriority, real_time aTimeInt)
    : CBasicList ()
    {
    if ((aType != SAMPLE_TIME) && (aType != PREEMPTIBLE))
        TR_Exit ("Task \"%s\" constructed with wrong parameters", aName);

    //  Call the function which does the work of copying member data, etc.
    Configure (aName, aType, aPriority, aTimeInt);
    }


//-------------------------------------------------------------------------------------
//  Constructor: CTask (Version for event tasks)
//      A new task event object is created and the fields are filled in with the name,
//      task function, etc.  This version must be used for event tasks only.

CTask::CTask (const char *aName, TaskType aType, int aPriority) : CBasicList ()
    {
    if (aType != EVENT)
        TR_Exit ("Task \"%s\" constructed with wrong parameters", aName);

    //  Call the function which does the work of copying member data, etc.
    Configure (aName, aType, aPriority, (real_time)0.0);
    }


//-------------------------------------------------------------------------------------
//  Constructor: CTask (version for continuous tasks only)
//      A new task object is created and filled.  Since no timing information is given,
//      this must be for a continuous task.

CTask::CTask (const char *aName, TaskType aType) : CBasicList ()
    {
    if (aType != CONTINUOUS)
        TR_Exit ("Task \"%s\" constructed with wrong parameters", aName);

    Configure (aName, aType, 0, (real_time)0.0);
    }


//-------------------------------------------------------------------------------------
//  Function: Configure
//      All the task constructors have to copy the member data from argument lists and
//      allocate some memory, so those functions are performed here.

void CTask::Configure (const char* aName, TaskType aType, int aPriority,
real_time aTimeInt)
    {
    //  Make some space and save the name of this object
    if ((Name = new char[strlen (aName) + 1]) != NULL)
        strcpy (Name, aName);

    TheType = aType;                //  Save type of task
    MyPriority = aPriority;         //  Save priority, which is sort key for task list
    TimeInterval = aTimeInt;        //  and time interval
    TimesRun = (long)0;             //  Task hasn't run yet
    Do_TL_Trace = TRUE;             //  Default is state transition tracing on
    SerialNumber = -1;              //  Serial number of this task in process: invalid
    State = 0L;                     //  Set the initial state to 0 (user can override)
    FirstTimeRun = TRUE;            //  This will be true during the first run only
    pInitialState = NULL;           //  Don't have an initial state yet
    pCurrentState = NULL;           //  Nor have we transitioned into any state
    InsertStateCounter = 0;         //  The first state inserted will be number 0

    //  If this is a preemptible, sample time or event task, its initial status is
    //  IDLE, waiting for a reason to run.  Otherwise the status is READY.
    if ((TheType == SAMPLE_TIME) || (TheType == EVENT) || (TheType == TIMER_INT)
        || (TheType == PREEMPTIBLE))
        Status = TS_IDLE;
    else
        Status = TS_READY;

    //  Set timing tolerance (the time by which a task can run late without problems)
    TimingTolerance = (double)LATE_TIME_FRACTION * (double)TimeInterval;

    //  The first time to run this task will be a random time between 0 and aTimeInt
    NextTime = aTimeInt * (real_time)rand () / RAND_MAX;

    //  Create an execution time profiler object; profiling is off by default 
    RunProfiler = new CProfiler ();
    DoProfile = FALSE;
    }


//-------------------------------------------------------------------------------------
//  Destructor:  ~CTask
//      This destructor frees up memory used by the task item.  Since a TranRun3 task
//      contains a bunch of states, this destructor deletes the states one by one.

CTask::~CTask (void)
    {
    //  Delete all states which are under this task
    for (void* pCur = GetHead (); pCur != NULL; pCur = GetNext ())
        delete (CState*)(GetCurrent ());

    DELETE_ARRAY Name;                      //  Also delete arrays and objects 
    delete RunProfiler;
    }


//-------------------------------------------------------------------------------------
//  Function: SetSampleTime
//      Here we change the time interval between task runs.  It will take effect after
//      the next call to Schedule().  The tolerance is also adjusted.

void CTask::SetSampleTime (real_time aNewTime)
    {
    TimeInterval = aNewTime;
    TimingTolerance = (double)LATE_TIME_FRACTION * (double)TimeInterval;
    }


//-------------------------------------------------------------------------------------
//  Function: Schedule
//      This is the function where the task decides whether or not to execute its Run()
//      method.  The task checks to see if it's supposed to run at this time (which
//      depends on the time and on what kind of task it is); if it's time to run, the
//      Run() method is called.  The task then checks to see if the transition logic
//      state has changed; if so, it writes state-change information to the transition
//      logic trace file if the trace file is writeable.

TaskStatus CTask::Schedule (void)
    {
    long OldState;                      //  State number before we run Run()
    real_time BeginTime;                //  Time when Run() function starts

    #if defined (TR_TIME_SIM)           //  If there's no hardware to keep time (as
        TheTimer->Increment ();         //  in simulation mode), increment time now
    #endif

    //  If this task is already running or has been pre-empted or deactivated, don't
    //  start it now; instead, return the state
    if ((Status == TS_RUNNING) || (Status == TS_PREEMPTED)
                               || (Status == TS_DEACTIVATED))
        return (Status);

    //  If the task is of type TIMER_INT or SAMPLE_TIME and if its status is IDLE,
    //  check to see if it's time to run yet.  If it's time to run again, update
    //  the next time register; if not, return without running yet
    if (((TheType == TIMER_INT) || (TheType == SAMPLE_TIME)) && (Status == TS_IDLE)
        || (TheType == PREEMPTIBLE))
        {
        if (GetTimeNowUnprotected () < NextTime)
            return (TS_IDLE);

        //  Here we check to see if the task has been delayed way too much and missed
        //  its execution time; if it has, complain and exit
        if (GetTimeNowUnprotected () > (NextTime + TimingTolerance))
            {
            TR_Exit ("Unable to run task \"%s\" on time", Name);
            return (TS_DEACTIVATED);            //  Here in case function is modified
            }

        //  The task is going to run, so update status and next-run-time register
        Status = TS_READY;
        NextTime += TimeInterval;
        }

    //  If this is an IDLE event task, don't run.  A non-idle event task, including
    //  one which has been set to PENDING by TriggerEvent(), will run.
    if ((TheType == EVENT) && (Status == TS_IDLE))
        return (TS_IDLE);

    Status = TS_RUNNING;                //  Set status flag to indicate it's going

    //  Saves old T.L. state, record the time, and enable interrupts before Run() runs
    OldState = State;
    if (DoProfile)  BeginTime = GetTimeNowUnprotected ();
    #if defined (TR_THREAD_MULTI)
        EnableInterrupts ();
    #endif

    Run ();                             //  Run the task's Run() function

    //  Disable interrupts after running Run()
    #if defined (TR_THREAD_MULTI)
        DisableInterrupts ();
    #endif

    //  If in task-based mode and profiling is on, save the function's run time
    if (DoProfile)  RunProfiler->SaveData (GetTimeNowUnprotected () - BeginTime);

    //  Do a transition-logic trace
    if (OldState != State)  TL_TraceLine (this, OldState, State);

    TimesRun++;                         //  Increment count of times we've run

    if (Status == TS_IDLE)              //  If the user has just called Idle() during
        return (TS_IDLE);               //  this execution, return idle code now

    if (Status == TS_RUNNING)           //  Set status of task which has been running
        Status = TS_READY;              //  to ready; idle task will remain idle

    return (TS_READY);
    }


//-------------------------------------------------------------------------------------
//  Function: Run (Version for state-based TranRun3 scheduler)
//      This function calls the Entry(), Action(), and/or TransitionTest() functions
//      associated with the state in which this task is currently running.  If the
//      user is writing his own task, he'll override this Run() function.

void CTask::Run (void)
    {
    CState* pNextState;                     //  Pointer to the next state to be run

    //  If this is very first time to run (or restart), go to specified initial state
    if (FirstTimeRun == TRUE)
        {
        FirstTimeRun = FALSE;               //  Well it's not gonna be true next time

        //  If no initial state has been specified, we can't run; so exit
        if (pInitialState == NULL)
            TR_Exit ("No initial state specified for task \"%s\"", Name);
        //  Else, an initial state was given, so go to its location in the list
        else
            pCurrentState = pInitialState;
        }

    //  Make sure there really is a state to run; if not, complain and exit
    if (pCurrentState == NULL)
        {
        TR_Exit ("No stats or Run() function specified for task \"%s\"", Name);
        }
    else
        {
        //  The CState::Schedule() method returns a pointer to the next state, or
        //  NULL if no transition is going to occur this time
        if ((pNextState = pCurrentState->Schedule ()) != NULL)
            {
            //  If transition logic tracing is enabled, write a line in the trace file
            if (Do_TL_Trace == TRUE)
                TL_TraceLine (this, pCurrentState, pNextState);

            pCurrentState = pNextState;
            }
        }
    }


//-------------------------------------------------------------------------------------
//  Function: Idle
//      This function sets the status of the task to IDLE if it's a pre-emptive task.
//      If the task's not pre-emptive type, it does nothing, because other types of
//      tasks run only once per sample time anyway, so idling means nothing to them.

void CTask::Idle (void)
    {
    if ((TheType == SAMPLE_TIME) || (TheType == EVENT) || (TheType == TIMER_INT)
        || (TheType == PREEMPTIBLE))
        Status = TS_IDLE;
    }


//-------------------------------------------------------------------------------------
//  Function: TriggerEvent
//      This function is used to set the event flag of an event task so that this task 
//      will run as soon as it gets a chance.  It returns FALSE if everything is OK,
//      that is the task is an event task and there's no event previously pending.  If 
//      the task is another type or there was a previously pending event which hasn't 
//      yet been serviced, this function returns TRUE, meaning there was an error.  

boolean CTask::TriggerEvent (void)
    {
    if ((TheType == EVENT) && (Status == TS_IDLE))
        {
        Status = TS_PENDING;
        return (FALSE);
        }
    return (TRUE);
    }


//-------------------------------------------------------------------------------------
//  Function: Deactivate 
//      This function suspends the execution of the task until it's activated.  The 
//      sample time won't make it run; only an Activate() call will.  

void CTask::Deactivate (void)
    {
    Status = TS_DEACTIVATED; 
    }


//-------------------------------------------------------------------------------------
//  Function: Reactivate 
//      This function wakes a deactivated task back up so it can continue running.  

void CTask::Reactivate (void)
    {
    //  If the task is interrupt or sample time, restart the sample-time counter; 
    //  otherwise, the task would run many times to make up for the times it missed. 
    if ((TheType == SAMPLE_TIME) || (TheType == TIMER_INT))
        {
        Status = TS_IDLE;
        NextTime = GetTimeNowUnprotected ();
        }
    else
        Status = TS_READY;              //  Otherwise, it will run as soon as it can
    }



//-------------------------------------------------------------------------------------
//  Function: InsertState
//      The user will have created a class of state object, deriving his custom class
//      from class CState.  He also declared a pointer to a state object.  He then
//      calls this function to add that new state to this task's state list.

void CTask::InsertState (CState *pNew)
    {
    //  Call CListObj's method to insert this thing into the list
    CBasicList::Insert ((void*)pNew, 0);
    pNew->SetParent (this);

    //  Tell the state what its number in the state list is
    pNew->SetSerialNumber (++InsertStateCounter);
    }


//-------------------------------------------------------------------------------------
//  Function:  SetInitialState
//      This function must be called by the user to choose the state in which the
//      scheduler will begin running.  Note that this has nothing to do with the num-
//      ber of the state in the list.

void CTask::SetInitialState (CState *aState)
    {
    pInitialState = (CState *)aState;
    }


//-------------------------------------------------------------------------------------
//  Function: DumpConfiguration
//      This function writes a "configuration dump" for a task.  The idea is that it
//      writes one line containing task name, type, priority, timing and so on and
//      puts state names in a column at the right of that line.  This way when the
//      process asks for a configuration dump, it can get a bunch of task config.
//      dumps and put them in an easy-to-read matrix.  It does nothing for the task
//      based CTL_EXEC scheduler, because CTL_EXEC doesn't define state objects.

void CTask::DumpConfiguration (FILE* aFile)
    {
    char SampleTimeBuf[32];                 //  Buffer to hold sample time in string
    char PriorityBuf[8];                    //  This buffer holds task's priority

    //  If task has priority or sample time, make strings containing them; if not,
    //  just put a " - " there to indicate they're not available or interesting or...
    if ((TheType == SAMPLE_TIME) || (TheType == EVENT) || (TheType == PREEMPTIBLE))
        sprintf (PriorityBuf, "%4d", MyPriority);
    else
        strcpy (PriorityBuf, "    -");

    if ((TheType == TIMER_INT) || (TheType == SAMPLE_TIME) || (TheType == PREEMPTIBLE))
        sprintf (SampleTimeBuf, "%9.3lf", (double)TimeInterval);
    else
        strcpy (SampleTimeBuf, "  -  ");

    //  Print the stuff.  This includes info for the task and the name of the
    //  first state in the list (that's what the GetHead() is up to).
    fprintf (aFile, "\n%-24s %9s %4s  %11s  ", Name, TaskTypeNames[(int)TheType],
             PriorityBuf, SampleTimeBuf);

    //  Print the list of state names - there are none for task-based scheduling
    if (GetHead () != NULL)
        fprintf (aFile, "%-24s\n", ((CState*)GetHead())->GetName ());
    else
        fprintf (aFile, "%-24s\n", " - ");

    //  Now look for more states; if they're there put their names under first one
    CState* pState;
    while ((pState = (CState *)GetNext ()) != NULL)
        {
        fprintf (aFile,
                 "                                                        %s\n",
             pState->GetName ());
        }
    }


//-------------------------------------------------------------------------------------
//  Function: DumpStatus
//      This function sends a status dump for the task to the file object specified in
//      its argument.  This dump string is intended for diagnostic use.

void CTask::DumpStatus (FILE* aFile)
    {
    fprintf (aFile, "        Name: %-18s  Type: %-16s  Time: %lg\n",
             Name, TaskTypeNames[(int)TheType], (double)GetTimeNowUnprotected ());

    fprintf (aFile, "        Status: %-16s  Runs: %u\n",
             TaskStatusNames[(int)Status], TimesRun);

    if ((TheType == SAMPLE_TIME) || (TheType == TIMER_INT))
        fprintf (aFile, "        Sample Time: %-11lg", (double)TimeInterval);

    if ((TheType == SAMPLE_TIME) || (TheType == EVENT))
        fprintf (aFile, "  Priority: %d", MyPriority);

    fprintf (aFile, "\n\n");
    }


//-------------------------------------------------------------------------------------
//  Functions:  ProfileOn/Off
//      These functions turn execution-time profiling on or off for this task.  If the
//      task is using state-based scheduling, they turn profiling on or off for all
//      the states as well.  The states have ProfileOn()/Off() methods you can use if
//      you wish to profile just one state.  Note that these functions must be called
//      AFTER the tasks and states have all been set up. 

void CTask::ProfileOn (void)
    {
    //  If the initial state is NULL, this is a task which contains no state objects
    //  but has a Run() function, so turn profiling for the state on
    if (pInitialState == NULL)
        DoProfile = TRUE;

    //  If the initial state isn't null, this task has states; turn their profilers on
    else
        {
        CState* pCur;
        for (pCur = (CState*)GetHead (); pCur != NULL; pCur = (CState*)GetNext ())
            pCur->ProfileOn ();
        }
    }

void CTask::ProfileOff (void)
    {
    //  When turning profiling off, do so for this task and any existing states
    DoProfile = FALSE;
    for (CState* pCur = (CState*)GetHead (); pCur != NULL; pCur = (CState*)GetNext ())
        pCur->ProfileOff ();
    }


//-------------------------------------------------------------------------------------
//  Functions: DumpProfile
//      These functions write information to the given file about how long the Run()
//      function of this task took to run.  If there are state objects belonging to
//      this task, they print information about how long the Entry(), Action(), and
//      TransitionTest() functions of each state take to run.  If in state-based
//      scheduling mode, they call the DumpProcess() methods for all the states in
//      the state list to do so.  The version of this function which takes a file name
//      is intended to be called by the user program; the one which takes a pointer is
//      intended to be called by the task list (and above that, the process).  

void CTask::DumpProfile (const char* aFileName)
    {
    FILE* aFile;                            //  Handle of the file to which we write

    //  Attempt to open the file.  If it can't be opened, complain and exit
    if ((aFile = fopen (aFileName, "w")) == NULL)
        TR_Exit ("Unable to open file \"%s\" for task profile dump", aFileName);

    //  If the file was successfully opened, write to it and close it
    else
        {
        DumpProfile (aFile);
        fclose (aFile);
        }
    }

void CTask::DumpProfile (FILE* aFile)
    {
    CState *pCur;                           //  Pointer to the state currently dumping
    char Header[128];                       //  Place to store header lines for file

    //  If the first state in the list is NULL, this means we're in task based mode,
    //  so print out the Run() function's timing data
    if ((pCur = (CState*) GetHead ()) == NULL)
        {
        sprintf (Header, "----- Task \"%s\" Run Function ", Name);
        for (int Index = strlen (Header); Index < PAGE_WIDTH; Index++)
            strcat (Header, "-");
        fprintf (aFile, "%s\n", Header);
        RunProfiler->DumpProfile (aFile);
        }
    //  If in state-based mode, do the same state timing information print for the
    //  states.  Each state will print out Entry(), Action() and Run() information
    else
        {
        while (pCur != NULL)
            {
            pCur->DumpProfile (aFile, Name);
            pCur = (CState*) GetNext ();
            }
        }
    }


//-------------------------------------------------------------------------------------
//  Function:  DumpDurations
//      This function causes a file to be printed which contains a table of Run()
//      function execution times, if the task is in task-based execution mode.

void CTask::DumpDurations (const char* aFileName)
    {
    //  Attempt to open the file.  If it can't be opened, complain and exit
    FILE* aFile;
    if ((aFile = fopen (aFileName, "w")) == NULL)
        TR_Exit ("Unable to open file \"%s\" for task run times dump", aFileName);

    //  If the file was successfully opened, write to it and close it
    else
        {
        //  If the initial state's not NULL, there are states so we can't print table
        if (pInitialState != NULL)
            {
            fprintf (aFile, "ERROR!  Task \"%s\" is state based.\n", Name);
            fprintf (aFile, "        Run() function timing is not available\n");
            }
        else
            RunProfiler->DumpHistogram (aFile);
            
        fclose (aFile);
        }
    }


//-------------------------------------------------------------------------------------
//  Function:  DumpLatencies
//      This function causes a file to be printed which contains a table of latencies
//      as seen by the Run() function.  These indicate how long it has been since each
//      previous run of the Run() function.
//
//      OK, there's a little problem in that this function isn't yet implemented.  

void CTask::DumpLatencies (const char* aFileName)
    {
    }



//-------------------------------------------------------------------------------------
//  Friend function: TL_TraceLine (records pointers for a state-based task)
//      When the user calls this function, the name and state information for a given
//      transition are saved by the data logger object.  This information consists of:
//      - The current time
//      - A pointer to the process in which the transition occurred
//      - A pointer to the task
//      - State from which transition originated
//      - State to which transition transitioned

void TL_TraceLine (void *pTask, void *aFromState, void *aToState)
    {
    if (TheMaster->TraceLogger != NULL)
        {
        SaveLoggerData (TheMaster->TraceLogger, (double)GetTimeNowUnprotected (),
                     TheMaster->GetCurrent (), pTask, aFromState, aToState, -1, -1);
        }
    }


//-------------------------------------------------------------------------------------
//  Friend function: TL_TraceLine (records state state numbers for a task-based task)
//      When the user calls this function, the name and state information for a given
//      transition are saved by the data logger object.  This information consists of:
//      - The current time
//      - A pointer to the process in which the transition occurred
//      - A pointer to the task
//      - State from which transition originated
//      - State to which transition transitioned

void TL_TraceLine (void *pTask, long aFromState, long aToState)
    {
    if (TheMaster->TraceLogger != NULL)
        {
        SaveLoggerData (TheMaster->TraceLogger, (double)GetTimeNowUnprotected (),
                  TheMaster->GetCurrent(), pTask, NULL, NULL, aFromState, aToState);
        }
    }

