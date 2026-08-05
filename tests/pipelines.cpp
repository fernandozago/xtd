// Tests are included on separated files to avoid the "multiple definition" error when linking the test binaries.
#include "pipelines/pipeline_tests.h" // IWYU pragma: export
#include "pipelines/segmented_byte_view_tests.h" // IWYU pragma: export
#include "pipelines/custom_pool_resource_tests.h" // IWYU pragma: export
#include "pipelines/position_tests.h" // IWYU pragma: export

// Allow experimental allocator support for testing purposes.
// This is not a public API and may be removed in future releases.
#define XTD_ALLOW_EXPERIMENTAL 