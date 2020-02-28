# VirtualJoystick Multiplexer 

Takes some values passed in via a HidD_SetFeature call from user mode and makes
them look like a HID joystick device.

Usage: Build the driver (preferrably in release mode). Install the driver (use 
the vjoy_release.inf file), run the usermode JoystickMultiplexer and it will
push data from the X36 & Rudder Pedals into one virtual joystick that can be
used in apps that don't support multiple input devices.



## Compilation hints:

### JoystickMultiplexer 
Compiled with VS 2019 CE.

### Driver
Compiled in VS 2019 CE with Win10 DDK.


Notes:

This software is essentially completely untested. I've tried it very briefly on
Windows 10. It doesn't seem to cause any problems on my machines, but there's
no promise that it won't go nuts on other computers.

The source is based on the vhidmini sample app out of the Windows DDK, I'm not
completely sure as to the licensing of those samples, my changes to the sample
code as well as the user-mode JoystickMultiplexer app are released to the
public domain.


Support:
Unfortunately I don't have the free time to offer support of any kind, I didn't
like the fact that my nice new rudder pedals didn't work in 90% of the games I
own, so I fixed it. I also didn't like PPJoy's artificial limit on the number
of axes/buttons that can be pushed through to direct input, my code can be
freely tweaked to suit the user.

The current user-mode build is strongly tied to the Saitek X36 Flight Controller
and Pro Rudder Pedals. This is done via a simple string comparison in the
EnumJoysticksCB, so that's where you'd probably want to plug in the name of
your joysticks. Perhaps somebody will throw together a pretty GUI that
minimizes to the system tray and allows you to choose which stick/axis|button
pair maps to which virtual button.

I arbitrarily limited the number of supported buttons to 46, this can be
modified by changing the Button array in common.h's _DEVICE_REPORT struct as
well as by modifying both the INPUT and FEATURE fields for buttons in the
HIDReportDescriptor array (sys/HIDReportDescriptor.h). Each button is a single
bit, the total number of buttons should be something divisible by 8, so either
map more bits than you need, or use an extra Constant data field to hold the
leftover bits (check out the HID documentation on usb.org if you're
interested). DirectInput theoretically supports 128 buttons.

## Signing

http://woshub.com/how-to-sign-an-unsigned-driver-for-windows-7-x64/
Note that the tools for Win10 are under Program Files (x86)\Windows Kits\10\bin\...
