#include "id_utils.h"

std::string clean_id(const std::string& original_id) {
    // For now, just return the original ID without any modifications
    // The slice- prefix is part of the original filename and should be preserved
    return original_id;
}

std::string generate_geometry_id(const std::string& base_id) {
    // For SolidGeometry objects, use the base ID as-is
    return base_id;
}

std::string generate_face_id(const std::string& base_id) {
    // For Face objects, append "_face" suffix to ensure uniqueness
    return base_id + "_face";
}
