#include "kitezh/lock.hpp"

#include <stdexcept>

namespace kitezh {

Lock::Lock() {
}

Level Lock::level() const {
    return level_;
}

GateState Lock::gate(Gate gate) const {
    return gate == Gate::Lower ? lower_gate : upper_gate;
}

int Lock::pumpCycles() const {
    return pump_cycles;
}

std::vector<Vessel> Lock::inside() const {
    return inside_;
}

void Lock::open(Gate gate) {
    if (gate == Gate::Lower) {
        if (lower_gate == GateState::Open) {
            return;
        }
        if (level_ != Level::Low || upper_gate != GateState::Closed) {
            throw LockError("cannot open lower gate");
        }
        lower_gate = GateState::Open;
    } else {
        if (upper_gate == GateState::Open) {
            return;
        }
        if (level_ != Level::High || lower_gate != GateState::Closed) {
            throw LockError("cannot open upper gate");
        }
        upper_gate = GateState::Open;
    }
}

void Lock::close(Gate gate) {
    if (gate == Gate::Lower) {
        lower_gate = GateState::Closed;
    } else {
        upper_gate = GateState::Closed;
    }
}

void Lock::pump() {
    if (lower_gate != GateState::Closed || upper_gate != GateState::Closed) {
        throw LockError("gates must be closed before pumping");
    }

    const Level next_level = (level_ == Level::Low) ? Level::High : Level::Low;
    const int next_depth = (next_level == Level::Low) ? kLowDepthDm : kHighDepthDm;

    for (const auto& v : inside_) {
        if (v.draft_dm > next_depth) {
            throw LockError("draft exceeds target water depth");
        }
    }

    level_ = next_level;
    ++pump_cycles;
}

void Lock::enter(const Vessel& vessel) {
    if (validateVessel(vessel).has_value()) {
        throw LockError("invalid vessel");
    }

    for (const auto& v : inside_) {
        if (v.call_sign == vessel.call_sign) {
            throw LockError("duplicate call sign in chamber");
        }
    }

    const int current_depth = (level_ == Level::Low) ? kLowDepthDm : kHighDepthDm;
    if (vessel.draft_dm > current_depth) {
        throw LockError("vessel draft too deep");
    }

    const GateState entry_gate = (vessel.direction == Direction::Upstream) ? lower_gate : upper_gate;
    if (entry_gate != GateState::Open) {
        throw LockError("entry gate is closed");
    }

    if (vessel.kind == VesselKind::Oversized && !inside_.empty()) {
        throw LockError("oversized vessel requires empty chamber");
    }
    for (const auto& v : inside_) {
        if (v.kind == VesselKind::Oversized) {
            throw LockError("cannot enter chamber with oversized vessel");
        }
    }

    if (!fitsInChamber(inside_, vessel)) {
        throw LockError("vessel does not fit in chamber");
    }

    inside_.push_back(vessel);
}

void Lock::exit(const std::string& call_sign) {
    if (inside_.empty() || inside_.front().call_sign != call_sign) {
        throw LockError("vessel is not at the front of the queue");
    }

    const GateState exit_gate = (inside_.front().direction == Direction::Upstream) ? upper_gate : lower_gate;
    if (exit_gate != GateState::Open) {
        throw LockError("exit gate is closed");
    }

    inside_.erase(inside_.begin());
}

}  // namespace kitezh
