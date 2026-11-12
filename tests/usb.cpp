#include "gchd.hpp"
#include "streamer.hpp"
#include <cstdio>
#include <unistd.h>
#include <stdexcept>
struct UsbTestAccess {
    static void arm(GCHD &g) { g.interfaceClaimed_=true; g.devh_=reinterpret_cast<libusb_device_handle *>(1); }
    static void initialized(GCHD &g) { g.isInitialized_=true; }
    static void firmware(GCHD &g, const char *path) { FILE *file=fopen(path,"rb");
        if (!file) { g.dlfirm(path); return; }
        try { g.uploadFirmware(file,16); } catch (...) { fclose(file); throw; }
        fclose(file); }
};
static int code=0, length=0, calls=0;
extern "C" int __wrap_libusb_bulk_transfer(libusb_device_handle *, unsigned char, unsigned char *, int, int *transferred, unsigned timeout) {
    if (!timeout) throw std::runtime_error("infinite timeout");
    ++calls;
    if (code != LIBUSB_ERROR_NO_DEVICE) *transferred=length;
    return code;
}
extern "C" int __wrap_libusb_control_transfer(libusb_device_handle *, uint8_t, uint8_t, uint16_t, uint16_t, unsigned char *, uint16_t, unsigned timeout) {
    if (!timeout) throw std::runtime_error("infinite timeout");
    return LIBUSB_ERROR_NO_DEVICE;
}
extern "C" void __wrap_libusb_close(libusb_device_handle *) {}
extern "C" int __wrap_libusb_release_interface(libusb_device_handle *, int) { return 0; }
static void check(bool ok) { if (!ok) throw std::runtime_error("USB regression"); }
template<class F> void throws(F f) { bool caught=false; try { f(); } catch (const usb_error &) { caught=true; } check(caught); }
int main() {
    Process process;
    {
        GCHD g(&process,InputSettings(),TranscoderSettings()); UsbTestAccess::arm(g);
        std::vector<unsigned char> b(10,0xab);
        code=LIBUSB_ERROR_TIMEOUT; length=188; g.stream(&b); check(b.size()==188);
        length=0; g.stream(&b); check(b.empty());
        code=LIBUSB_ERROR_OVERFLOW; length=16384; g.stream(&b); check(b.empty());
        code=LIBUSB_ERROR_IO; throws([&] { g.stream(&b); }); check(b.empty());
        code=LIBUSB_ERROR_NO_DEVICE; throws([&] { g.stream(&b); }); check(b.empty());
        const int before=calls; throws([&] { g.stream(&b); }); check(calls==before);
    }
    {
        GCHD g(&process,InputSettings(),TranscoderSettings()); UsbTestAccess::arm(g); UsbTestAccess::initialized(g);
        // Destructor encounters a disconnected device during control transfer and must not throw.
    }
    {
        GCHD g(&process,InputSettings(),TranscoderSettings()); UsbTestAccess::arm(g);
        throws([&] { UsbTestAccess::firmware(g,"/nonexistent/gchd-firmware"); });
        char path[]="/tmp/gchd-test-XXXXXX"; int fd=mkstemp(path); check(fd>=0);
        char data[16]{}; check(write(fd,data,sizeof data)==sizeof data); close(fd);
        code=0; length=1; throws([&] { UsbTestAccess::firmware(g,path); });
        code=LIBUSB_ERROR_TIMEOUT; throws([&] { UsbTestAccess::firmware(g,path); });
        code=0; length=16; const int before=calls; UsbTestAccess::firmware(g,path); check(calls==before+1);
        unlink(path);
        code=LIBUSB_ERROR_TIMEOUT; length=0;
        Streamer streamer(&g,&process,StreamOutput::Disk); check(streamer.disk.enable("/dev/null")==0);
        const auto start=std::chrono::steady_clock::now(); streamer.loop(0.02);
        check(std::chrono::steady_clock::now()-start<std::chrono::seconds(1));
        Process::setActive(false); throws([&] { std::vector<unsigned char> b; g.stream(&b); });
    }
}
