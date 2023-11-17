fn main() {
    let spirv_path = std::env::args().nth(1).unwrap();
    let spirv_bytes = std::fs::read(spirv_path).unwrap();
    let options = naga::front::spv::Options::default();
    let module = naga::front::spv::parse_u8_slice(&spirv_bytes, &options).unwrap();

    println!("VkVertexInputAttributeDescription vk_vertex_input_attribute_descriptions [] = {{");

    let mut offset = 0;
    for entry_point in module.entry_points.iter() {
        for arg in entry_point.function.arguments.iter() {
            //let arg_type = module.types.get(arg.ty.into());
            let arg_type = &module.types[arg.ty];

            // https://vulkan-tutorial.com/Vertex_buffers/Vertex_input_description
            let type_name = match arg_type.inner {
                naga::TypeInner::Scalar {
                    kind: naga::ScalarKind::Float,
                    width: 4
                } => "VK_FORMAT_R32_SFLOAT",
                naga::TypeInner::Vector {
                    size,
                    kind: naga::ScalarKind::Float,
                    width: 4
                } => {
                    match size {
                        naga::VectorSize::Bi => "VK_FORMAT_R32G32_SFLOAT",
                        naga::VectorSize::Tri => "VK_FORMAT_R32G32B32_SFLOAT",
                        naga::VectorSize::Quad => "VK_FORMAT_R32G32B32A32_SFLOAT",
                    }
                }
                _ => {
                    panic!("Unhandled type: {:?}", arg_type.inner);
                }
            };
            //println!("  TYPE NAME: {:?}", arg_type.name);
            //println!("  TYPE INNER: {:?}", arg_type.inner);

            match arg.binding {
                Some(naga::Binding::BuiltIn(built_in)) => {
                    //eprintln!("Built-in binding: {:?}", built_in);
                }
                Some(naga::Binding::Location {
                         location,
                         second_blend_source,
                         interpolation,
                         sampling,
                     }) => {
                    println!("    {{");
                    //println!("Location binding: {:?}", location);
                    println!("        .binding = 0,");
                    println!("        .location = {location},");
                    println!("        .format = {type_name},"); //.format = VK_FORMAT_R32G32B32_SFLOAT,
                    println!("        .offset = {offset},");
                    println!("    }},");
                }
                _ => {
                    //println!("NO BINDING");
                }
            }

            offset += arg_type.inner.size(module.to_ctx());
        }
        //println!("entry point: {:?}", entry_point);
    }
    println!("}};");

    println!("\nVkVertexInputBindingDescription vk_vertex_input_binding_descriptions[] = {{
    {{
        .binding = 0,
        .stride = {offset}
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    }}
}};");
}
