//*************************************************************************************
//  TR3_MSTR.CPP
//      This is the implementation file for the master scheduler object.  This object
//      is the one which the user sets up in his program.  It creates the timer, task
//      lists, and whatever else it needs to schedule the project.
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

#include <conio.h>
#include <dos.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <TranRun4.hpp>


//=====================================================================================
//  Class: CMaster
//      One instance of this class is created in each user program.  The user program
//      passes messages to this object to set up and run the real-time scheduler.
//=====================================================================================

//-------------------------------------------------------------------------------------
//  Constructor: CMaster (default constructor) 
//      This constructor creates a new CMaster object.  There should be just one for a 
//      single-computer project and one per separate computer for a multi-computer
//      project.  Here we allocate the task list objects and output files and such.

CMaster::CMaster (void) : CBasicList ()
    {
    //  Set default stop time so that the program will run forever (almost)
    StopTime = (real_time)9E99;

    //  Create the transition trace logger object.  The user can write the trace which
    //  has been recorded by the trace logger by calling DumpTrace().
    if ((TraceLogger = new CDataLogger (LOG_CIRCULAR, 1024)) == NULL)
        TR_Exit ("Unable to create transition trace logger object");

    //  Define trace log's 7 columns:  time, process, task, pointer to state from
    //  which we came, pointer to state to which we went, and state numbers of the
    //  from and to states
    TraceLogger->DefineData (7, LOG_DOUBLE, LOG_POINTER, LOG_POINTER, LOG_POINTER,
                             LOG_POINTER, LOG_LONG, LOG_LONG);

    //  Unless there's an error, this "I'm OK" message will be shown when Master stops
    ExitMessage = new CString (128);
    *ExitMessage = "Normal Exit from scheduler\n";
    }


//-------------------------------------------------------------------------------------
//  Destructor: ~CMaster
//      Shuts down the master scheduler.  Frees the memory it has been using for the
//      process list and timer.  Process list object automatically deletes processes.

CMaster::~CMaster (void)
    {
    //  Get rid of the trace logger, freeing its memory
    delete TraceLogger;

    //  If the exit message exists, it must be zapped now
    if (ExitMessage != NULL) delete ExitMessage;

    //  Now delete all of the processes which are under this scheduler
    for (void* pCur = GetHead (); pCur != NULL; pCur = GetNext ())
        delete (CProcess*)(GetCurrent ());

    //  Call the basic list destructor
    this->CBasicList::~CBasicList ();
    }


//-------------------------------------------------------------------------------------
//  Function:  DumpTrace
//      The user calls this function to dump (write) the transition logic trace data
//      to a file.  Note that an existing file of the same name will be overwritten.
//      This function also flushes the trace data logger so you can start tracing all
//      over again.

void CMaster::DumpTrace (const char* aFileName)
    {
    FILE* TraceFile;                        //  File to which T.L. trace is written
    unsigned Counter;                       //  Counts lines of TL trace data
    double aTime;                           //  Time when transition occurred
    long OldState;                          //  States are stored as long integers
    long NewState;                          //  and retreived into these variables
    void* pOldState;                        //  Pointers to T.L. states are
    void* pNewState;                        //  stored in the buffer buffer
    void* pTask;                            //  Task where transition occurred
    void* pProc;                            //  Points to transition's process

    if ((TraceFile = fopen (aFileName, "w")) == NULL)
        {
        TR_Message ("Warning:  Unable to open transition logic trace file \"%s\"",
            aFileName);
        }
    else
        {
        fprintf (TraceFile, "TranRun4 State Transition Logic Trace File \n\n");
        fprintf (TraceFile,
           "Time       Process           Task              From              To \n\n");

        //  Gotta write the transition information from the logger object to disk.
        for (Counter = TraceLogger->GetNumLines (); Counter > 0; Counter--)
            {
            GetLoggerData (TraceLogger, &aTime, &pProc, &pTask,
                           &pOldState, &pNewState, &OldState, &NewState);

            //  Make sure none of the pointers is NULL; if one is we're out of data
            if (pProc == NULL || pTask == NULL)
                break;

            //  If the from or to state pointer is NULL, the states were saved as
            //  longs because the task is running in task-based scheduling mode.  In
            //  that case, print out the state ID's as longs, not character strings
            if ((pOldState == NULL) && (pNewState == NULL))
                {
                fprintf (TraceFile, "%-10.4f %-17s %-17s %-17ld %-17ld\n",
                         aTime, (((CProcess*)pProc)->GetName ()),
                         (((CTask*)pTask)->GetName ()), OldState, NewState);
                }
            else
                {
                fprintf (TraceFile, "%-10.4f %-17s %-17s %-17s %-17s\n",
                         aTime, (((CProcess*)pProc)->GetName ()),
                         (((CTask*)pTask)->GetName ()),
                         (((CState*)pOldState)->GetName ()),
                         (((CState*)pNewState)->GetName ()));
                }
            }

        //  Reset trace data logger so the same data can be read again if necessary
        TraceLogger->Rewind ();

        //  Close that data file now
        fclose (TraceFile);
        }
    }


//-------------------------------------------------------------------------------------
//  Function:  DumpTraceNumbers
//      This function is used to write the transition logic trace in numerical form to
//      a file.  Note that an existing file of the same name will be overwritten.
//      This function also flushes the trace data logger so you can start tracing all
//      over again.

void CMaster::DumpTraceNumbers (const char* aFileName)
    {
    FILE* TraceFile;                        //  File to which T.L. trace is written
    unsigned Counter;                       //  Counts lines of TL trace data
    double aTime;                           //  Time when transition occurred
    void* pTask;                            //  Task where transition occurred
    void* pProc;                            //  Points to transition's process
    void* pOldState;                        //  Pointers to T.L. states are
    void* pNewState;                        //  stored in the buffer buffer
    long OldState;                          //  States in CTL_EXEC are represented
    long NewState;                          //  by long integers

    //  Make sure we can open the file; if we can't, complain
    if ((TraceFile = fopen (aFileName, "w")) == NULL)
        {
        TR_Message ("Warning:  Unable to open transition logic trace file \"%s\"",
            aFileName);
        }
    else
        {
        fprintf (TraceFile, "TranRun4 State Transition Logic Trace File \n\n");
        fprintf (TraceFile,
           "Time       Process                  Task     From     To \n\n");

        //  Gotta write the transition information from the logger object to disk
        for (Counter = TraceLogger->GetNumLines (); Counter > 0; Counter--)
            {
            GetLoggerData (TraceLogger, &aTime, &pProc, &pTask,
                           &pOldState, &pNewState, &OldState, &NewState);

            //  Make sure none of the pointers is NULL; if one is we're out of data
            if (pProc == NULL || pTask == NULL)
                break;

            //  If the from or to state pointer is NULL, the states were saved as
            //  longs because the task is running in task-based scheduling mode.  In
            //  that case, print out the state ID's as longs, not character strings
            if ((pOldState == NULL) && (pNewState == NULL))
                {
                fprintf (TraceFile, "%-10.4f %-17s %8d %8d %8d\n",
                        aTime, (((CProcess*)pProc)->GetName ()),
                        (((CTask*)pTask)->GetSerialNumber ()),
                        (((CState*)OldState)->GetSerialNumber ()),
                        (((CState*)NewState)->GetSerialNumber ()));
                }
            else
                {
                fprintf (TraceFile, "%-10.4f %-17s %-17d %-17ld %-17ld\n",
                         aTime, (((CProcess*)pProc)->GetName ()),
                         (((CTask*)pTask)->GetSerialNumber ()), OldState, NewState);
                }
            }

        //  Reset trace data logger so the same data can be read again if necessary
        TraceLogger->Rewind ();

        //  Close that data file now
        fclose (TraceFile);
        }
    }


//-------------------------------------------------------------------------------------
//  Function: AddProcess
//      This function creates a new process with the given name and inserts a pointer
//      to it into the process list.  It returns a pointer to the new process.

CProcess *CMaster::AddProcess (char *aName)
    {
    //  Create a new process by calling the CProcess constructor
    CProcess *pNew = new CProcess (aName);

    //  Call function to insert this thing into the list and return the pointer
    return (InsertProcess (pNew));
    }


//-------------------------------------------------------------------------------------
//  Function: InsertProcess
//      This inserts a process into the process list, given a pointer to that process.
//      Normally the user would use AddProcess(), which creates the process and
//      inserts it in the list.  It returns the pointer it was given.

CProcess *CMaster::InsertProcess (CProcess* aProc)
    {
    //  Call basic list's insert() method to insert this thing into the list
    CBasicList::Insert ((void*)aProc);
    return (aProc);
    }


//-------------------------------------------------------------------------------------
//  Function: SetTickTime 
//      This function sets the tick time of the master controller.  It checks the time 
//      in its argument, and if it's "reasonable" sends it to the timer object.  

void CMaster::SetTickTime (real_time aTickTime)
    {
    if ((aTickTime < (real_time)(1E-6)) || (aTickTime > (real_time)(1E6)))
        TR_Exit ("Tick time is not within bounds (1 usec to 1Msec)");

    TheTimer->Setup (aTickTime);
    }


//-------------------------------------------------------------------------------------
//  Function: SetStopTime 
//      Function to set the stopping time of the controller.  If the user supplied a 
//      reasonable value, the data field for ending time will be filled therewith.  

void CMaster::SetStopTime (real_time aStopTime)
    {
    if (aStopTime < (real_time)(1E-6))
        TR_Exit ("Scheduler stopping time is infinitesimal or zero");

    StopTime = aStopTime;
    }


//-------------------------------------------------------------------------------------
//  Function: Go
//      This function turns the scheduler "on" and starts the processes running.  In
//      single-threading modes, a scheduler runs again and again until stopping time.  
//      In multithreading modes, Go() calls the timer object to install ISRs which run 
//      a scheduler that takes care of timer interrupt, event, sample time, and such 
//      tasks.  Then the background runs only continuous tasks.

void CMaster::Go (void)
    {
    real_time TheTime = (real_time)0.0;         //  Saves current time read from timer


    //  Set status to GOING, until something in the program changes it
    Status = GOING;

    //  Check if the timer has been set up properly; if not, its delta time is ~0.0
    if (TheTimer->GetDeltaTime () < (real_time)1E-6)
        TR_Exit ("Tick time is too small; have you called SetupTimer()?");

    //  Start the timer.  It responds differently in different modes
    TheTimer->Go ();

    //  Now run the background tasks by repeatedly calling RunBackground() and any
    //  other functions which need to be run in the given real-time mode.  Exit the 
    //  loop when time is StopTime or someone called Stop() to set Status to not GOING 
    while (Status == GOING)
        {
        //  Call function to send run messages to tasks in the task list in sequence
        RunBackground ();

        //  Check the time; if time's up, say so, stop timer, and cause an exit
        if ((TheTime = GetTimeNowUnprotected ()) > StopTime)
            {
            Status = STOPPED;
            TheTimer->Stop ();
            *ExitMessage = "Normal scheduler exit at end time ";
            *ExitMessage << (double)TheTime << "\n";
            }
        }

    TR_Message (ExitMessage->GetString ());         //  Display exit message
    }


//-------------------------------------------------------------------------------------
//  Function: Stop
//      This function puts the scheduler into a stopped state.  It doesn't destroy any
//      data, so the scheduler can be restarted from where it left off.  

void CMaster::Stop (void)
    {
    Status = STOPPED;
    TheTimer->Stop ();
    }


//-------------------------------------------------------------------------------------
//  Function: RunBackground 
//      This function runs all the background tasks (tasks not called by ISR's) which 
//      are in all the processes in the process list.  In single-threading modes, this
//      actually runs all the task functions.

void CMaster::RunBackground (void)
    {
    CProcess* pProcess = (CProcess*) GetHead ();

    while (pProcess != NULL)
        {
        pProcess->RunBackground ();

        GetObjWith (pProcess);
        pProcess = (CProcess*) GetNext ();
        }
    }


//-------------------------------------------------------------------------------------
//  Function: RunForeground
//      This function is intended to be called by the interrupt service routine.  It
//      makes one pass through each process with the scheduler.  The meaning of 'one
//      pass' changes with the real-time mode you are employing.
//  Changes
//       9-02-95  JR   Added GetObjWith() call for re-entrance during RunForeground()  

void CMaster::RunForeground (void)
    {
    CProcess* pProcess = (CProcess*) GetHead ();
    while (pProcess != NULL)
        {
        pProcess->RunForeground ();
        GetObjWith (pProcess);
        pProcess = (CProcess*) GetNext ();
        }
    }


//-------------------------------------------------------------------------------------
//  Function:  ProfileOn and ProfileOff
//      These functions activate and deactivate execution-time profiling for all
//      processes in the list. 

void CMaster::ProfileOn (void)
    {
    CProcess *pCur;                     //  Pointer to each process in the list
    for (pCur = (CProcess*)GetHead (); pCur != NULL; pCur = (CProcess *)GetNext ())
        pCur->ProfileOn ();
    }

void CMaster::ProfileOff (void)
    {
    CProcess *pCur;
    for (pCur = (CProcess*)GetHead (); pCur != NULL; pCur = (CProcess *)GetNext ())
        pCur->ProfileOff ();
    }


//-------------------------------------------------------------------------------------
//  Function:  DumpProfiles
//      This function causes each process to print a set of execution-time profiles
//      to the given file.

void CMaster::DumpProfiles (const char* aFileName)
    {
    FILE* aFile;                            //  Handle of the file to which we write

    //  Attempt to open the file.  If it can't be opened, complain and exit
    if ((aFile = fopen (aFileName, "w")) == NULL)
        TR_Exit ("Unable to open file \"%s\" for task profile dump", aFileName);
    else
        {
        CProcess *pCur;                     //  Pointer to process currently dumping

        for (pCur = (CProcess*)GetHead (); pCur != NULL; pCur = (CProcess *)GetNext ())
            pCur->DumpProfiles (aFile);
        }
    //  Since this function opened the file, this function must close it
    fclose (aFile);
    }


//-------------------------------------------------------------------------------------
//  Function: TR_Exit
//      In order to create nice clean one-line error exits, call this function and
//      give it a text string.  Then, no matter what mode the program is running in,
//      this function will print a complaint message and cleanly (?) exit.  This func-
//      tion sets the Master's state to stopped (the timer will remove the ISR if it
//      has set up one) and sets the exit text to the complaint you supplied.

void TR_Exit (const char* aFormat, ...)
    {
    va_list Arguments;                          //  List of arguments of variable size 

    //  Begin the variable-argument processing stuff
    va_start (Arguments, aFormat);

    char* FmtBuf = new char[128];               //  Make buffer for format string
    char* ExitBuf = new char[256];              //  Another buffer for full string
    strcpy (FmtBuf, "*** ERROR: ");             //  Put "ERROR" message into it
    strcat (FmtBuf, aFormat);                   //  Add the user's format string
    vsprintf (ExitBuf, FmtBuf, Arguments);      //  Use format to make message

    //  Tell TheMaster to stop and give it the complaint message as exit text.
    TheMaster->Stop ();                         //  Halt the scheduler right now
    TheMaster->SetExitMessage (ExitBuf);        //  Set ExitBuf as exit message

    //  Delete those pesky error messsage objects
    DELETE_ARRAY FmtBuf;
    DELETE_ARRAY ExitBuf;

    //  Clean up after the variable-argument processing
    va_end (Arguments);
    }

