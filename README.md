# poleko2024

## About
This is a project that got awarded a 2rd place in a contest organised by a Polish company
[POL-EKO](https://www.pol-eko.com.pl/en/).
Application running on ESP32 makes use of [arduino-esp32](https://github.com/espressif/arduino-esp32) and
[PlatformIO](https://platformio.org/).
Web app is made using C#, making use of .NET 8 (latest version at the time of development) with Blazor as the UI 
solution and MariaDB as the database solution.

Manual in Polish is available [here](https://github.com/schmaldeo/poleko2024/blob/master/instrukcja.docx).

### ESP32
The ESP32 makes use of a proprietary weather probe. When it's connected to a wireless network, it can do 3 things:
1. Announce itself in the network by sending UDP packets every given interval. The monitoring app then detects it and
gives the user a possibility to monitor it.
2. Establish a TCP connection with the monitoring app and periodically send measurements to it (there is a possibility
to adjust the interval at which the data is sent).
3. Send a single measurement over HTTP.

The device indicates its current network status with the LED positioned on the right side of the USB port and the red
power LED. If it's illuminated, it means that the device is connected to a network. If it's not, it changes its network 
module operating mode to access point which allows the user to connect to it and connect to a network as well as 
adjust its network (like the IP address, subnet mask or default gateway) statically as well as change
them back to default (make use of DHCP). The device automatically enters this mode when it's not connected to a network
and hasn't got one saved in the memory or if it can't connect to a saved network. You can achieve the same by pressing 
the _BOOT_ button on the device.

### Monitoring app
The monitoring app runs on a server. To use it, you're required to sign up. The first registered account receives a
_Super Admin_ role, which allows it to change other accounts' roles. Other roles are Admin and User. Admin can add new
devices, change their settings within the app and stop monitoring. Users can only view current readings.

## Screenshots
TBA