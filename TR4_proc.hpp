//*************************************************************************************
//  TR4_PROC.HPP
//      Header file for the process class which is used in the TranRun4 scheduler.
//
//  Copyright (c) 1994-1997 D.M.Auslander and J.R.Ridgely
//      May be used and distributed for any non-commercial purposes as long as this
//      copyright notice is included.
//
//  Version
//       1-30-95  JR   Original version
//       5-20-95  JR   File changed to TranRun3 name
//       7-30-95  JR   DumpStatus and DumpConfiguration given file names as arguments
//      12-21-96  JR   File changed to TranRun4 version
//*************************************************************************************

#ifndef TR4_PROC_HPP                        //  Variable to prevent multiple inclusions
    #define  TR4_PROC_HPP

//  We need forward declarations of the task lists so process can hold them
class CTaskList;
class CTimerIntTaskList;
class CPreemptiveTaskList;
class CContinuousTaskList;


//=====================================================================================
//  Class: CProcess
//      This is the object which runs a process.  Here we define a process as one 
//      set of tasks which are intended to run on one computer.  However, for purposes
//      of program development, more than one process may share a computer in some 
//      simulation modes.  The user's program creates this process object.  The master 
//      scheduler passes it messages to schedule and run the tasks in the process.  
//=====================================================================================

class CProcess
    {
    private:
        char *Name;                             //  Name of this process
        CTimerIntTaskList *TimerIntTasks;       //  Unitary interrupt tasks
        CPreemptiveTaskList *PreemptibleTasks;  //  Pre-emptible interrupt tasks
        CPreemptiveTaskList *BackgroundTasks;   //  Sample time and digital event tasks
        CContinuousTaskList *ContinuousTasks;   //  List of continuous tasks
        int TaskSerialNumber;                   //  Gives tasks in list their numbers

    public:
        CProcess (void) { }                     //  Default constructor - don't use it
        CProcess (const char*);                 //  Constructor which we use sets name
        ~CProcess (void);                       //  Virtual destructor called by list

        void DumpStatus (const char*);          //  Dump status of all tasks
        void DumpConfiguration (const char*);   //  Dump task configuration information
        void ProfileOn (void);                  //  Turn profiling on for all tasks
        void ProfileOff (void);                 //  Turn all tasks' profiling back off
        void DumpProfiles (const char*);        //  Dump info about how fast tasks ran
        void DumpProfiles (FILE*);              //  Same function as called by CMaster
        void RunBackground (void);              //  Run one sweep through task lists
        void RunForeground (void);              //  Run a sweep through ISR driven list
        const char *GetName (void)              //  Function returns pointer to name
            { return (Name); }                  //    of process in a character string

        //  The AddTask() methods create a new task object and add it into the task
        //  list for this process, returning a pointer to the newly created task.
        //  InsertTask() is called to put an already-created task into the task list.
        //  State objects are used with TranRun3, but not with CTL_EXEC
        CTask *AddTask (const char*, TaskType);
        CTask *AddTask (const char*, TaskType, int);
        CTask *AddTask (const char*, TaskType, real_time);
        CTask *AddTask (const char*, TaskType, int, real_time);

        CTask *InsertTask (CTask*);
        CTask *InsertTask (CTask&);
    };


//=====================================================================================
//  Class: CTaskList
//      This class contains the common items among linked lists intended for storing
//      all kinds of CTask objects (i.e. it has common code from the task lists below).
//=====================================================================================

class CTaskList : public CBasicList
    {
    public:
        CTaskList (void);                   //  Constructor does nearly nothing
        ~CTaskList (void);

        boolean RunAll (void);              //  Run all the tasks in sequence
        boolean RunOne (void);              //  Run one task at a time
        void DumpStatus (FILE*);            //  Print status dump of all tasks
        void DumpConfiguration (FILE*);     //  Print status dump of all tasks
        void ProfileOn (void);              //  Turn execution time on or off for 
        void ProfileOff (void);             //    all tasks in the task list
        void DumpProfiles (FILE*);          //  Print a dump of timing information
        CTask *Insert (CTask*);             //  Insert task at end of list
    };


//=====================================================================================
//  Class: CTimerIntTaskList
//      This class implements a type of linked list intended for storing CTask objects
//      which represent continuous tasks.  The CTimerIntTaskList class is also
//      designed to pass messages to the tasks in a list, for example a [run] message
//      telling the tasks to run their action functions if they find it's time to.  
//=====================================================================================

class CTimerIntTaskList : public CTaskList 
    {
    public:
        CTimerIntTaskList (void);                       //  Constructor does nothing
        ~CTimerIntTaskList (void);

        boolean CTimerIntTaskList::RunAll (void);       //  Runs all the tasks
    };


//=====================================================================================
//  Class: CPreemptiveTaskList
//      This class implements a list of pre-emptive tasks, that is sample time, event,
//      or preemptible interrupt-driven tasks.  The list is kept sorted by priority
//      and the [run] message causes preemptive scheduling to occur if we're in a
//      preemptive scheduling mode.
//=====================================================================================

class CPreemptiveTaskList : public CTaskList
    {
    public:
        CPreemptiveTaskList (void);         //  Default do-nothing contstructor 
        ~CPreemptiveTaskList (void);

        boolean RunAll (void);              //  Run all ready tasks in priority order 
        boolean RunOne (void);              //  Run highest priority task that's ready 
    };


//=====================================================================================
//  Class: CContinuousTaskList
//      This class implements a type of linked list intended for storing CTask objects
//      which represent continuous tasks.  The CContTaskList class can also pass mes-
//      sages such as a [run] message to the tasks in its list.
//=====================================================================================

class CContinuousTaskList : public CTaskList
    {
    public:
        CContinuousTaskList (void);         //  Constructor does nothing
        ~CContinuousTaskList (void);
    };


//-------------------------------------------------------------------------------------
//  Global Main Process Pointer
//      It is expected that every project will have one "main" process, so for conven-
//      ience we declare an external pointer to it here; it's created in TranRun4.cpp
extern CProcess* MainProcess;

#endif

