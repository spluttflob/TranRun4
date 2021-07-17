//*************************************************************************************
//  TR4_stat.cpp
//      This is the implementation for the Transition Logic state class used in the
//      TranRun4 project.
//
//  Copyright (c) 1994-1996, D.M.Auslander and J.R.Ridgely
//      May be used and distributed for any non-commercial purposes as long as this
//      copyright notice is included.
//
//  Version
//       5-22-95  JR   Original version
//      12-21-96  JR   Changed to TranRun4 version
//*************************************************************************************

#include <stdlib.h>
#include <string.h>
#include <TranRun4.hpp>


//=====================================================================================
//  Class: CState
//      This class implements a transition logic state.  A task contains a list of
//      states; each state must have at least one of the following:
//        - Entry function, run when the task enters the given state
//        - Action function, run again and again while the task remains in a state
//        - Test function(s) which determine if task will enter a new state
//=====================================================================================

//-------------------------------------------------------------------------------------
//  Constructor:  CState
//      This constructor builds a state object.  The state has a name and a set of
//      functions (Entry(), Action(),...) which it can run.  CState is used as a base
//      class so that the user can define his own states which exhibit the behavior
//      desired in a state-transition-logic based program.  The user must have over-
//      ridden at least one of the Entry, Action, or Transition Test functions; so
//      the constructor checks to see that at least one has been overridden and if
//      none has, it dumps out of the program with an error message.

CState::CState (const char *aName)
    {
    //  Make some space and save the name of this object
    Name = new char[strlen (aName) + 1];
    if (Name != NULL)
        strcpy (Name, aName);

    //  Set the FirstTimeRun variable to TRUE so that a test will be performed to
    //  verify that user has overloaded at least one entry, action, or test functions
    FirstTimeRun = TRUE;

    //  'EnteringThisState' is TRUE when we're going to enter this state from 
    //  another state.  It's true now, since we haven't even run the scheduler yet
    EnteringThisState = TRUE;

    //  This state hasn't been inserted into a task's list, so serial number is bunk
    SerialNumber = -1;

    //  Create the objects which will keep execution time data for each function
    EntryProfiler = new CProfiler ();
    ActionProfiler = new CProfiler ();
    TestProfiler = new CProfiler ();
    DoProfile = FALSE;                      //  Profiler is activated by ProfileOn()
    }


//-------------------------------------------------------------------------------------
//  Destructor:  CState
//      This destructor just frees up any memory allocated for the state object.

CState::~CState (void)
    {
    DELETE_ARRAY Name;
    delete EntryProfiler;
    delete ActionProfiler;
    delete TestProfiler;
    }


//-------------------------------------------------------------------------------------
//  Function:  Schedule
//      This is the supervisory function which runs the Entry(), Action(), and
//      TransitionTest() functions when they need to be run.

CState* CState::Schedule (void)
    {
    CState* TheNextState;                   //  Points to state to which we transition

    #if defined (TR_THREAD_MULTI) || defined (TR_TIME_FREE)
        real_time BeginTime;                //  Used for measuring execution times
    #endif

    //  If this state is being entered, run the Entry() function
    if (EnteringThisState == TRUE)
        {
        EnteringThisState = FALSE;

        #if defined (TR_THREAD_MULTI) || defined (TR_TIME_FREE)
            if (DoProfile == TRUE)  BeginTime = GetTimeNowUnprotected ();
        #endif

        EnableInterrupts ();                //  Turn interrupts on (in multithreading
        Entry ();                           //  modes only) so that this function can
        DisableInterrupts ();               //  be pre-empted by higher priority tasks

        #if defined (TR_THREAD_MULTI) || defined (TR_TIME_FREE)
            if (DoProfile == TRUE)
                EntryProfiler->SaveData (GetTimeNowUnprotected () - BeginTime);
        #endif
        }
        
    //  We're remaining within this state, so run the Action() function
    #if defined (TR_THREAD_MULTI) || defined (TR_TIME_FREE)
        if (DoProfile == TRUE)  BeginTime = GetTimeNowUnprotected ();
    #endif

    EnableInterrupts ();
    Action ();
    DisableInterrupts ();

    #if defined (TR_THREAD_MULTI) || defined (TR_TIME_FREE)
        if (DoProfile == TRUE)
            ActionProfiler->SaveData (GetTimeNowUnprotected () - BeginTime);
    #endif

    //  Run the transition test function and save the next state
    #if defined (TR_THREAD_MULTI) || defined (TR_TIME_FREE)
        if (DoProfile == TRUE)  BeginTime = GetTimeNowUnprotected ();
    #endif

    EnableInterrupts ();
    TheNextState = TransitionTest ();
    DisableInterrupts ();

    #if defined (TR_THREAD_MULTI) || defined (TR_TIME_FREE)
        if (DoProfile == TRUE)
            TestProfiler->SaveData (GetTimeNowUnprotected () - BeginTime);
    #endif

    //  If a transition has been called for, set the entering variable TRUE so the
    //  Entry() function runs next time this state's Schedule() function is called
    if (TheNextState != NULL)
        EnteringThisState = TRUE;

    //  Return a pointer to the state which will be run the next time around
    return (TheNextState);
    }


//-------------------------------------------------------------------------------------
//  Function:  Entry
//      The entry function is called when the parent task enters this state after a
//      transition.  The user may override this function; if not, it adds 1 to the
//      count of non-overloaded functions which have been run.

void CState::Entry (void)
    {
    static boolean FirstTimeRun = FALSE;
    if (FirstTimeRun == TRUE)
        {
        NonOverloadedCount++;
        FirstTimeRun = FALSE;
        }
    Idle ();
    }


//-------------------------------------------------------------------------------------
//  Function:  Action
//      The action function runs repeatedly while the parent task remainins in this
//      state.

void CState::Action (void)
    {
    static boolean FirstTimeRun = FALSE;
    if (FirstTimeRun == TRUE)
        {
        NonOverloadedCount++;
        FirstTimeRun = FALSE;
        }
    Idle ();
    }


//-------------------------------------------------------------------------------------
//  Function:  TransitionTest
//      The transition test function is called at end of every scan and returns the
//      value of a new state to be entered next scan (or it returns NULL for no
//      transition).  If not overloaded, this function always returns NULL.

CState *CState::TransitionTest (void)
    {
    static boolean FirstTimeRun = FALSE;
    if (FirstTimeRun == TRUE)
        {
        NonOverloadedCount++;
        FirstTimeRun = FALSE;
        }
    Idle ();
    return (NULL);
    }


//-------------------------------------------------------------------------------------
//  Function:  SetSampleTime
//      This function calls the parent task's SetSampleTime method.  It's placed here
//      so the user doesn't have to worry about parent task pointers to adjust the
//      sample time.  If there is no parent task, it doesn't set anything.

void CState::SetSampleTime (real_time aTime)
    {
    if (pParent != NULL)
        ((CTask *)pParent)->SetSampleTime (aTime);
    }


//-------------------------------------------------------------------------------------
//  Function:  GetSampleTime
//      A user might wish to get the sample time from within a state too; this func-
//      tion does that.  If no parent task has been set, it returns zero.

real_time CState::GetSampleTime (void)
    {
    if (pParent != NULL)
        return (((CTask *)pParent)->GetSampleTime ());
    else
        return ((real_time)0.0);
    }


//-------------------------------------------------------------------------------------
//  Function:  GetPriority
//      This function is used to get the priority of a state's parent task.  If the
//      parent task has been set, it returns the priority as given by the parent; if
//      not, it returns -1, which is an invalid priority for which the user can test.

int CState::GetPriority (void)
    {
    if (pParent != NULL)
        return (((CTask *)pParent)->GetPriority ());
    else
        return (-1);
    }


//-------------------------------------------------------------------------------------
//  Function:  Idle
//      If you're writing a sample-time or event task, you must call the Idle() func-
//      tion to stop the action function's execution until the next time step.  This
//      function calls the Idle() method of the parent task of this state for you.

void CState::Idle (void)
    {
    if (pParent != NULL)
        ((CTask *)pParent)->Idle ();
    }


//-------------------------------------------------------------------------------------
//  Function:  Deactivate
//      Tasks have the methods Deactivate() and Reactivate() to stop and restart exe-
//      cution of the task's functions - all of them.  Call this function to halt the
//      running of the parent task.

void CState::Deactivate (void)
    {
    if (pParent != NULL)
        ((CTask *)pParent)->Deactivate ();
    }


//-------------------------------------------------------------------------------------
//  Function:  Reactivate
//      If you have called the above function, Deactivate(), to stop the parent task
//      from running this state, you may call either that task's Reactivate() function
//      or this state's Reactivate() function; either one will wake the task up again.

void CState::Reactivate (void)
    {
    if (pParent != NULL)
        ((CTask *)pParent)->Reactivate ();
    }


//-------------------------------------------------------------------------------------
//  Function:  ProfileOn (version which sets histogram parameters)
//      This function activates execution-time profiling and sets the number of bins
//      used for keeping a histogram as well as the minimum and maximum bins.
//      The version of this function which takes no parameters is declared inline. 

void CState::ProfileOn (int aNum, real_time aMin, real_time aMax)
    {
    EntryProfiler->SetBins (aNum, aMin, aMax);
    ActionProfiler->SetBins (aNum, aMin, aMax);
    TestProfiler->SetBins (aNum, aMin, aMax);
    DoProfile = TRUE;
    }


//-------------------------------------------------------------------------------------
//  Function:  DumpProfile
//      This function writes information about how long the Entry(), Action(), and
//      TransitionTest() functions of this state took to run.  Average and maximum
//      times are reported as well as standard deviation and a table of frequencies
//      which can be used to plot a histogram.

void CState::DumpProfile (FILE* aFile, const char* aTaskName)
    {
    //  Create a string to hold headers for functions' timing printouts
    char Header[128];

	//  Call profiler objects which belong to the entry, action, and test functions
    //  and tell them to print their timing information
    sprintf (Header, "----- Task \"%s\" State \"%s\" Entry Function ",
             aTaskName, Name);
    for (int Index = strlen (Header); Index < PAGE_WIDTH; Index++)
        strcat (Header, "-");
    fprintf (aFile, "%s\n", Header);
    EntryProfiler->DumpProfile (aFile);

    sprintf (Header, "----- Task \"%s\" State \"%s\" Action Function ",
             aTaskName, Name);
    for (int Index = strlen (Header); Index < PAGE_WIDTH; Index++)
        strcat (Header, "-");
    fprintf (aFile, "%s\n", Header);
    ActionProfiler->DumpProfile (aFile);

    sprintf (Header, "----- Task \"%s\" State \"%s\" Test Function ",
             aTaskName, Name);
    for (int Index = strlen (Header); Index < PAGE_WIDTH; Index++)
        strcat (Header, "-");
    fprintf (aFile, "%s\n", Header);
    TestProfiler->DumpProfile (aFile);
    }


//-------------------------------------------------------------------------------------
//  Function:  DumpDurations
//      This function writes up to three files, each containing a histogram table for
//      the Entry(), Action(), or TransitionTest() function's run time.  If any of
//      the filenames given is a NULL string, or if any files cannot be opened, these
//      functions' profiles will not be written.  

void CState::DumpDurations (const char* aEntryName, const char* aActionName,
                            const char* aTestName)
    {
    FILE* EntryFile;                        //  Pointers to the files to which the
    FILE* ActionFile;                       //  frequency tables will be written
    FILE* TestFile;

    //  Attempt to open the files; if one can't be opened, no problem
    if (aEntryName != NULL)  EntryFile = fopen (aEntryName, "w");
    if (aActionName != NULL) ActionFile = fopen (aActionName, "w");
    if (aTestName != NULL)   TestFile = fopen (aTestName, "w");

    //  If any file was successfully opened, write to it and close it
    if (EntryFile != NULL)
        {
        EntryProfiler->DumpHistogram (EntryFile);
        fclose (EntryFile);
        }

    if (ActionFile != NULL)
        {
        ActionProfiler->DumpHistogram (ActionFile);
        fclose (ActionFile);
        }

    if (TestFile != NULL)
        {
        TestProfiler->DumpHistogram (TestFile);
        fclose (TestFile);
        }
    }


//-------------------------------------------------------------------------------------
//  Function:  DumpLatencies
//      This function isn't yet implemented.  I'll get to it sometime...

void CState::DumpLatencies (const char* aEntryName, const char* aActionName,
                            const char* aTestName)
    {
    }

