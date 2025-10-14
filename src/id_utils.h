#pragma once
#include <string>

// Clean ID by removing common prefixes and suffixes
std::string clean_id(const std::string& original_id);

// Generate unique ID for different object types
std::string generate_geometry_id(const std::string& base_id);
std::string generate_face_id(const std::string& base_id);
