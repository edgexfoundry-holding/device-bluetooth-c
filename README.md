# BLE Device Service
The BLE device service connects Bluetooth Low Energy
devices with EdgeX. This device service currently only supports the 
BLE GATT profile.

Disclaimer: It is only possible to run this device service fully confined 
on distros with AppArmor enabled such as Ubuntu. At this time, this 
device service is not supported on any distro with SE Linux enabled by 
default.

## Prerequisites

This device service is only supported on Linux, as it requires both DBus and the BlueZ Bluetooth stack. 
Although it may be possible to get both running on MacOS or Windows, use on either is considered unsupported.

Only Ubuntu 16.04 and 18.04 have been tested for building and running 
this device service. Please note, BlueZ must be updated to at least the 
version specified below if running on a Ubuntu 16.04 host.

In addition to [Device-SDK-C][device-sdk-c] (version 1.1.1 or greater) and its 
dependencies (ie, cbor, yaml, curl, uuid, microhttpd) the following 
packages are required for building and running the device service.

### Building

- A Linux build host
- [Make][make]
- [GCC][gcc] version 5.x or greater.
- [CMake][cmake] version 3.0 or greater.
- [D-Bus][dbus] developer package version 1.10.6 or greater.

### Runtime 

- [D-Bus][dbus] library and system daemon version 1.10.6 or greater.
- [BlueZ][bluez] system daemon version 5.48 or greater.

## Configuration File

In the device service ```res/configuration.toml```
file, you will find two extra configurable options
under the `[Driver]` table.

```
[Driver]
  BLE_Interface = "hci0"
  BLE_DiscoveryDuration = 20
```

#### BLE_Interface
BLE_Interface specifies which Host Controller
Interface (HCI) the device service will use.
You can find out, which interfaces are
available with BlueZ ```hciconfig``` tool.

#### BLE_DiscoveryDuration
When the device service starts, Bluetooth
discovery will be enabled for the time
amount specified by BLE_DiscoveryDuration
(in seconds). After which, Bluetooth discovery
will be disabled. 

Discovery uses BlueZ to look for any Bluetooth devices around and logs 
their details. If a device has not been logged by BlueZ then the Device 
Service will not be able to perform a connection to the device to read 
or write a value.

## Adding A Device
To add a new BLE device to the Device
Service, insert the layout below into the
configuration.toml file. Update the Name,
Profile, Description, MAC to match the device
you wish to add. Start the device service
and make sure that the device you're wanting
to add is in advertising mode, when the Device
Service starts.

```
[[DeviceList]]
  Name = "SensorTag-01"
  Profile = "CC2650 SensorTag"
  Description = "TI SensorTag 2650 STK"
  Labels = [ "ble" ]
  [DeviceList.Protocols]
    [DeviceList.Protocols.BLE]
      MAC = "00:00:00:00:00:00"
```

## Device Resources
BLE GATT device resources are statically configured in the device 
profile. These resources map their GATT characteristic UUID’s to 
read/write specific values. A template device resource is provided.

```
name: "<device resource name>"
description: "<description>"
attributes:
  { characteristicUuid: "<GATT characteristic UUID>" }
properties:
  value:
    { type: "<data type>", readWrite: "<R/W/RW>" }
  units:
    { type: "<data type>", readWrite: "<R/W/RW>", defaultValue: "<default value>" }
```
### Conversions

Some GATT characteristics are vendor specific, meaning they don't 
follow standard data types. Some of these devices still return 
data in a final readable format, such as an Integer or a Float. However, 
some of these characteristics don't. In order to convert the data into a 
readable format that can be used within EdgeX, conversion options are 
available where each one implemented and registered in `conversion.c` is 
for a specific device resource.

The current conversion functions implemented for this Device Service are 
for the Texas Instruments C2650 SensorTag. Any device resources that have 
conversions implemented require two extra attributes, `startByte` and 
`rawType`. The `startByte` attribute specifies at which byte should the 
`rawType` be cast from. The casted value will then be handed to the 
conversion function. In the example provided for `CC2650HumidityData`, 
`startByte` is 3. The number of bytes in the array received from the 
device is 4.

```
name: "CC2650HumidityData"
description: "I/O Configuration"
attributes:
  { characteristicUuid: "f000aa21-0451-4000-b000-000000000000", startByte: "3", rawType: "Uint16" }
properties:
  value:
    { type: "Float32",  floatEncoding: "eNotation", readWrite: "RW" }
  units:
    { type: "String", readWrite: "R", defaultValue: "Mode" }
```

The supported `rawTypes` are as follows.
```
Uint8, Uint16, Uint32, Uint64, Int8, Int16, Int32, Int64, Float32, Float64
```

## Build
First clone the device service repository. Before building the device 
service, please ensure that you have the EdgeX Device-SDK-C installed 
and make sure that the current directory is the device service 
directory (device-ble-c). 

The SDK is installed by either unpacking the tar.gz file into an 
appropriate directory, or by installing the .deb or .rpm package by the 
usual method if you are on a platform where that's supported. 

To build the device service, enter the 
command below into the command line to run the build script.

```shell
$ ./scripts/build.sh
```

In the event that your Device-SDK-C is not installed in the
system default paths, you may specify its location
using the environment variable CSDK_DIR (as the build will look
for $CSDK_DIR/include, $CSDK_DIR/lib etc).

After having built the device service, the executable
can be found at `build/{debug,release}/device-ble/device-ble`.

## Run
After successfully building the device service,
you should have the device service executable
in `./build/release/device-ble/` named
`device-ble`. To run this executable,
make sure your current directory is the root
project directory. As the device service
executable requires the configuration.toml in
`res/` and any device profiles in `profiles/`.

The following shell command can be used to start the device service.

```./build/release/device-ble/device-ble```


#### Command Line Options
|Option     | Shorthand Option  | Description                     |
|-----------|-------------------|:--------------------------------|
|--help     | -h                | Show help                       |
|--registry | -r                | Use the registry service        |
|--profile  | -p                | Set the profile name            |
|--confdir  | -c                | Set the configuration directory |

An example of using a command line option from the table above.
```./build/release/device-ble/device-ble -c res/```

## Docker

#### Build
To build a Docker image of the device service,
clone the repository and enter the following
shell commands.


```shell
$ cd device-ble-c
$ docker build --no-cache -t device-ble \
  -f ./scripts/Dockerfile.alpine-3.9 \
  --build-arg arch=$(uname -m) .
```

#### Run

Firstly, build a Docker image of the device service.
The provided shell command can then be used to run the Docker image 
within a Docker container. 

The device service docker container requires the
host machine `system_bus_socket` socket to be
mounted, to allow communication between the
container and the host machine D-Bus.

```shell
$ docker run --rm \
  -v /var/run/dbus/system_bus_socket:/var/run/dbus/system_bus_socket \
  --privileged -p 49971:49971 --network=<docker-network> device-ble
```

Where `<docker-network>` is the name of the currently running Docker 
network, which the container is to be attached to.

#### AppArmor
 If you are running the device service on a system with AppArmor,
 to be able to run the device service without requiring the `--privileged` 
 flag please apply the provided security policy  `docker-ble-policy` 
 using the command:
 
```shell
$ sudo apparmor_parser -r -W docker-ble-policy
```

If this security policy has been applied to the system, the `--security-opt` 
option replaces the  `--privileged` flag when running the device service:

```shell
$ docker run \
  -v /var/run/dbus/system_bus_socket:/var/run/dbus/system_bus_socket \
  --security-opt apparmor=docker-ble-policy -p 49971:49971 
  --network=<docker-network> device-ble
```

[libcbor]: https://github.com/PJK/libcbor
[device-sdk-c]: https://github.com/edgexfoundry/device-sdk-c
[dbus]: https://www.freedesktop.org/wiki/Software/dbus/
[bluez]: http://www.bluez.org/
[make]: https://www.gnu.org/software/make/
[cmake]: https://cmake.org/
[gcc]: https://gcc.gnu.org/
