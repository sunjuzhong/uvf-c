#include "vtp_to_uvf.h"

extern "C" {
// Parse VTP file, return 1 on success, 0 on failure
int parse_vtp(const char* vtp_path) {
    auto poly = parse_vtp_file(vtp_path);
    return poly ? 1 : 0;
}
// Generate UVF file from VTP, return 1 on success, 0 on failure
int generate_uvf(const char* vtp_path, const char* uvf_path) {
    auto poly = parse_vtp_file(vtp_path);
    if (!poly) return 0;
    return generate_uvf(poly, uvf_path) ? 1 : 0;
}
}
