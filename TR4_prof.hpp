//*************************************************************************************
//  TR4_prof.hpp
//      This is the header file for the TranLog4 execution-time profiler class used
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

#ifndef TR4_PROF_HPP                        //  Protect file from multiple inclusions
    #define TR4_PROF_HPP

//=====================================================================================
//  Class: CProfiler
//      This class implements a run-time profiling object which keeps useful statis-
//      tical data on the execution times of a user's functions.  It is given data
//      which is being measured during every run of the given function; it enters the
//      data into bins and presents a summary at the end of the run.
//=====================================================================================

class CProfiler
    {
    private:
        long NumberOfRuns;                  //  How many times the function was run
        real_time SumOfRunTimes;            //  Sum of all run times
        real_time SumOfSquares;             //  Sum of squares of run times
        real_time LongestRun;               //  Duration of the very longest run
        int NumberOfBins;                   //  Number of bins in duration histogram
        real_time Minimum;                  //  Minimum time on histogram - usually 0
        real_time Maximum;                  //  Time for maximum bin
        real_time LinearBinSize;            //  Size of each linearly spaced bin  
        long* HistogramBins;                //  Pointer to the array of duration bins

    public:
        //  Constructors for default version and user-sized version
        CProfiler (void);
        CProfiler (int, real_time, real_time);
        ~CProfiler (void);

        void SaveData (real_time);          //  Save one timing data point
        void ClearData (void);              //  Clear all the bins out

        //  Specify the number of bins and the minimum and maximum times
        void SetBins (int, real_time, real_time);

        void DumpProfile (FILE*);           //  Write profile for a function to file
        void DumpHistogram (FILE*);         //  Write a histogram table of times
        int GetNumberOfBins (void)          //  Ask how many bins there are in the
            { return NumberOfBins; }        //    histogram array
        long GetNumberOfRuns (void)         //  Function returns number of times
            { return NumberOfRuns; }        //    the function has been called
        real_time GetSumOfRunTimes (void)   //  Function to return total time the
            { return SumOfRunTimes; }       //    function has been running
        real_time GetSumOfSquares (void)    //  Function returns sum of squares of
            { return SumOfSquares; }        //    function execution times
        real_time GetLongestRun (void)      //  Function returns time of slowest
            { return LongestRun; }          //    execution of the function
    };

#endif                                      //  End of multiple inclusion protection

