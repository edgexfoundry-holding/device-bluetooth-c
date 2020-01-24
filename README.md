# Bluetooth Device Service
The Bluetooth Device Service connects Bluetooth
devices with EdgeX. The Device Service
currently limited to Bluetooth GATT devices/profiles.

## Dependencies

- [Device-C-SDK](https://github.com/edgexfoundry/device-sdk-c) -
The device c sdk provides a framework for building
EdgeX Device Services in C.

- [D-Bus](https://www.freedesktop.org/wiki/Software/dbus/) -
D-Bus is an Inter-Process Communication (IPC) and
Remote Procedure Calling (RPC) mechanism
specifically designed for efficient and easy-to-use
communication between processes running on the same
machine.

## Prerequisites
- [D-Bus](https://www.freedesktop.org/wiki/Software/dbus/) -
D-Bus daemon is required to be running on the host machine
alongside the Device Service to allow communication
between the Bluetooth Device Service and the BlueZ daemon.
 
- [BlueZ](http://www.bluez.org/) -
BlueZ Linux module is required to be installed on the host
machine. This allows the connection between the Device Service
and Bluetooth devices using D-Bus.

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
To add a new Bluetooth device to the Device
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
  Labels = [ "bluetooth" ]
  [DeviceList.Protocols]
    [DeviceList.Protocols.BLE]
      MAC = "00:00:00:00:00:00"
```

## Build
To build a local version of the Device Service,
clone the repository and run the following
shell commands.

```shell
$ cd device-bluetooth-c
$ ./scripts/build.sh
```

## Run
After successfully building the Device Service,
you should have the Device Service executable
in `./build/release/device-bluetooth-c/` named
`device-bluetooth-c`. To run this executable,
make sure your current directory is the root
project directory. As the Device Service
executable requires the configuration.toml in
`res/` and any device profiles in `profiles/`.

Run the following command in shell to start
the Device Service

```./build/release/device-bluetooth-c/device-bluetooth-c```


#### Command Line Options
|Option     | Shorthand Option  | Description                     |
|-----------|-------------------|:--------------------------------|
|--help     | -h                | Show help                       |
|--registry | -r                | Use the registry service        |
|--profile  | -p                | Set the profile name            |
|--confdir  | -c                | Set the configuration directory |

An example of using a command line option from the table above.
```./build/release/device-bluetooth-c/device-bluetooth-c -c res/```

## Docker

#### Build
To build a Docker image of the Device Service,
clone the repository and enter the following
shell commands.
```shell
$ cd device-bluetooth-c
$ docker build -t device-bluetooth \
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
  --privileged -p 49971:49971 device-bluetooth
```
