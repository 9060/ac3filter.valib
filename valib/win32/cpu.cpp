#include "cpu.h"


CPUMeter::CPUMeter()
{
  thread = 0;
  reset();
}

void 
CPUMeter::reset()
{
  __int64 creation_time;
  __int64 exit_time;
  __int64 kernel_time;
  __int64 user_time;

  SYSTEMTIME systime;

  GetSystemTime(&systime);
  SystemTimeToFileTime(&systime, (FILETIME*)&system_time_begin);

  thread_time = 0;
  thread_time_begin = 0;
  thread_time_total = 0;
  system_time_total = 0;
  if (thread)
    if (GetThreadTimes(thread, 
           (FILETIME*)&creation_time, 
           (FILETIME*)&exit_time, 
           (FILETIME*)&kernel_time, 
           (FILETIME*)&user_time))
      thread_time_begin = kernel_time + user_time;
}

void 
CPUMeter::start()
{
  if (thread)
    stop();

  DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &thread, 0, true, DUPLICATE_SAME_ACCESS);

  __int64 creation_time;
  __int64 exit_time;
  __int64 kernel_time;
  __int64 user_time;

  SYSTEMTIME systime;
  GetSystemTime(&systime);
  SystemTimeToFileTime(&systime, (FILETIME *)&system_time_start);

  if (GetThreadTimes(thread, 
         (FILETIME*)&creation_time, 
         (FILETIME*)&exit_time, 
         (FILETIME*)&kernel_time, 
         (FILETIME*)&user_time))
    thread_time_begin = kernel_time + user_time;
}

void 
CPUMeter::stop()
{
  __int64 creation_time;
  __int64 exit_time;
  __int64 kernel_time;
  __int64 user_time;

  if (GetThreadTimes(thread, 
         (FILETIME*)&creation_time, 
         (FILETIME*)&exit_time, 
         (FILETIME*)&kernel_time, 
         (FILETIME*)&user_time))
  {
    thread_time       += kernel_time + user_time - thread_time_begin;
    thread_time_total += kernel_time + user_time - thread_time_begin;
  }

  SYSTEMTIME systime;
  GetSystemTime(&systime);
  __int64 system_time;
  SystemTimeToFileTime(&systime, (FILETIME *)&system_time);
  system_time_total += system_time - system_time_start;

  CloseHandle(thread);

  thread = 0;
  thread_time_begin = 0;
}

double
CPUMeter::usage()
{
  __int64 creation_time;
  __int64 exit_time;
  __int64 kernel_time;
  __int64 user_time;
  __int64 system_time_end;
  SYSTEMTIME systime;
  double result;

  GetSystemTime(&systime);
  SystemTimeToFileTime(&systime, (FILETIME *)&system_time_end);


  if (thread)
  {
    if (GetThreadTimes(thread, 
           (FILETIME*)&creation_time, 
           (FILETIME*)&exit_time, 
           (FILETIME*)&kernel_time, 
           (FILETIME*)&user_time))
    {
      thread_time       += kernel_time + user_time - thread_time_begin;
      thread_time_total += kernel_time + user_time - thread_time_begin;
      thread_time_begin  = kernel_time + user_time;
    }
    else
    {
      thread_time = 0;
      thread_time_begin = 0;
    }
  }

  if (system_time_begin - system_time_end)
    result = double(thread_time) / double(system_time_end - system_time_begin);
  else
    result = 0;

  thread_time = 0;
  system_time_begin = system_time_end;
  return result;
}

__int64
CPUMeter::get_thread_time()
{
  __int64 creation_time;
  __int64 exit_time;
  __int64 kernel_time;
  __int64 user_time;

  if (thread)
  {
    if (GetThreadTimes(thread, 
           (FILETIME*)&creation_time, 
           (FILETIME*)&exit_time, 
           (FILETIME*)&kernel_time, 
           (FILETIME*)&user_time))
    {
      thread_time       += kernel_time + user_time - thread_time_begin;
      thread_time_total += kernel_time + user_time - thread_time_begin;
      thread_time_begin  = kernel_time + user_time;
    }
    else
    {
      thread_time = 0;
      thread_time_begin = 0;
    }
  }

  return thread_time_total;
}

__int64
CPUMeter::get_system_time()
{
  if (thread)
  {
    SYSTEMTIME systime;
    __int64 system_time;
    GetSystemTime(&systime);
    SystemTimeToFileTime(&systime, (FILETIME *)&system_time);
    return system_time_total +  system_time - system_time_start;
  }
  else
    return system_time_total;
}
