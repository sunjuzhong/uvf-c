#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// ====== Basic Functions ======

/**
 * Parse a VTP file and check if it's valid
 * @param vtp_path Path to the VTP file
 * @return 1 if successful, 0 if failed
 */
int parse_vtp(const char* vtp_path);

/**
 * Generate UVF from a single VTP file (basic mode)
 * @param vtp_path Path to input VTP file
 * @param uvf_dir Path to output UVF directory
 * @return 1 if successful, 0 if failed
 */
int generate_uvf(const char* vtp_path, const char* uvf_dir);

// ====== Enhanced Functions ======

/**
 * Generate UVF using structured parsing (field-based classification)
 * @param vtp_path Path to input VTP file
 * @param uvf_dir Path to output UVF directory
 * @return 1 if successful, 0 if failed
 */
int generate_uvf_structured(const char* vtp_path, const char* uvf_dir);

/**
 * Generate UVF from directory with multiple VTK files (recommended)
 * Automatically classifies files into groups (slices, surfaces, etc.)
 * @param input_dir Path to directory containing VTK files
 * @param uvf_dir Path to output UVF directory
 * @return 1 if successful, 0 if failed
 */
int generate_uvf_directory(const char* input_dir, const char* uvf_dir);

// ====== Status and Information Functions ======

/**
 * Get the error message from the last operation
 * @return Error string (valid until next API call)
 */
const char* uvf_get_last_error();

/**
 * Get the point count from the last operation
 * @return Number of points processed
 */
int uvf_get_last_point_count();

/**
 * Get the triangle count from the last operation
 * @return Number of triangles processed
 */
int uvf_get_last_triangle_count();

/**
 * Get the file count from the last operation
 * @return Number of files processed
 */
int uvf_get_last_file_count();

/**
 * Get the group count from the last operation
 * @return Number of geometry groups created
 */
int uvf_get_last_group_count();

/**
 * Get the operation type of the last call
 * @return Operation type string (basic_uvf, structured_uvf, directory_multi, etc.)
 */
const char* uvf_get_last_operation_type();

// ====== Utility Functions ======

/**
 * Check if a path is a directory
 * @param path Path to check
 * @return 1 if directory, 0 if not
 */
int uvf_is_directory(const char* path);

/**
 * Count VTK files in a directory
 * @param dir_path Path to directory
 * @return Number of VTK files, -1 if error
 */
int uvf_count_vtk_files(const char* dir_path);

/**
 * Get API version
 * @return Version string
 */
const char* uvf_get_version();

#ifdef __cplusplus
}
#endif
