#include "uvf_c_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv) {
    printf("UVF C API Test Program\n");
    printf("API Version: %s\n\n", uvf_get_version());
    
    if (argc < 3) {
        printf("Usage: %s <input_path> <output_dir> [mode]\n", argv[0]);
        printf("  mode: basic, structured, directory (default: auto-detect)\n");
        return 1;
    }
    
    const char* input = argv[1];
    const char* output = argv[2];
    const char* mode = (argc > 3) ? argv[3] : "auto";
    
    printf("Input: %s\n", input);
    printf("Output: %s\n", output);
    printf("Mode: %s\n\n", mode);
    
    // Auto-detect mode if not specified
    int is_dir = uvf_is_directory(input);
    int result = 0;
    
    if (strcmp(mode, "auto") == 0) {
        if (is_dir) {
            printf("Auto-detected: Directory mode\n");
            result = generate_uvf_directory(input, output);
        } else {
            printf("Auto-detected: Single file structured mode\n");
            result = generate_uvf_structured(input, output);
        }
    } else if (strcmp(mode, "basic") == 0) {
        printf("Using basic mode\n");
        result = generate_uvf(input, output);
    } else if (strcmp(mode, "structured") == 0) {
        printf("Using structured mode\n");
        result = generate_uvf_structured(input, output);
    } else if (strcmp(mode, "directory") == 0) {
        printf("Using directory mode\n");
        if (!is_dir) {
            printf("Warning: Input is not a directory but directory mode requested\n");
        }
        result = generate_uvf_directory(input, output);
    } else {
        printf("Error: Unknown mode '%s'\n", mode);
        return 1;
    }
    
    // Print results
    if (result) {
        printf("\n✅ SUCCESS!\n");
        printf("Operation: %s\n", uvf_get_last_operation_type());
        printf("Files processed: %d\n", uvf_get_last_file_count());
        printf("Groups created: %d\n", uvf_get_last_group_count());
        printf("Points: %d\n", uvf_get_last_point_count());
        printf("Triangles: %d\n", uvf_get_last_triangle_count());
    } else {
        printf("\n❌ FAILED!\n");
        printf("Error: %s\n", uvf_get_last_error());
    }
    
    return result ? 0 : 1;
}
