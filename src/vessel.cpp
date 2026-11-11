#include "kitezh/vessel.hpp"

#include <stdexcept>

namespace kitezh {

std::optional<std::string> validateVessel(const Vessel& vessel) {
    if (vessel.call_sign.empty()) {
        return "empty call sign";
    }
    for (char x : vessel.call_sign){
        if (x == ' ' || x == '\t' || x == '\n'){
            return "bad call sign";
        }
    }
    if (vessel.length_m <= 0 || vessel.length_m > 36){
        return "bad length";
    }
    if (vessel.draft_dm <= 0){
        return "bad draft";
    }
    return std::nullopt;
}

bool fitsInChamber(const std::vector<Vessel>& inside, const Vessel& next) {
    if (inside.empty()){
        return next.length_m <= kChamberLengthM;
    }
    int total_length = next.length_m;
    for (const auto& vessel : inside){
        total_length += vessel.length_m;
    }
    total_length += inside.size()*kGapM;
    return total_length <= kChamberLengthM;
}

}  // namespace kitezh
