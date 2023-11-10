struct Application {
    int field_one;
};

enum MyEnum {
    ONE,
    TWO
};

typedef void* VkDevice;
typedef void* VkShaderModule;

typedef unsigned char uint8_t;
typedef unsigned long size_t;

VkShaderModule compile_shader(VkDevice device, uint8_t const* bytes, size_t size);
