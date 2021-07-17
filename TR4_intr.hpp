//*************************************************************************************
//  TR3_INTR.HPP
//      This is the header file for interrupt handling objects.
//
//  Copyright (c) 1994-1997, D.M.Auslander and J.R.Ridgely
//      May be used and distributed for any non-commercial purposes as long as this
//      copyright notice is included.
//
//  Version:  Based on a program written 11-94 by DMA
//       2-02-95  JR   Interrupt object moved to this separate file
//       3-16-95  JR   Ported to Borland C++ v1.0
//       5-20-95  JR   Ported to BC+ v4.5 and TranRun3 project
//*************************************************************************************

#ifndef  TR3_INTR_HPP
    #define  TR3_INTR_HPP                   //  Variable to prevent multiple inclusions

//=====================================================================================
//  Class: CInterruptObj
//      This object handles the installation and removal of interrupt service routines
//      and stuff.  It also contains the SetAlarm() function which adjusts the rate at
//      which timer interrupts occur.
//=====================================================================================

class CInterruptObj
    {
    private:
        unsigned VectorNumber;                  //  Which interrupt vector is this?
        boolean ISR_Installed;                  //  True if interrupt handlers active
        ISR_POINTER (OldVect);                  //  Save pointer to old ISR here
        ISR_POINTER (The_ISR);                  //  This is a pointer to the new ISR

    public:
        //  Constructor configures interrupts for a given vector
        CInterruptObj (unsigned, ISR_POINTER (aFunc));
        ~CInterruptObj (void);

        void InstallISR (void);             //  Install ISR so it can begin running
        void RemoveISR (void);              //  Stop interrupts by removing them
        boolean SetAlarm (double);          //  Change rate of PC timer interrupts
    };

#endif                                          //  Multiple-inclusion protection

