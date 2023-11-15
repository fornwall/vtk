fn main() {
    let mut application = vtk::VtkApplication::new();
    let mut device = vtk::VtkDevice::new(&mut application);
    let mut window = vtk::VtkWindow::new(&mut device);
    println!("Starting run..");
    window.debug_print();
    application.run();
    window.debug_print();
    println!("Finishing run..");
}
