//*************************************************************************************
//  TR4_lite.hpp
//      This is the TranRun 4 Light scheduler source file.  The "light" version has an
//      extra wrapper around the TranRun4 framework which makes using it easier; the
//      same TranRun4 calls are still available as well.
//
//  Version
//       8-12-95  JRR  Original file made for BC++ 5.0 under Windows 95 or NT
//      12-21-96  JRR  Ported to TranRun4 from version 3 
//*************************************************************************************

#include <TR4_lite.hpp>                     //  Header file for any Tranrun4 project


//-------------------------------------------------------------------------------------
//  Functions:  Create...Task (without run function pointers)
//      These functions create task objects.  Each creates a task with a different
//      real-time mode.  These tasks will expect to be given states in which to run.
//      A pointer to the newly created task is returned by each function in case the
//      user needs to use that pointer to call the task's member functions.

CTask* CreateTimerIntTask (char* aName, real_time aTime)
    {
    CTask* TheTask = new CTask (aName, TIMER_INT, aTime);
    MainProcess->InsertTask ((CTask*)TheTask);
    return TheTask;
    }

CTask* CreatePreemptibleTask (char* aName, int aPri, real_time aTime)
    {
    CTask* TheTask = new CTask (aName, PREEMPTIBLE, aPri, aTime);
    MainProcess->InsertTask ((CTask*)TheTask);
    return TheTask;
    }

CTask* CreateSampleTimeTask (char* aName, int aPri, real_time aTime)
    {
    CTask* TheTask = new CTask (aName, SAMPLE_TIME, aPri, aTime);
    MainProcess->InsertTask ((CTask*)TheTask);
    return TheTask;
    }

CTask* CreateEventTask (char* aName, int aPri)
    {
    CTask* TheTask = new CTask (aName, SAMPLE_TIME, aPri);
    MainProcess->InsertTask ((CTask*)TheTask);
    return TheTask;
    }

CTask* CreateContinuousTask (char* aName)
    {
    CTask* TheTask = new CTask (aName, CONTINUOUS);
    MainProcess->InsertTask ((CTask*)TheTask);
    return TheTask;
    }


//-------------------------------------------------------------------------------------
//  Functions:  Create...Task (with run function pointers)
//      These functions also create task objects, but instead of creating tasks to be
//      given state lists, these tasks are given run function pointers right away.
//      A pointer to the newly created task is returned by each function in case the
//      user needs to use that pointer to call the task's member functions.

CTask* CreateTimerIntTask (char* aName, real_time aTime, long (*aFunc)(long))
    {
    CTask* TheTask = new C_Lite_Task (aName, TIMER_INT, aTime, aFunc);
    MainProcess->InsertTask ((CTask*)TheTask);
    return TheTask;
    }

CTask* CreatePreemptibleTask (char* aName, int aPri, real_time aTime,
                              long (*aFunc)(long))
    {
    CTask* TheTask = new C_Lite_Task (aName, PREEMPTIBLE, aPri, aTime, aFunc);
    MainProcess->InsertTask ((CTask*)TheTask);
    return TheTask;
    }

CTask* CreateSampleTimeTask (char* aName, int aPri, real_time aTime,
                             long (*aFunc)(long))
    {
    CTask* TheTask = new C_Lite_Task (aName, SAMPLE_TIME, aPri, aTime, aFunc);
    MainProcess->InsertTask ((CTask*)TheTask);
    return TheTask;
    }

CTask* CreateEventTask (char* aName, int aPri, long (*aFunc)(long))
    {
    CTask* TheTask = new C_Lite_Task (aName, EVENT, aPri, aFunc);
    MainProcess->InsertTask ((CTask*)TheTask);
    return TheTask;
    }

CTask* CreateContinuousTask (char* aName, long (*aFunc)(long))
    {
    CTask* TheTask = new C_Lite_Task (aName, CONTINUOUS, aFunc);
    MainProcess->InsertTask ((CTask*)TheTask);
    return TheTask;
    }


//-------------------------------------------------------------------------------------
//  Function:  CreateState
//      This function will create a state object, giving it a name and assigning it to
//      a parent task.  The state will call the entry, action and transition test
//      functions given by the function pointers in the argument list.  A pointer to
//      the newly created state is returned in case the user wants to call the state's
//      member functions.

CState* CreateState (char* aName, CTask* aParentTask, StateInit aInitialState,
    void (*aEntryFunc)(void), void (*aActionFunc)(void), CState* (*aTestFunc)(void))
    {
    //  Call constructor to create a new state and save a pointer to that state
    CState* TheState;
    TheState = (CState*) new C_Lite_State (aName, aEntryFunc, aActionFunc, aTestFunc);

    //  Tell the parent task that it has a new baby state to take care of
    aParentTask->InsertState (TheState);

    //  If the user specified that this is the initial state, tell the task it is so
    if (aInitialState == IS_INITIAL_STATE)  aParentTask->SetInitialState (TheState);

    //  Return a pointer to the state we just created
    return TheState;
    }


//=====================================================================================
//  Class:  C_Lite_Task
//      This class provides a wrapper for tasks used in the TranRun4 Light scheduler.
//      Task execution is always unitary.  Tasks' Run() methods call functions 
//      supplied by the user when CreateTask() is called.  In essence, we wind up with
//      an easy-to-use scheduler that just runs the user's task functions on time.
//=====================================================================================

//-------------------------------------------------------------------------------------
//  Constructors:  C_Lite_Task
//      These constructors create task objects for the various types of tasks which
//      are defined under TranRun4.

C_Lite_Task::C_Lite_Task (char* aName, TaskType aType, real_time aTime,
                          long (*aFunc)(long))
    : CTask (aName, aType, aTime)
    {
    RunFunc = aFunc;
    WillRunAgain = false;
    }

C_Lite_Task::C_Lite_Task (char* aName, TaskType aType, int aPri, real_time aTime,
                          long (*aFunc)(long))
    : CTask (aName, aType, aPri, aTime)
    {
    RunFunc = aFunc;
    WillRunAgain = false;
    }

C_Lite_Task::C_Lite_Task (char* aName, TaskType aType, int aPri, long (*aFunc)(long))
    : CTask (aName, aType, aPri)
    {
    RunFunc = aFunc;
    WillRunAgain = false;
    }

C_Lite_Task::C_Lite_Task (char* aName, TaskType aType, long (*aFunc)(long))
    : CTask (aName, aType)
    {
    RunFunc = aFunc;
    WillRunAgain = false;
    }


//-------------------------------------------------------------------------------------
//  Destructor:  ~C_Lite_Task
//      This destructor just calls that of its ancestor, CTask.

C_Lite_Task::~C_Lite_Task (void)
    {
    }


//-------------------------------------------------------------------------------------
//  Function:  Run
//      The run function of a CTask object is overloaded here; this prevents the
//      state-based scheduler from running on this task and instead causes the user's
//      run function to be called when this task runs.

void C_Lite_Task::Run (void)
    {
    if (RunFunc != NULL)
        State = (*RunFunc)(State);
    else
        TR_Exit ("No run function provided for task \"%s\"", Name);

    if (WillRunAgain == true)
        WillRunAgain = false;               //  If the run-again flag is set, we must
    else                                    //  run the Run() function again during
        Idle ();                            //  this sample time, so don't call Idle()
    }


//-------------------------------------------------------------------------------------
//  Function:  RunAgain
//      This function is called when the task will need to run its Action function
//      again during the current sample time.  It prevents Idle() from being called.

void C_Lite_Task::RunAgain (void)
    {
    WillRunAgain = true;
    }



//=====================================================================================
//  Class:  C_Lite_State
//      This class provides an easy-to-use interface for creating states which are to
//      be placed in a TranRun4 Light task's state list.  Each state has an entry,
//      action and test function which will be run by the scheduler at the appropriate
//      times.
//=====================================================================================

//-------------------------------------------------------------------------------------
//  Constructor:  C_Lite_State
//      This constructor is given a task name, sample time, priority, and the task
//      function which is to be run every so-and-so milliseconds.  It creates a new
//      task object, calling the CTask constructor to set it up.

C_Lite_State::C_Lite_State (const char* aName, void (*aEntryFunc)(void),
               void (*aActionFunc)(void), CState* (*aTestFunc)(void)) : CState (aName)
    {
    //  This state won't be much use if there are no run functions; if so, complain
    if ((aEntryFunc == NULL) && (aActionFunc == NULL) && (aTestFunc == NULL))
        {
        TR_Message ("ERROR:  No Entry, Action, or Test function for state \"%s\" ",
                    aName);
        exit (2);
        }

    //  Set the state's run function pointers to whatever the user is pointing at.
    //  If the pointers are NULL, this means there's no function of that type 
    EntryFunc = aEntryFunc;
    ActionFunc = aActionFunc;
    TestFunc = aTestFunc;
    }


//-------------------------------------------------------------------------------------
//  Destructor:  ~C_Lite_State
//      This destructor just deletes the state object.

C_Lite_State::~C_Lite_State (void)
    {
    }


//-------------------------------------------------------------------------------------
//  Function:  Entry
//      This is an overload of the Entry() function for the CState class.  This run
//      function runs the task function which has been supplied by the user and then
//      idles the task until the next sample time.

void C_Lite_State::Entry (void)
    {
    if (EntryFunc != NULL) (*EntryFunc)();  //  Run user's task function if it exists
    Idle ();                                //  Idle the task until next sample time
    }


//-------------------------------------------------------------------------------------
//  Function:  Action
//      This is the overloaded Action() function for the CState class.  It works the
//      same way as Entry(), only usually more often.

void C_Lite_State::Action (void)
    {
    if (ActionFunc != NULL) (*ActionFunc)();
    Idle ();
    }


//-------------------------------------------------------------------------------------
//  Function:  TransitionTest
//      Finally we present the overloaded TransitionTest() function.  This function
//      runs the user's transition test function, which returns a pointer to the next
//      state to which we'll transition (or NULL if there will be no transition).
//      The next state pointer is returned by this function to the scheduler.

CState* C_Lite_State::TransitionTest (void)
    {
    if (TestFunc != NULL)  return (*TestFunc)();
    else                   return NULL;
    }

