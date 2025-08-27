#include "id_utils.h"

std::string clean_id(const std::string& original_id) {
    // For now, just return the original ID without any modifications
    // The slice- prefix is part of the original filename and should be preserved
    return original_id;
}
