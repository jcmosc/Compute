#include "WeakAttributeID.h"

#include "AttributeID.h"
#include "Data/Table.h"
#include "Data/Zone.h"

namespace AG {

bool WeakAttributeID::expired() const {
    uint64_t raw_page_seed = data::table::shared().raw_page_seed(_attribute.page_ptr());
    if (raw_page_seed & 0xff00000000) {
        auto zone_info = data::zone::info::from_raw_value(uint32_t(raw_page_seed));
        if (zone_info.zone_id() == _zone_id) {
            return false;
        }
    }
    return true;
}

const AttributeID &WeakAttributeID::evaluate() const {
    return _attribute.without_kind() != 0 && !expired() ? _attribute : AttributeIDNil;
};

} // namespace AG
