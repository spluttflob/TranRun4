//*************************************************************************************
//  TR4_prof.cpp
//      This is the implementation for the TranLog4 execution-time profiler class used
//      in TranLog4 programs.  This profiler is a simple tool which measures the
//      execution times of the user's Run(), Entry(), Action(), and TransitionTest()
//      functions and keeps some statistical data on them.     
//
//  Copyright (c) 1994-1997, D.M.Auslander and J.R.Ridgely
//      May be used and distributed for any non-commercial purposes as long as this
//      copyright notice is included.
//
//  Version
//      12-22-96  JR   Original version.  Idea taken from Howell and Uchanski's ME230
//                     class project, in which they wrote a nice fancy profiler 
//*************************************************************************************

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <TranRun4.hpp>


//=====================================================================================
//  Class: CProfiler
//      This class implements a run-time profiling object which keeps useful statis-
//      tical data on the execution times of a user's functions.  It is given data
//      which is being measured during every run of the given function; it enters the
//      data into bins and presents a summary at the end of the run.
//=====================================================================================

//-------------------------------------------------------------------------------------
//  Constructors:  CProfiler
//      This constructor creates an execution-time profiler with default parameters
//      of 100 linearly spaced histogram bins from 0 to 0.05 seconds.  This default is
//      chosen because it's convenient for use with Windows NT 4.0 on a Pentium-100.

CProfiler::CProfiler (void)
    {
    HistogramBins = NULL;                   //  Begin with a NULL bin pointer
    SetBins (100, 0.0, 0.05);               //  Set number of bins and min, max size
    ClearData ();                           //  Clear everything out
    }

CProfiler::CProfiler (int aNum, real_time aMin, real_time aMax)
    {
    HistogramBins = NULL;                   //  Begin with a NULL bin pointer
    SetBins (aNum, aMin, aMax);             //  Set number of bins and min, max size
    ClearData ();                           //  Clear everything out
    }


//-------------------------------------------------------------------------------------
//  Destructor:  ~CProfiler
//      This destructor frees memory used by the profiler object. 

CProfiler::~CProfiler (void)
    {
    if (HistogramBins != NULL)  DELETE_ARRAY HistogramBins;
    }


//-------------------------------------------------------------------------------------
//  Function:  SetBins
//      This function sets the number of bins in the histogram and allocates memory
//      for an array to hold the data in the bins.  It also specifies the minimum and
//      maximum for the histogram; bins will be linearly spaced between them.

void CProfiler::SetBins (int aNumber, real_time aMin, real_time aMax)
    {
    //  Check that the parameters are all legal; if not, complain and exit
    if ((aNumber < 1) || (aMin < 0.0) || (aMax < 0.0) || (aMin >= aMax))
        TR_Exit ("Invalid histogram parameters: %d bins, %lf min, %lf max",
                 aNumber, aMin, aMax);

    //  Save the parameters
    NumberOfBins = aNumber;
    Minimum = aMin;
    Maximum = aMax;
    LinearBinSize = (Maximum - Minimum) / (real_time)NumberOfBins;

    //  De-allocate memory for the histogram array if necessary and re-allocate it 
    if (HistogramBins != NULL)
        delete HistogramBins;
    HistogramBins = new long[NumberOfBins];

    //  Make sure the bins are empty
    ClearData ();
    }


//-------------------------------------------------------------------------------------
//  Function:  SaveData
//      This function saves a data point to all the registers which are affected by
//      the taking of this point.

void CProfiler::SaveData (real_time aTime)
    {
    int BinNumber;                              //  Into which bin goes this datum

    NumberOfRuns++;                             //  Save all the basic statistical
    SumOfRunTimes += aTime;                     //  data
    SumOfSquares += aTime * aTime;
    if (aTime > LongestRun) LongestRun = aTime;

    //  Save a point in histogram.  If the point's off one end it goes in the end bin
    BinNumber = (int)((aTime - Minimum) / LinearBinSize);
    if (BinNumber < 0)              BinNumber = 0;
    if (BinNumber >= NumberOfBins)  BinNumber = NumberOfBins - 1;
    HistogramBins[BinNumber]++;
    }


//-------------------------------------------------------------------------------------
//  Function:  ClearData
//      This function is called to reset all the histogram data.  Note that it doesn't
//      de-allocate the histogram array, it just sets its data to all zeros.

void CProfiler::ClearData (void)
    {
    NumberOfRuns = 0;
    SumOfRunTimes = 0.0;
    SumOfSquares = 0.0;
    LongestRun = 0.0;

    //  Set each element in the timing histogram to zero
    if (HistogramBins != NULL)
        {
        for (int Bin = 0; Bin < NumberOfBins; Bin++)
            HistogramBins[Bin] = 0L;
        }
    }


//-------------------------------------------------------------------------------------
//  Function:  DumpProfile
//      This function writes the profile data for given function to a file.  It is
//      meant to be called by the task or state object which owns this profiler.   

void CProfiler::DumpProfile (FILE *aFile)
    {
    //  If the function didn't run, mention that no data is available
    if (NumberOfRuns < 1)
        {
        fprintf (aFile, "Did not run\n\n");
        }
    else    //  else the function did run so compute and print out information
        {
        //  First compute and print the simple stuff - max, average, standard dev.
        fprintf (aFile, "Number of runs:     %ld\n", NumberOfRuns);

        real_time Average = SumOfRunTimes / (real_time)NumberOfRuns;
        fprintf (aFile, "Average duration:   %lf sec\n", Average);

        if (NumberOfRuns > 1)
            {
            real_time StdDev = sqrt ((1.0 / (real_time)(NumberOfRuns - 1))
                               * (SumOfSquares - (NumberOfRuns * Average * Average)));
            fprintf (aFile, "Standard deviation: %lf sec\n", StdDev);
            }
        fprintf (aFile, "Maximum duration:   %lf sec\n", LongestRun);
        fprintf (aFile, "\n");
        }
    }


//-------------------------------------------------------------------------------------
//  Function:  DumpHistogram
//      This function writes the table of histogram data to the given file.

void CProfiler::DumpHistogram (FILE* aFile)
    {
    //  The function ran; write the histogram table of times vs. numbers of hits
    for (int Bin = 0; Bin < NumberOfBins; Bin++)
        {
        //  Bin N is centered around time T = minimum + (bin width)(N + 0.5)
        fprintf (aFile, "%12.6lf %12ld\n",
                 Minimum + (LinearBinSize * ((double)Bin + 0.5)), HistogramBins[Bin]);
        }
    }


