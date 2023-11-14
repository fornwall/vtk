#include <Cocoa/Cocoa.h>

#include "vtk_cffi.h"

struct VtkMacApplication {
    NSApplication* ns_application;
};

_Bool vtk_application_init(struct VtkApplication* application) {
    id ns_application = [NSApplication sharedApplication];
    [ns_application setActivationPolicy:NSApplicationActivationPolicyRegular];

    struct VtkMacApplication* vtk_mac_application = malloc(sizeof(struct VtkMacApplication));
    vtk_mac_application->ns_application = ns_application,

    application->platform_specific = vtk_mac_application;

    return true;
}
