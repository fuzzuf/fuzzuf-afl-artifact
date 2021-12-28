#include <ostream>
#include "Feedback/PUTExitReasonType.hpp"

// Pass just raw integer value
std::ostream& boost_test_print_type(std::ostream& ostr, PUTExitReasonType const &val) {
    ostr << static_cast<int>(val);
    return ostr;
}
