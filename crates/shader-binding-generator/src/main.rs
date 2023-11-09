fn main() {
    let spirv_path = std::env::args().nth(1).unwrap();
    let spirv_bytes = std::fs::read(spirv_path).unwrap();
    let options = naga::front::spv::Options::default();
    let module = naga::front::spv::parse_u8_slice(&spirv_bytes, &options).unwrap();

    for entry_point in module.entry_points.iter() {
        for arg in entry_point.function.arguments.iter() {
            println!("ARGUMENT: {:?}", arg);
            //let arg_type = module.types.get(arg.ty.into());
            let arg_type = &module.types[arg.ty];
            println!("  TYPE NAME: {:?}", arg_type.name);
            println!("  TYPE INNER: {:?}", arg_type.inner);

            match arg.binding {
                Some(naga::Binding::BuiltIn(built_in)) => {
                    println!("Built-in binding: {:?}", built_in);
                }
                Some(naga::Binding::Location {
                         location,
                         second_blend_source,
                         interpolation,
                         sampling,
                     }) => {
                    println!("Location binding: {:?}", location);
                }
                _ => {
                    println!("NO BINDING");
                }
            }
        }
        //println!("entry point: {:?}", entry_point);
    }
}
