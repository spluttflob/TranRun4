//*************************************************************************************
//  TR4_LITE.HPP
//      This file contains the headers for the TranRun4 Light project.
//
//  Version
//       8-12-96  JRR  Original file
//      10-14-96  JRR  Modified to use state-based rather than task-based scheduling
//      12-21-96  JRR  Changed to TranRun4 Light from version 3
//       1-14-96  JRR  Added macros for oper-int, logger; added initial state enum.   
//*************************************************************************************

//  Include the general TranRun4 header file here
#include <TranRun4.hpp>
#include <oper_int.hpp>

//  Pointers to the logger and process objects are global to the whole project
extern CProcess* MainProcess;               //  Points to primary process object

//  These macros are used to make invoking TheMaster's methods a little easier
#define  SetTickTime(x)                TheMaster->SetTickTime((real_time)(x))
#define  SetStopTime(x)                TheMaster->SetStopTime((real_time)(x))
#define  RunScheduler()                TheMaster->Go()
#define  StopScheduler()               TheMaster->Stop()
#define  WriteTraceToFile(x)           TheMaster->DumpTrace(x)
#define  TurnProfilingOn()             TheMaster->ProfileOn()
#define  WriteTimingProfiles(x)        TheMaster->DumpProfiles (x);

//  These macros allow the use of C style to create and use an operator interface
#define  CreateOperatorInterface(x)    new COperatorWindow(x)
#define  DisplayOperatorInterface(p)   (p)->Display()
#define  UpdateOperatorInterface(p)    (p)->Update()
#define  CreateInputItem(p,x,y)        (p)->AddInputItem((x),(y))
#define  CreateOutputItem(p,x,y)       (p)->AddOutputItem((x),(y))
#define  CreateKey(p,x,y,z,w)          (p)->AddKey((x),(y),(z),(w))

//  This macro and functions in data_lgr.hpp allow the use of C-style calling conven-
//  tions to create and use a data logger
#define  CreateDataLogger              new CDataLogger

//  Macro to allow C style calling of function to writes state function timing data
#define  WriteDurations(p,x,y,z)       (p)->DumpDurations((x),(y),(z))

//  More macros for C style calling of tasks' member functions
#define  SetSampleTime(p,t)            (p)->SetSampleTime(t)
#define  RunTaskAgain(p)               (p)->RunAgain()


//=====================================================================================
//  Interface Function Prototypes
//      The following functions are defined C style so using them is easier to learn
//      (Note that they're still written in C++, however.)
//=====================================================================================

//  These functions create task objects for state-based scheduling.  Each task created
//  with these functions must have one or more state objects in its state list.
CTask* CreateTimerIntTask (char*, real_time);
CTask* CreatePreemptibleTask (char*, int, real_time);
CTask* CreateSampleTimeTask (char*, int, real_time);
CTask* CreateEventTask (char*, int);
CTask* CreateContinuousTask (char*);

//  Functions to create task objects for task-based scheduling.  Each task created
//  with these functions just calls the supplied run function
CTask* CreateTimerIntTask (char*, real_time, long (*)(long));
CTask* CreatePreemptibleTask (char*, int, real_time, long (*)(long));
CTask* CreateSampleTimeTask (char*, int, real_time, long (*)(long));
CTask* CreateEventTask (char*, int, long (*)(long));
CTask* CreateContinuousTask (char*, long (*)(long));

//  Enumeration for specifying whether a given state is to be a task's initial state
typedef enum TagSI {IS_INITIAL_STATE, NOT_INITIAL_STATE} StateInit;
 

//=====================================================================================
//  Class:  C_Lite_Task
//      This class is a wrapper for tasks used in the TranRun4 Light scheduler which
//      have their own run functions but no state objects.
//=====================================================================================

class C_Lite_Task : public CTask
    {
    private:
        long (*RunFunc)(long);              //  Pointer to user's run function
        bool WillRunAgain;                  //  If true, Idle() won't be called

    public:
        //  Constructors get name, priority, sample time, and/or run function pointer
        C_Lite_Task (char*, TaskType, real_time, long (*)(long));
        C_Lite_Task (char*, TaskType, int, real_time, long (*)(long));
        C_Lite_Task (char*, TaskType, int, long (*)(long));
        C_Lite_Task (char*, TaskType, long (*)(long));

        //  Destructor doesn't get anything; of course it doesn't want it either
        ~C_Lite_Task (void);

        //  Run function must have been supplied by the user
        virtual void Run (void);

        //  Call this function to give another scan during this sample time
        void RunAgain (void);
    };


//=====================================================================================
//  Class:  C_Lite_State
//      This class provides a wrapper for states used in the TranRun3 Light scheduler.
//      States are given entry, action, and transition test functions by the user
//      and can run these functions when asked to.
//=====================================================================================

class C_Lite_State : public CState
    {
    private:
        void (*EntryFunc)(void);            //  Pointer to user's entry function
        void (*ActionFunc)(void);           //  Pointer to user's action function
        CState* (*TestFunc)(void);          //  Pointer to transition test function

    public:
        //  Constructor is given a task name, sample time, priority, and run functions
        C_Lite_State (const char* aName, void (*aEntryFunc)(void),
               void (*aActionFunc)(void), CState* (*aTestFunc)(void));
        ~C_Lite_State (void);

        //  Entry, action, and test functions run their user-written functions
        virtual void Entry (void);
        virtual void Action (void);
        virtual CState* TransitionTest (void);

        //  Create a state object with a name and parent task and run function pointers
        friend CState* CreateState (char*, CTask*, StateInit, void (*E)(void),
                                    void (*A)(void), CState* (*T)(void));
    };


//--------------------------------------------------------------------------------------
//  Function prototype:  PIDControl
//      This function runs a PID control loop.  Written 3-10-97 by DMA.  

double PIDControl
    (
    double kp, double ki, double kd,   // Controller gains
    double mmin, double mmax,          // Output limits
    double process,                    // Process value
    double set,                        // Setpoint
    double &integ,                     // Integrator - value is
                                       // maintained by calling function
    double &prev_val,                  // Previous  value - either error or
                                       // process value (see diff_type), also
                                       // maintained by calling function
    double delta_t,                    // Time since last call
    int diff_type,                     // 0 - use error,
                                       // 1 - use proc for derivative
    int first                          // Marks first call
    );

