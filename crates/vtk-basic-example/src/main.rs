fn main() {
    let vertex_shader_bytes =
        include_bytes!(concat!(env!("OUT_DIR"), "/shaders/triangle.vert.spv"));
    let fragment_shader_bytes =
        include_bytes!(concat!(env!("OUT_DIR"), "/shaders/triangle.vert.spv"));

    let mut context = vtk::VtkContext::new();
    let mut device = context.create_device();
    let mut window = context.create_window(&mut device);

    let _ = std::thread::spawn(move || {
        loop {
            window.render();
        }
    });

    context.run();
}
