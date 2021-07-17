//*************************************************************************************
//  SCHEDULER CONFIGURATION HEADER FILE
//      This is a configuration file for UCB real-time scheduler projects.  Many
//      versions of this file can be created, one for each combination of scheduler
//      type, timekeeping mode, threading mode, etc. etc.  The files are named
//      according to the convention described in unmodified versions of TR3_CONF.HPP.
//
//  Revisions
//      Original file copyright 1994,95 by DM Auslander and JR Ridgely, UC Berkeley
//      Use for non-commercial purposes is permitted as long as this copyright notice
//      is included.
//       9-18-95  JR   Added state or task based scheduler definitions
//      12-21-95  JR   Ported to TranRun4 by removing _CTL_EXEC_ and _TRANRUN3_
//*************************************************************************************

//  Define exactly one time keeping mode here, TR_TIME_[something]
//#define  TR_TIME_SIM
  #define  TR_TIME_FREE
//#define  TR_TIME_INT
//#define  TR_TIME_FTIME
//#define  TR_TIME_EXTSIM

//  Either #define or #undef the symbol TR_THREAD_MULTI here - note, multithreading
//  is only used with TR_TIME_INT or TR_TIME_FREE timing modes
  #define  TR_THREAD_SINGLE
//#define  TR_THREAD_MULTI

//  Pick a scheduling mode, SEQuential or MINimum latency, here
  #define  TR_EXEC_SEQ
//#define  TR_EXEC_MIN


