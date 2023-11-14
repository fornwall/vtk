#define VTK_ARRAY_SIZE(x) ((int)(sizeof(x) / sizeof((x)[0])))

#define VTK_ARRAY_ALLOC(element_type, array_size) ((element_type*) malloc(sizeof(element_type) * array_size))
