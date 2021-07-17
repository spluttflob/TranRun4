//*************************************************************************************
//  TR4_PROC.CPP
//      This is the implementation file for the process object.  This object contains 
//      task lists and whatever else it needs to schedule the project.  
//
//  Copyright (c) 1994-1997, D.M.Auslander and J.R.Ridgely
//      May be used and distributed for any non-commercial purposes as long as this 
//      copyright notice is included.  
//      Original program 11-94 by DMA 
//      11-21-94  JR   New class structure
//      12-19-94  JR   Spun off as separate implementation file
//       5-20-95  JR   Changed to TranRun3 project
//      12-21-96  JR   Changed to TranRun4 project (another sequel) 
//*************************************************************************************

#include <conio.h>
#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "TranRun4.hpp"

//=====================================================================================
//  Class: CProcess
//      This is the scheduler which runs a process.  Here we define a process as one 
//      set of tasks which are intended to run on one computer.  However, for purposes 
//      of program development, more than one process may share a computer in some 
//      simulation modes.  The master scheduler creates this process object and passes 
//      it messages to set up and schedule the running of run its process. 
//=====================================================================================

//-------------------------------------------------------------------------------------
//  Constructor:  CProcess (Default constructor)
//      If no parameters are given to this constructor, it creates a process with all
//      empty task lists

CProcess::CProcess (const char* aName)
    {
    //  Make some space and save the name of this object
    if ((Name = new char[strlen (aName) + 1]) != NULL)
        strcpy (Name, aName);

    //  The serial number given to inserted tasks begins at zero
    TaskSerialNumber = 0;

    //  Create and allocate space for new task lists; give them pointer to this object
    TimerIntTasks = new CTimerIntTaskList ();
    PreemptibleTasks = new CPreemptiveTaskList ();
    BackgroundTasks = new CPreemptiveTaskList ();
    ContinuousTasks = new CContinuousTaskList ();
    }


//-------------------------------------------------------------------------------------
//  Destructor:  ~CProcess
//      This destructor just frees up the memory which has been used by this object.

CProcess::~CProcess (void)
    {
    delete TimerIntTasks;           //  Delete the task lists
    delete PreemptibleTasks;
    delete BackgroundTasks;
    delete ContinuousTasks;

    DELETE_ARRAY Name;              //  Zap the name string
    }


//-------------------------------------------------------------------------------------
//  Function: DumpConfiguration
//      This function writes a "configuration dump" for a process.  The idea is that it
//      writes one line for each task wth name, type, priority, timing and so on and
//      puts state names in a column at the right of that line.  This way we get an
//      easy-to-read matrix of task and state information.

void CProcess::DumpConfiguration (const char* aName)
    {
    FILE *DumpFile;                             //  Handle of file to which to dump


    //  If a valid name was given, open the file, write-only, overwriting any old file
    if (aName == NULL)
        return;

    //  Create a new file object to which to write the dump
    if ((DumpFile = fopen (aName, "w")) != NULL)
        {
        fprintf (DumpFile, "\nConfiguration of Tasks in Process \"%s\"\n\n", Name);
        fprintf (DumpFile,
                 "Task                                            Sample  State\n");
        fprintf (DumpFile,
                 "Name                     Type       Priority    Time    Names\n\n");

        //  Each list of tasks must be asked to dump its configurations
        TimerIntTasks->DumpConfiguration (DumpFile);
        PreemptibleTasks->DumpConfiguration (DumpFile);
        BackgroundTasks->DumpConfiguration (DumpFile);
        ContinuousTasks->DumpConfiguration (DumpFile);

        fprintf (DumpFile, "\n\n");             //  Write some returns at the end
        fclose (DumpFile);                      //  of the file and then close it
        }
    }


//-------------------------------------------------------------------------------------
//  Function:  DumpStatus
//      This function writes a status dump which shows what all processes are doing.
//      It does so by calling the DumpStatus() methods for all the task lists.

void CProcess::DumpStatus (const char* aName)
    {
    FILE* StatusFile;                           //  Handle of file where the dump goes


    //  If a valid name was given, open the file, write-only, overwriting the old file
    if (aName == NULL)
        return;

    //  Create a new file object to which to write the dump
    if ((StatusFile = fopen (aName, "w")) != NULL)
        {
        fprintf (StatusFile, "Status Dump for Tasks in Process \"%s\"\n", Name);

        TimerIntTasks->DumpStatus (StatusFile);         //  Ask the lists of tasks
        PreemptibleTasks->DumpStatus (StatusFile);      //  to dump all their statuses
        BackgroundTasks->DumpStatus (StatusFile);
        ContinuousTasks->DumpStatus (StatusFile);

        fclose (StatusFile);
        }
    }


//-------------------------------------------------------------------------------------
//  Functions:  ProfileOn and ProfileOff
//      These functions activate and deactivate execution-time profiling for all
//      task lists belonging to this process.  

void CProcess::ProfileOn (void)
    {
    TimerIntTasks->ProfileOn ();
    PreemptibleTasks->ProfileOn ();
    BackgroundTasks->ProfileOn ();
    ContinuousTasks->ProfileOn ();
    }

void CProcess::ProfileOff (void)
    {
    TimerIntTasks->ProfileOff ();
    PreemptibleTasks->ProfileOff ();
    BackgroundTasks->ProfileOff ();
    ContinuousTasks->ProfileOff ();
    }


//-------------------------------------------------------------------------------------
//  Functions:  DumpProfiles
//      This function writes information about how long the tasks in a process take to
//      run.  It writes a header and then calls the DumpProfile() method for each
//      task and state; these methods will write execution time information for the
//      Run() or Entry(), Action(), and TransitionTest() functions.  The first func-
//      tion is meant to be called directly, the second by CMaster::DumpProfiles().

void CProcess::DumpProfiles (const char* aFileName)
    {
    FILE* aFile;                            //  Handle of the file to which we write

    //  Attempt to open the file.  If it can't be opened, complain and exit
    if ((aFile = fopen (aFileName, "w")) == NULL)
        TR_Exit ("Unable to open file \"%s\" for process profile dump", aFileName);
    //  If the file was opened, write the data to it and close it  
    else
        DumpProfiles (aFile);
        fclose (aFile);
    }

void CProcess::DumpProfiles (FILE* aFile)
    {
    #if (defined (TR_TIME_FREE) || defined (TR_THREAD_MULTI))
        fprintf (aFile, "\nTiming information for tasks in process \"%s\"\n\n", Name);

        //  Each list of tasks must be asked to dump its timing information
        TimerIntTasks->DumpProfiles (aFile);
        PreemptibleTasks->DumpProfiles (aFile);
        BackgroundTasks->DumpProfiles (aFile);
        ContinuousTasks->DumpProfiles (aFile);

    #else                                   //  In case we can't do timing...
        fprintf (aFile, "Function timing information is not available.\n");
        fprintf (aFile, "In order to measure function timing, you must use\n");
        fprintf (aFile, "a high-resolution timer or multithreading mode.\n\n");
    #endif
    }


//-------------------------------------------------------------------------------------
//  Function:  RunBackground
//      This function runs one 'sweep' through those tasks which are not called by the
//      interrupt service routine.  It works differently in different real-time modes:
//        - In sequential modes, runs all tasks in order, then increments time.
//        - In low-latency modes, it runs tasks a few at a time according to priority.
//        - In interrupt modes, it runs continuous tasks only, because the other tasks
//          are being called from the interrupt service routine.  

void CProcess::RunBackground (void)
    {
    //  If in single-thread, sequential simulation mode: just run all tasks in order 
    #if defined (TR_EXEC_SEQ) && defined (TR_THREAD_SINGLE)
        TimerIntTasks->RunAll ();
        PreemptibleTasks->RunAll (); 
        BackgroundTasks->RunAll ();
        ContinuousTasks->RunAll ();
    #endif

    //  If in single-thread, minimum-latency mode: run all timer tasks, then run one
    //  preemptible task which is ready to go.  If no preemptible task ran, run the
    //  next available background or continuous task
    #if defined (TR_EXEC_MIN) && defined (TR_THREAD_SINGLE)
        TimerIntTasks->RunAll ();
        if (PreemptibleTasks->RunOne () == FALSE)
            {
            if (BackgroundTasks->RunOne () == FALSE)
                ContinuousTasks->RunOne ();
            }
    #endif

    //  Multithreading:  Interrupts run timer-int and preemptible tasks only.  In the
    //  background, run one pre-emptible background task, or if none is ready to go,
    //  run a continuous task 
    #if defined (TR_THREAD_MULTI)
        if (BackgroundTasks->RunOne () == FALSE)
            ContinuousTasks->RunOne ();
    #endif

    //  Externally controlled simulation: nope, we haven't got it yet
    #if defined (TR_TIME_EXTSIM)
        TR_Exit ("External simulation mode not yet implemented");
    #endif
    }


//-------------------------------------------------------------------------------------
//  Function:  RunForeground
//      This function runs the foreground tasks, i.e. those which are called by the 
//      interrupt service routine.  In multithreading mode, this means that it first
//      gives all the timer interrupt tasks a call, and they run if their sample time
//      has come; then it runs the highest priority preemptible task.

void CProcess::RunForeground (void)
    {
    //  Multithreading:  Interrupts run timer interrupt and preemptible tasks
    #if defined (TR_THREAD_MULTI)
        TimerIntTasks->RunAll ();       //  This calls all timer interrupt tasks
        PreemptibleTasks->RunAll ();    //  This runs high priority preemptible tasks
    #endif
    }


//-------------------------------------------------------------------------------------
//  Functions:  AddTask (versions for each task type)
//      This function creates a new task of the type specified by the user and calls
//      InsertTask() to put it into the process's task list.  Several overloaded ver-
//      sions of this method are given because the functions which create different
//      types of tasks (timer interrupt, continuous, etc.) take different numbers
//      and types of parameters.  The CTL_EXEC version of this program does not permit 
//      adding tasks in this manner because there are no state objects to run in them.

//  This version creates a continuous task and takes no timing parameters
CTask *CProcess::AddTask (const char *aName, TaskType aType)
    {
    CTask *pNewTask;                            //  Pointer to the task we create here

    pNewTask = new CTask (aName, aType);            //  Create the task object
    ContinuousTasks->Insert (pNewTask);             //  Insert it in the task list
    pNewTask->SetSerialNumber (TaskSerialNumber++); //  Give the task a serial number
    return (pNewTask);                              //  Return a pointer to it
    }

//  This one is for a timer-interrupt task which takes a time but no priority
CTask *CProcess::AddTask (const char *aName, TaskType aType, real_time aTime)
    {
    CTask *pNewTask;

    pNewTask = new CTask (aName, aType, aTime);
    TimerIntTasks->Insert (pNewTask);
    pNewTask->SetSerialNumber (TaskSerialNumber++);
    return (pNewTask);
    }

//  This version is for an event task which takes a proirity but no timing information
CTask *CProcess::AddTask (const char *aName, TaskType aType, int aPriority)
    {
    CTask *pNewTask;

    pNewTask = new CTask (aName, aType, aPriority);
    BackgroundTasks->Insert (pNewTask);
    pNewTask->SetSerialNumber (TaskSerialNumber++);
    return (pNewTask);
    }

//  This method creates and inserts a sample time task which needs timing and priority
CTask *CProcess::AddTask (const char *aName, TaskType aType, int aPriority,
                          real_time aTime)
    {
    CTask *pNewTask;

    pNewTask = new CTask (aName, aType, aPriority, aTime);
    if (aType == PREEMPTIBLE)  PreemptibleTasks->Insert (pNewTask);
    if (aType == SAMPLE_TIME)  BackgroundTasks->Insert (pNewTask);
    pNewTask->SetSerialNumber (TaskSerialNumber++);
    return (pNewTask);
    }


//-------------------------------------------------------------------------------------
//  Function:  InsertTask (versions which take a task pointer or a task reference)
//      These functions insert the given task into the appropriate task list.  The task
//      object must already have been instantiated.  These functions are usually used
//      to insert custom task objects of a type descended from CTask into a task list.

CTask *CProcess::InsertTask (CTask* pTask)
    {
    switch (pTask->TheType)
        {
        case (TIMER_INT):
            TimerIntTasks->Insert (pTask);
            break;
        case (PREEMPTIBLE):
            PreemptibleTasks->Insert (pTask);
            break;
        case (SAMPLE_TIME):
        case (EVENT):
            BackgroundTasks->Insert (pTask);
            break;
        case (CONTINUOUS):
            ContinuousTasks->Insert (pTask);
            break;
        default:
            TR_Exit ("Attempt to insert task \"%s\" of unknown type into process",
                     pTask->GetName ());
        }

    //  This line gives the task a uniqu serial number
    pTask->SetSerialNumber (TaskSerialNumber++);

    return (pTask);
    }


CTask *CProcess::InsertTask (CTask& aTask)
    {
    return (InsertTask (&aTask));
    }



//=====================================================================================
//  Class: CTaskList
//      This class contains the common items among linked lists intended for storing 
//      all kinds of CTask objects (i.e. it has common code from the task lists below).
//=====================================================================================

//-------------------------------------------------------------------------------------
//  Constructor: CTimerIntTaskList
//      This constructor is just the default sort.

CTaskList::CTaskList (void) : CBasicList ()
    {
    }


//-------------------------------------------------------------------------------------
//  Destructor: ~CTaskList
//      This destructor zaps all the tasks in the list.  These tasks actually have
//      virtual destructors, so the user's tasks' destructors get called.

CTaskList::~CTaskList (void)
    {
    for (CTask* pCur = (CTask*)GetHead (); pCur != NULL; pCur = (CTask*)GetNext ())
        delete pCur;
    }


//-------------------------------------------------------------------------------------
//  Function: RunAll
//      This function passes on a [run] message which it has received from the main 
//      scheduler object.  Each of the tasks in the list is given the message to "go
//      run yourself."  

boolean CTaskList::RunAll (void)
    {
    CTask* pCurTask;                    //  Pointer to the task currently being run


    //  Send a run message to each task in the list, in sequence
    pCurTask = (CTask*) GetHead ();
    while (pCurTask != NULL)
        {
        pCurTask->Schedule ();
        pCurTask = (CTask*) GetNext ();
        }

    return (TRUE);
    }


//-------------------------------------------------------------------------------------
//  Function: RunOne
//      This function acts on a run message from the main scheduler by sending just 
//      one task in the list a run-yourself message.  RunOne messages are sequenced 
//      through the list.  

boolean CTaskList::RunOne (void)
    {
    CTask* pCurTask;                        //  Pointer to the task currently being run


    pCurTask = (CTask*) GetNext ();         //  See if there's a next task available

    if (pCurTask == NULL)                   //  If we were at end of task list, wrap
        pCurTask = (CTask*) GetHead ();     //  back around to the beginning

    if (pCurTask != NULL)
        pCurTask->Schedule ();              //  Now run the task (if there is one) 

    return (TRUE);                          //  If we get here, no task has run 
    }


//-------------------------------------------------------------------------------------
//  Function: Insert 
//      This function inserts the task which is pointed to by its argument into the 
//      list of timer interrupt tasks.  

CTask *CTaskList::Insert (CTask* pNew)
    {
    //  Call function to insert this task into the list; tell it its parent's address 
    CBasicList::Insert ((void*)pNew, pNew->GetPriority ());

    return (pNew);
    }


//-------------------------------------------------------------------------------------
//  Function: DumpConfiguration
//      This function writes a configuration dump to the stream given in its argument.
//      It does so by calling the DumpStatus() methods for tasks in the list.

void CTaskList::DumpConfiguration (FILE* aFile)
    {
    CTask *pCur;                            //  Pointer to the task currently dumping

    for (pCur = (CTask *)GetHead (); pCur != NULL; pCur = (CTask *)GetNext ())
        pCur->DumpConfiguration (aFile);
    }


//-------------------------------------------------------------------------------------
//  Function: DumpStatus
//      This function writes a task list status dump to the stream given in its argu-
//      ment.  It does so by calling the DumpStatus() methods for tasks in the list.

void CTaskList::DumpStatus (FILE* aFile)
    {
    CTask *pCur;                            //  Pointer to the task currently dumping

    for (pCur = (CTask*)GetHead (); pCur != NULL; pCur = (CTask *)GetNext ())
        pCur->DumpStatus (aFile);
    }


//-------------------------------------------------------------------------------------
//  Functions:  ProfileOn and ProfileOff
//      These functions activate and deactivate execution-time profiling for all
//      tasks in the list. 

void CTaskList::ProfileOn (void)
    {
    CTask *pCur;
    for (pCur = (CTask*)GetHead (); pCur != NULL; pCur = (CTask *)GetNext ())
        pCur->ProfileOn ();
    }

void CTaskList::ProfileOff (void)
    {
    CTask *pCur;
    for (pCur = (CTask*)GetHead (); pCur != NULL; pCur = (CTask *)GetNext ())
        pCur->ProfileOff ();
    }


//-------------------------------------------------------------------------------------
//  Function: DumpTimingInfo
//      This function writes information about how long task functions take to run
//      to the stream given in its argument.  It does so by calling DumpProfile()
//      methods for tasks in the list.

void CTaskList::DumpProfiles (FILE* aFile)
    {
    CTask *pCur;                            //  Pointer to the task currently dumping

    for (pCur = (CTask*)GetHead (); pCur != NULL; pCur = (CTask *)GetNext ())
        pCur->DumpProfile (aFile);
    }


//=====================================================================================
//  Class: CTimerIntTaskList
//      This class implements a type of linked list intended for storing CTask objects 
//      which represent timer interrupt tasks.  
//=====================================================================================

//-------------------------------------------------------------------------------------
//  Constructor: CTimerIntTaskList
//      This constructor is just the default sort.  

CTimerIntTaskList::CTimerIntTaskList (void)
    {
    }


//-------------------------------------------------------------------------------------
//  Destructor: ~CTimerIntTaskList
//      This destructor frees the memory which has been taken up by all the task 
//      objects in the task list.   

CTimerIntTaskList::~CTimerIntTaskList (void)
    {
    this->CTaskList::~CTaskList ();
    }



//-------------------------------------------------------------------------------------
//  Function: RunAll
//      This function passes on a [run] message which it has received from the main 
//      scheduler object.  Each of the tasks in the list is given the message to "go
//      run yourself."
//  Changes
//       8-17-95  JR   Each task is run repeatedly until it idles itself
//       9-02-95  JR   GetObjWith() added in case list is changed during re-entrance
//                     of this task if we were interrupted during pCurTask->Schedule()

boolean CTimerIntTaskList::RunAll (void)
    {
    CTask *pCurTask;                    //  Pointer to the task currently being run
    TaskStatus RetStatus;               //  Status returned by a task which ran

    //  Send a run message to each task in the list, in sequence.  Each task will get
    //  as many scans as it needs, i.e. we run it until it idles itself
    pCurTask = (CTask *) GetHead ();
    while (pCurTask != NULL)
        {
        //  Call the task's Schedule() method, which runs the Entry() and/or Action()
        //  functions, repeatedly until the task becomes idle or deactivated
        do
            {
            //  Run the task by calling the task's Schedule() method
            RetStatus = pCurTask->Schedule ();
            pCurTask->GetSampleTime ();

            //  If this task interrupted itself, it's too slow so complain and exit
            if (RetStatus == TS_RUNNING)
                {
                TR_Exit ("Timer interrupt task \"%s\" has interrupted itself",
                         pCurTask->GetName ());
                return (FALSE);
                }
            }
        while ((RetStatus != TS_IDLE) && (RetStatus != TS_DEACTIVATED));

        GetObjWith (pCurTask);
        pCurTask = (CTask *) GetNext ();
        }

    return (TRUE);
    }


//=====================================================================================
//  Class: CPreemptiveTaskList
//      This class implements a type of linked list intended for storing CTask objects 
//      which represent timer interrupt tasks.  
//=====================================================================================

//-------------------------------------------------------------------------------------
//  Constructor: CTimerIntTaskList
//      This is just a default constructor.  

CPreemptiveTaskList::CPreemptiveTaskList (void) 
    {
    }


//-------------------------------------------------------------------------------------
//  Destructor: ~CPreemptiveTaskList
//      This destructor frees the memory which has been taken up by all the task 
//      objects in the task list.   

CPreemptiveTaskList::~CPreemptiveTaskList (void)
    {
    this->CTaskList::~CTaskList ();
    }


//-------------------------------------------------------------------------------------
//  Function: RunAll
//      This function acts on a Run message from the process scheduler.  It runs all
//      the tasks which are ready to run, beginning with the highest priority one and
//      working down to the bottom - unless it encounters a task which is already
//      running, in which case it quits and returns control to the thread in which
//      that already running task is running.
//  Changes
//       9-02-95  JR   GetObjWith() added in case list is changed during re-entrance
//                     of this task if we were interrupted during pCurTask->Schedule()

boolean CPreemptiveTaskList::RunAll (void)
    {
    CTask *pCurTask;                        //  Pointer to the task currently being run
    TaskStatus RetStatus;                   //  Status returned by a task which ran 


    //  Begin with the first task in the list; it has the highest priority.  Run it.  
    //  If this task is already running or has been preempted, exit; otherwise, after 
    //  this task is done go on to the next task which might need a chance to run too 
    pCurTask = (CTask*) GetHead ();
    while (pCurTask != NULL)
        {
        RetStatus = pCurTask->Schedule ();
        if ((RetStatus == TS_RUNNING) || (RetStatus == TS_PREEMPTED))
            break;
        GetObjWith (pCurTask);
        pCurTask = (CTask *) GetNext ();
        }

    return (TRUE);
    }


//-------------------------------------------------------------------------------------
//  Function: RunOne
//      This function runs the highest priority ready-to-go task at any given time, if
//      there actually is a task ready to run.  It returns FALSE if no task was found 
//      which was ready to run and TRUE if a task did run.

boolean CPreemptiveTaskList::RunOne (void)
    {
    CTask *pCurTask;                        //  Pointer to the task currently being run
    TaskStatus RetStatus;                   //  Status returned by a task which ran 


    //  Begin with the first task in the list; it has the highest priority.  Send a 
    //  Schedule message to each task until one of them actually runs 
    pCurTask = (CTask*) GetHead ();
    while (pCurTask != NULL)
        {
        RetStatus = pCurTask->Schedule ();
        if (RetStatus == TS_READY)          //  The TS_READY return tells us the task
            return (TRUE);                  //  ran; another code means it didn't 
        pCurTask = (CTask *) GetNext ();
        }

    return (FALSE);                         //  If we get here, no task function ran 
    }


//=====================================================================================
//  Class: CContinuousTaskList
//      This class implements a type of linked list intended for storing CTask objects 
//      which represent timer interrupt tasks.  
//      The CTaskList class is also designed to pass messages to the tasks in a list,
//      for example a [run] message telling the tasks to run their action functions.  
//=====================================================================================

//-------------------------------------------------------------------------------------
//  Constructor: CTimerIntTaskList
//      Well, as one can see, this doesn't do much in the current incarnation.

CContinuousTaskList::CContinuousTaskList ()
    {
    }


//-------------------------------------------------------------------------------------
//  Destructor: ~CContinuousTaskList
//      This destructor frees the memory which has been taken up by all the task
//      objects in the task list.

CContinuousTaskList::~CContinuousTaskList (void)
    {
    this->CTaskList::~CTaskList ();
    }


