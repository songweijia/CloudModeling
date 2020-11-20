#!/bin/bash
HEADER_FILE=linux_perf_tracepoint_ids.hpp

rm -f $HEADER_FILE
echo "#ifndef LINUX_PERF_TRACEPOINT_IDS_HPP" >> $HEADER_FILE
echo "#define LINUX_PERF_TRACEPOINT_IDS_HPP" >> $HEADER_FILE
echo "" >> $HEADER_FILE

if [ $# -eq 0 ]; then
    SCOPE="predefined"
else
    SCOPE=$1
fi

tracepoints=(
    "sched:sched_kthread_stop"
    "sched:sched_kthread_stop_ret"
    "sched:sched_migrate_task"
    "sched:sched_move_numa"
    "sched:sched_pi_setprio"
    "sched:sched_process_exec"
    "sched:sched_process_exit"
    "sched:sched_process_fork"
    "sched:sched_process_free"
    "sched:sched_process_hang"
    "sched:sched_process_wait"
    "sched:sched_stat_blocked"
    "sched:sched_stat_iowait"
    "sched:sched_stat_runtime"
    "sched:sched_stat_sleep"
    "sched:sched_stat_wait"
    "sched:sched_stick_numa"
    "sched:sched_swap_numa"
    "sched:sched_switch"
    "sched:sched_wait_task"
    "sched:sched_wake_idle_without_ipi"
    "sched:sched_wakeup"
    "sched:sched_wakeup_new"
    "sched:sched_waking")

if [ ${SCOPE} == "all" ] ; then
    for event in `sudo perf list tracepoint | grep Tracepoint\ event | sed 's/:/\//g' | awk '{print $1}'`
    do
        echo -n -e "${event}\t"| sed -E 's/\/|-/_/g' | awk '{printf "#define %s ", toupper($1)}' >> $HEADER_FILE
        sudo cat `sudo find /sys/kernel/ | grep "${event}/id"` >> $HEADER_FILE
    done
else
    for event in ${tracepoints[@]}
    do
        event=`echo $event |  sed 's/:/\//g' | awk '{print $1}'`
        echo -n -e "${event}\t"| sed -E 's/\/|-/_/g' | awk '{printf "#define %s ", toupper($1)}' >> $HEADER_FILE
        sudo cat `sudo find /sys/kernel/ | grep "${event}/id"` | head -1 >> $HEADER_FILE
    done
fi
echo "" >> $HEADER_FILE
echo "#endif" >> $HEADER_FILE

# clang-format -i $HEADER_FILE
