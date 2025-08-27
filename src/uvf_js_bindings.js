/**
 * UVF JavaScript API Bindings
 * Enhanced C++ API wrapper for WebAssembly
 */

// Basic Functions
Module['parse_vtp'] = Module.cwrap('parse_vtp', 'number', ['string']);
Module['generate_uvf'] = Module.cwrap('generate_uvf', 'number', ['string', 'string']);

// Enhanced Functions
Module['generate_uvf_structured'] = Module.cwrap('generate_uvf_structured', 'number', ['string', 'string']);
Module['generate_uvf_directory'] = Module.cwrap('generate_uvf_directory', 'number', ['string', 'string']);

// Status Functions
Module['uvf_get_last_error'] = Module.cwrap('uvf_get_last_error', 'string', []);
Module['uvf_get_last_point_count'] = Module.cwrap('uvf_get_last_point_count', 'number', []);
Module['uvf_get_last_triangle_count'] = Module.cwrap('uvf_get_last_triangle_count', 'number', []);
Module['uvf_get_last_file_count'] = Module.cwrap('uvf_get_last_file_count', 'number', []);
Module['uvf_get_last_group_count'] = Module.cwrap('uvf_get_last_group_count', 'number', []);
Module['uvf_get_last_operation_type'] = Module.cwrap('uvf_get_last_operation_type', 'string', []);

// Utility Functions
Module['uvf_is_directory'] = Module.cwrap('uvf_is_directory', 'number', ['string']);
Module['uvf_count_vtk_files'] = Module.cwrap('uvf_count_vtk_files', 'number', ['string']);
Module['uvf_get_version'] = Module.cwrap('uvf_get_version', 'string', []);

// High-level JavaScript wrapper
Module['UVF'] = {
    /**
     * Convert VTK files to UVF format
     * @param {string} input - Input file or directory path
     * @param {string} output - Output directory path
     * @param {string} mode - 'basic', 'structured', or 'directory'
     * @returns {Object} Result with success flag and statistics
     */
    convert: function(input, output, mode = 'directory') {
        let result = {
            success: false,
            error: '',
            stats: {
                points: 0,
                triangles: 0,
                files: 0,
                groups: 0,
                operation: ''
            }
        };
        
        let success = 0;
        
        switch(mode) {
            case 'basic':
                success = Module.generate_uvf(input, output);
                break;
            case 'structured':
                success = Module.generate_uvf_structured(input, output);
                break;
            case 'directory':
                success = Module.generate_uvf_directory(input, output);
                break;
            default:
                result.error = 'Invalid mode: ' + mode;
                return result;
        }
        
        result.success = (success === 1);
        
        if (!result.success) {
            result.error = Module.uvf_get_last_error();
        }
        
        // Get statistics
        result.stats.points = Module.uvf_get_last_point_count();
        result.stats.triangles = Module.uvf_get_last_triangle_count();
        result.stats.files = Module.uvf_get_last_file_count();
        result.stats.groups = Module.uvf_get_last_group_count();
        result.stats.operation = Module.uvf_get_last_operation_type();
        
        return result;
    },
    
    /**
     * Check if input is a directory
     * @param {string} path - Path to check
     * @returns {boolean} True if directory
     */
    isDirectory: function(path) {
        return Module.uvf_is_directory(path) === 1;
    },
    
    /**
     * Count VTK files in directory
     * @param {string} dirPath - Directory path
     * @returns {number} Number of VTK files, -1 if error
     */
    countVTKFiles: function(dirPath) {
        return Module.uvf_count_vtk_files(dirPath);
    },
    
    /**
     * Get API version
     * @returns {string} Version string
     */
    getVersion: function() {
        return Module.uvf_get_version();
    },
    
    /**
     * Parse and validate a VTP file
     * @param {string} vtpPath - Path to VTP file
     * @returns {Object} Result with success flag and basic stats
     */
    parseVTP: function(vtpPath) {
        const success = Module.parse_vtp(vtpPath);
        return {
            success: success === 1,
            error: success === 1 ? '' : Module.uvf_get_last_error(),
            points: Module.uvf_get_last_point_count(),
            triangles: Module.uvf_get_last_triangle_count()
        };
    }
};
