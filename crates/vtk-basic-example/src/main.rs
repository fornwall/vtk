fn main() {
    let mut context = vtk::VtkContext::new();
    let mut device = context.create_device();
    let mut window = context.create_window(&mut device);

    let _ = std::thread::spawn(move || {
        let vertex_shader_bytes =
            include_bytes!(concat!(env!("OUT_DIR"), "/shaders/triangle.vert.spv"));
        let fragment_shader_bytes =
            include_bytes!(concat!(env!("OUT_DIR"), "/shaders/triangle.frag.spv"));

        let vertex_shader = device.create_shader(vertex_shader_bytes);
        let fragment_shader = device.create_shader(fragment_shader_bytes);

        loop {
            window.render();
        }
    });

    context.run();
}
