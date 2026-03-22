#ifndef RECASTCONTEXT_H
#define RECASTCONTEXT_H

#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <Recast.h>

#include <map>

class RecastContext : public rcContext {
public:
    RecastContext();

protected:
    virtual void doResetLog();
    virtual void doLog(const rcLogCategory category, const char *msg, const int len);
    virtual void doResetTimers();
    virtual void doStartTimer(const rcTimerLabel label);
    virtual void doStopTimer(const rcTimerLabel label);
    virtual int doGetAccumulatedTime(const rcTimerLabel label) const;

private:
    std::map<rcTimerLabel, int64_t> _timers;
    std::map<rcTimerLabel, int64_t> _accumulatedTime;
};

#endif // RECASTCONTEXT_H
