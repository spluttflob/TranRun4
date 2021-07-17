//*************************************************************************************
//  TR4_SHAR.CPP
//      This file contains the member functions classes used to maintain shared
//      inter-task variables.  These variables are communicated automatically between
//      tasks so that the user needn't go to the trouble of satisfying mutual
//      exclusion rules or handling network communications and such by hand.
//
//  Version
//      8-2-95   JR  Original File
//*************************************************************************************

#include <tranrun4.hpp>


// typedef enum SV_CommType {SV_TRANSMIT, SV_RECEIVE, SV_TRANSMIT_RECEIVE};

// typedef int SharedVariableID;


//=====================================================================================
//  Class:  CSharedVariable
//      This basic shared-variable class encapsulates stuff needed to run any type of
//      shared variable - interstate on one computer, intertask on two, etc.
//=====================================================================================

CSharedVariable::CSharedVariable (int);
CSharedVariable::~CSharedVariable (void);


//=====================================================================================
//  Class:  CSharedVariableArray
//      This class maintains an array of shared variables which is used by the sched-
//      uler to find and update, or find and access, any given variable when called
//      for by a task or state function.
//=====================================================================================

//-------------------------------------------------------------------------------------
//  Constructor:  CSharedVariableArray
//      This constructor creates a new, empty array which can hold shared variable
//      objects.

CSharedVariableArray::CSharedVariableArray (int)
    {
    }


//-------------------------------------------------------------------------------------
//  Destructor:  ~CSharedVariableArray
//      This destructor frees up the memory used by an array of shared variable
//      objects, and it calls delete to zap those shared variable objects too.

CSharedVariableArray::~CSharedVariableArray (void)
    {
    }


//-------------------------------------------------------------------------------------
//
SharedVariableID CSharedVariableArray::Insert (CSharedVariable&)
    {
    }


//-------------------------------------------------------------------------------------
//  Functions:  Add
//      The Add functions are called by a user who wants a variable to be shared.
//      Each function creates a shared variable object and inserts it in the array of
//      such objects.

SharedVariableID CSharedVariableArray::Add (short*)
    {
    }

SharedVariableID CSharedVariableArray::Add (int*)
    {
    }

SharedVariableID CSharedVariableArray::Add (unsigned*)
    {
    }

SharedVariableID CSharedVariableArray::Add (long*)
    {
    }

SharedVariableID CSharedVariableArray::Add (unsigned long*)
    {
    }

SharedVariableID CSharedVariableArray::Add (float*)
    {
    }

SharedVariableID CSharedVariableArray::Add (double*)
    {
    }

SharedVariableID CSharedVariableArray::Add (long double*)
    {
    }


//-------------------------------------------------------------------------------------
//  Operator:  []
//      Find the variable with the given ID number

CSharedVariable& CSharedVariableArray::operator[] (SharedVariableID)
    {
    }




