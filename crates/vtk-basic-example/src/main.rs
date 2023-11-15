fn main() {
    let vertex_shader_bytes =
        include_bytes!(concat!(env!("OUT_DIR"), "/shaders/triangle.vert.spv"));
    let fragment_shader_bytes =
        include_bytes!(concat!(env!("OUT_DIR"), "/shaders/triangle.vert.spv"));

    let mut application = vtk::VtkApplication::new();
    let mut device = vtk::VtkDevice::new(&mut application);
    let mut window = vtk::VtkWindow::new(&mut device, |device| {
        println!("Callback called!");
    });
    println!("Starting run..");
    window.debug_print();
    application.run();
    window.debug_print();
    println!("Finishing run..");
}
