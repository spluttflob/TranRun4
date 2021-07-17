//*************************************************************************************
//  TR4_INTR.CPP
//      This is the implementation file for interrupt handling objects.
//
//  Copyright (c) 1994, D.M.Auslander and J.R.Ridgely
//      May be used and distributed for any non-commercial purposes as long as this
//      copyright notice is included.
//      Based on a program written 11-94 by DMA
//         2-02-95  JR   Interrupt object moved to this separate file
//         5-20-95  JR   Promoted to the TL3 project
//*************************************************************************************

#include <dos.h>                                //  Functions for interrupt processing
#include <TranRun4.hpp>


//=====================================================================================
//  Class: CInterruptObj
//      This object handles the installation and removal of interrupt service routines 
//      and stuff.  It also contains the SetAlarm() function which adjusts the rate at 
//      which timer interrupts occur.  
//=====================================================================================

//-------------------------------------------------------------------------------------
//  Constructor: CInterruptObj
//      This constructor creates a new CInterruptObj object.  It initializes the data 
//      but does not install the ISR yet - that's done with a call to InstallISR().  

CInterruptObj::CInterruptObj (unsigned aNum, ISR_POINTER (aISR))
    {
    VectorNumber = aNum;                //  Save number of vector on which this goes 
    The_ISR = aISR;                     //  Save address of interrupt service routine 
    ISR_Installed = FALSE;              //  Flag indicates no ISR has been installed
    }


//-------------------------------------------------------------------------------------
//  Destructor: CInterruptObj
//      This destructor checks to make sure the user removed the ISR before destroying 
//      this object.  If he forgot, the ISR is removed to prevent a crash.

CInterruptObj::~CInterruptObj (void)
    {
    if (ISR_Installed == TRUE)
        RemoveISR ();
    }


//-------------------------------------------------------------------------------------
//  Function: InstallISR
//      This function installs an interrupt service routine on the given vector and 
//      sets a flag so the ISR will be removed at program end or when asked to exit.
//  Changes
//      9-05-95  JR   Removed EnableInterrupts() to prevent reentrance of scheduler 

void CInterruptObj::InstallISR (void)
    {
    #if defined (TR_THREAD_MULTI) || defined (TR_TIME_INT)
        if (ISR_Installed == FALSE)
            {
            //  Swap interrupt vectors around, thus activating the ISR 
            DisableInterrupts ();
            OldVect = _dos_getvect (VectorNumber);
            _dos_setvect (VectorNumber, The_ISR);
            ISR_Installed = TRUE;
            }
    #endif
    }


//-------------------------------------------------------------------------------------
//  Function: RemoveISR 
//      This function removes the timer interrupt service routine installed by the 
//      function above and resets the interrupt-installed flag to FALSE.  If a timer 
//      ISR is being used, it resets the clock rate to DOS default.  
//  Changes
//      9-05-95  JR   Removed EnableInterrupts() call  

void CInterruptObj::RemoveISR (void)
    {
    #if defined (TR_THREAD_MULTI) || defined (TR_TIME_INT)
        if (ISR_Installed == TRUE)
            {
            DisableInterrupts ();                   //  Switch vectors back to the way
            _dos_setvect (VectorNumber, OldVect);   //  they were before this program
            ISR_Installed = FALSE;                  //  Flag indicates ISR removed
            }
    #endif
    }


//-------------------------------------------------------------------------------------
//  Function: SetAlarm 
//      Function to set the rate at which the time of day clock in the PC causes timer 
//      interrupts.  This clock runs at about 1.19MHz and can count down from any 16-
//      bit unsigned int to zero.  When it reaches zero, it causes a timer interrupt.  
//      Call SetAlarm ('time') where 'time' is an interval in seconds.  If 'time' is
//      outside the allowable range of intervals, the clock will be reset to the DOS 
//      default value of ~54.9 ms, so call SetAlarm with a time less than zero at the 
//      end of the program.  SetAlarm() returns TRUE if a non-DOS default time is set
//      and FALSE if the default is set (so that if the user wants to set a non-default
//      time, FALSE signifies an error).
//  Note:
//      For a maximum count of 0xFFFF, the time interval is about 0.0549 sec.  
//  Version:
//      Original - from the lab of D.M.Auslander, origin shrouded in history 
//      12-13-94  JJ  Removed compiler specific macro and inout.h; changed all I/O bus 
//                    functions to outp().  Changed scope of 'count' to local.  
//      12-18-94  JR  Changed to C++; integrated into CTL_EXEC scheduler
//       2-02-94  JR  Moved into CTL_INTR.CPP but not changed
//       9-20-95  JR  #defined out the function body if in mode not using interrupts

boolean CInterruptObj::SetAlarm (double period)
    {
    #if defined (USES_INTERRUPTS)
    const real_time CK_FREQ = 1193180.0;        //  Frequency of PC clock in Hz
    const real_time MAX_PER = 0.0549;           //  Maximum possible timer period, sec.
    const unsigned TIMER = 0x40;                //  I/O address of timer number 0
    const unsigned TIMER_CTL = 0x43;            //  8253 Timer control port
    unsigned count = 0xFFFF;                    //  Initialized to DOS default

    if ((period > 0.0) && (period < MAX_PER))           //  If argument's a valid time
        {
        count = (unsigned)((CK_FREQ * period) + 0.5);   //  calculate rate divisor
        outp (TIMER_CTL, 0x36);                         //  Mode 3, 16-bit binary count
        outp (TIMER, (int)(count & 0xFF));              //  Send least significant byte
        outp (TIMER, (int)((count >> 8) & 0xFF));       //  Then most significant one
        return (TRUE);                                  //  Return value: yes, non-DOS
        }
    else
        {
        outp (TIMER_CTL, 0x36);                         //  Reset period to DOS default
        outp (TIMER, (int)0xFF);
        outp (TIMER, (int)0xFF);
        return (FALSE);                                 //  Return "No, it's DOS time"
        }
    #else
    return (period);                                    //  Do nothing in modes which
    #endif                                              //  don't use interrupts
    }

