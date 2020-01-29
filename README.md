# BLE Device Service
The BLE Device Service connects Bluetooth Low Energy
devices with EdgeX. The Device Service
currently limited to Bluetooth GATT devices/profiles.

## Prerequisites
- A Linux build host.
- [GCC][gcc] that supports C11.
- [Make][make] version 4.1.
- [CMake][cmake] version 3.1 or greater.
- [Cbor][libcbor] version 0.5.
- [Device-SDK-C][device-sdk-c] version 1.0.0 or greater.
- [D-Bus][dbus] version 1.12.2.
- [BlueZ][bluez] version 5.48.

## Configuration File

In the Device Service ```res/configuration.toml```
file, you will find two extra configurable options
under the `[Driver]` table.

```
[Driver]
  BLE_Interface = "hci0"
  BLE_DiscoveryDuration = 20
```

#### BLE_Interface
BLE_Interface specifies which Host Controller
Interface (HCI) the Device Service will use.
You can find out, which interfaces are
available with BlueZ ```hciconfig``` tool.

#### BLE_DiscoveryDuration
When the Device Service starts, Bluetooth
discovery will be enabled for the time
amount specified by BLE_DiscoveryDuration
(in seconds). After which, Bluetooth discovery
will be disabled.

## Adding A Device
To add a new BLE device to the Device
Service, insert the layout below into the
configuration.toml file. Update the Name,
Profile, Description, Mac to match the device
you wish to add. Start the Device Service
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

## Build
To build a local version of the Device Service,
clone the repository and run the following
shell commands.

Before building the device service, please
ensure that you have the EdgeX Device-SDK-C installed and
make sure that the current directory is the device
service directory (device-ble-c). To build
the device service, enter the command below into
the command line to run the build script.

```shell
$ ./scripts/build.sh
```

In the event that your Device-SDK-C is not installed in the
system default paths, you may specify its location
using the environment variable CSDK_DIR (as the build will look
for $CSDK_DIR/include, $CSDK_DIR/lib etc).

After having built the device service, the executable
can be found at `build/{debug,release}/device-ble-c/device-ble-c`.

## Run
After successfully building the Device Service,
you should have the Device Service executable
in `./build/release/device-ble-c/` named
`device-ble-c`. To run this executable,
make sure your current directory is the root
project directory. As the Device Service
executable requires the configuration.toml in
`res/` and any device profiles in `profiles/`.

Run the following command in shell to start
the Device Service

```./build/release/device-ble-c/device-ble-c```


#### Command Line Options
|Option     | Shorthand Option  | Description                     |
|-----------|-------------------|:--------------------------------|
|--help     | -h                | Show help                       |
|--registry | -r                | Use the registry service        |
|--profile  | -p                | Set the profile name            |
|--confdir  | -c                | Set the configuration directory |

An example of using a command line option from the table above.
```./build/release/device-ble-c/device-ble-c -c res/```

## Docker

#### Build
To build a Docker image of the Device Service,
clone the repository and enter the following
shell commands.
```shell
$ cd device-ble-c
$ docker build -t device-ble \
  -f ./scripts/Dockerfile.alpine-3.9 \
  --build-arg arch=$(uname -m) .
```

#### Run
First, build a Docker image of the Device Service,
and enter the shell command below to run the Docker
image within a Docker container.

The Device Service docker container requires the
host machine `system_bus_socket` socket to be
mounted, to allow communication between the
container and the host machine D-Bus.

```shell
$ docker run \
  -v /var/run/dbus/system_bus_socket:/var/run/dbus/system_bus_socket \
  --privileged -p 49971:49971 device-ble
```
#### AppArmor
 If you are running the device service on a system with AppArmor,
 to be able to run the device service without requiring the `--privileged` 
 flag please apply the provided security policy  `docker-ble-policy` 
 using the command:
 
```shell
$ sudo apparmor_parser -r -W docker-ble-policy
```

If this security policy has been applied to the system, the `--security-opt` 
flag replaces the  `--privileged` flag when running the device service:

```shell
$ docker run \
  -v /var/run/dbus/system_bus_socket:/var/run/dbus/system_bus_socket \
  --security-opt apparmor=docker-ble-policy -p 49971:49971 device-ble
```

[libcbor]: https://github.com/PJK/libcbor
[device-sdk-c]: https://github.com/edgexfoundry/device-sdk-c
[dbus]: https://www.freedesktop.org/wiki/Software/dbus/
[bluez]: http://www.bluez.org/
[make]: https://www.gnu.org/software/make/
[cmake]: https://cmake.org/
[gcc]: https://gcc.gnu.org/
