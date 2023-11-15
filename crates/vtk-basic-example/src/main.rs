struct TriangleApplication {
    value: u32,
}

impl vtk::VtkApplication for TriangleApplication {
    fn setup_window(&mut self, device: &mut vtk::VtkDevice, window: &mut vtk::VtkWindow) {
        println!("Setup body - value = {}", self.value);
    }
    fn render_frame(&mut self, device: &mut vtk::VtkDevice, window: &mut vtk::VtkWindow) {
        println!("Render frame - value = {}", self.value);
    }
}

fn main() {
    let application = TriangleApplication { value: 23897 };
    let mut application = Box::new(application);

    let vertex_shader_bytes =
        include_bytes!(concat!(env!("OUT_DIR"), "/shaders/triangle.vert.spv"));
    let fragment_shader_bytes =
        include_bytes!(concat!(env!("OUT_DIR"), "/shaders/triangle.vert.spv"));

    let mut context = vtk::VtkContext::new(application);
    let mut device = context.create_device();
    context.request_window(&mut device);
    println!("Starting run..");
    context.run();
    println!("Finishing run..");
}
