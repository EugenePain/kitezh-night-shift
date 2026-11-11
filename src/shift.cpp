#include "kitezh/shift.hpp"

#include "kitezh/lock.hpp"

#include <stdexcept>

namespace kitezh {

ShiftReport runNightShift(const std::vector<Vessel>& queue) {
    ShiftReport report;
    std::vector<Vessel> waiting;
    std::vector<std::string> seen;

    for (const auto& vessel : queue) {
        bool is_duplicate = false;
        for (const auto& existing : seen) {
            if (existing == vessel.call_sign) {
                is_duplicate = true;
                break;
            }
        }

        if (is_duplicate) {
            report.refused.push_back({vessel.call_sign, "duplicate"});
            continue;
        }
        seen.push_back(vessel.call_sign);

        if (validateVessel(vessel).has_value()) {
            report.refused.push_back({vessel.call_sign, "invalid"});
            continue;
        }
        if (vessel.draft_dm > kLowDepthDm) {
            report.refused.push_back({vessel.call_sign, "draft"});
            continue;
        }

        waiting.push_back(vessel);
    }

    Lock lock;

    while (!waiting.empty()) {
        std::vector<Vessel> batch;
        batch.push_back(waiting.front());
        waiting.erase(waiting.begin());

        const Direction dir = batch.front().direction;

        if (batch.front().kind != VesselKind::Oversized) {
            bool skipped = false;
            std::size_t i = 0;
            while (i < waiting.size()) {
                const Vessel& cand = waiting[i];
                const bool can_take = (cand.direction == dir) &&
                                      (cand.kind != VesselKind::Oversized) &&
                                      fitsInChamber(batch, cand) &&
                                      (!skipped || cand.kind == VesselKind::Ambulance);
                if (can_take) {
                    batch.push_back(cand);
                    waiting.erase(waiting.begin() + static_cast<std::ptrdiff_t>(i));
                } else {
                    skipped = true;
                    ++i;
                }
            }
        }

        const Level entry_level = (dir == Direction::Upstream) ? Level::Low : Level::High;
        const Gate entry_gate = (dir == Direction::Upstream) ? Gate::Lower : Gate::Upper;
        const Gate exit_gate = (dir == Direction::Upstream) ? Gate::Upper : Gate::Lower;

        if (lock.level() != entry_level) {
            lock.pump();
        }

        lock.open(entry_gate);
        for (const auto& v : batch) {
            lock.enter(v);
        }
        lock.close(entry_gate);

        lock.pump();

        lock.open(exit_gate);
        for (const auto& v : batch) {
            lock.exit(v.call_sign);
            report.passed.push_back(v.call_sign);
        }
        lock.close(exit_gate);
    }

    report.pump_cycles = lock.pumpCycles();
    return report;
}
}  // namespace kitezh
