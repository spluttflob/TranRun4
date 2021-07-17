//*************************************************************************************
//  TR4_SHAR.HPP
//      This file contains definitions of classes used to maintain shared inter-task
//      variables.  These variables are communicated automatically between tasks so
//      that the user needn't go through the hassle of satisfying mutual exclusion
//      rules and such by hand.
//
//  Version
//      8-2-95   JR  Original File
//*************************************************************************************

#define  TR4_SHAR_HPP                       //  Variable to prevent multiple inclusions

//  Macro for Share() function:  The user gives as parameters a reference to a variable
//  and a communication type; the macro expands the command line to include data size
#define  Share(x,y)     Share((x), this, sizeof(x), (y))


//-------------------------------------------------------------------------------------
//  Enumerations:  SV_CommType
//      Each shared variable is treated in a different way by different tasks:  some
//      tasks may change the variable and transmit a new value to others (SV_TRANSMIT)
//      while others just look at the variable (SV_RECEIVE) and others do both.

typedef enum SV_CommType {SV_TRANSMIT, SV_RECEIVE, SV_TRANSMIT_RECEIVE};

typedef int SharedVariableID;

//=====================================================================================
//  Class:  CSharedVariable
//      This basic shared-variable class encapsulates stuff needed to run any type of
//      shared variable - interstate on one computer, intertask on two, etc.
//=====================================================================================

class CSharedVariable
    {
    private:
        void* pData;                        //  Pointer to the data item
        int SerialNumber;                   //  Position in the array of shared vars.

    public:
        CSharedVariable (int);              //  Constructor gets serial number
        ~CSharedVariable (void);



        int GetSerialNumber (void)          //  Call this function if you want to know
            { return (SerialNumber); }      //  where in the array this variable goes
    };


//=====================================================================================
//  Class:  CSharedVariableArray
//      This class maintains an array of shared variables which is used by the sched-
//      uler to find and update, or find and access, any given variable when called
//      for by a task or state function.
//=====================================================================================

class CSharedVariableArray : public CBasicArray
    {
    private:
        int CurrentSerialNumber;            //  Number of item now being inserted

    public:
        CSharedVariableArray (int);         //  Create empty array
        ~CSharedVariableArray (void);       //  Trash the whole #$%^! thing

        //  Insert a variable into the array, get back an ID which is used to find it
        SharedVariableID Insert (CSharedVariable&);

        //  Create a shared variable object, then call Insert() to insert it in list
        SharedVariableID Add (short*);
        SharedVariableID Add (int*);
        SharedVariableID Add (unsigned*);
        SharedVariableID Add (long*);
        SharedVariableID Add (unsigned long*);
        SharedVariableID Add (float*);
        SharedVariableID Add (double*);
        SharedVariableID Add (long double*);

        //  Find the variable with the given ID number
        CSharedVariable& operator[] (SharedVariableID);
    };



