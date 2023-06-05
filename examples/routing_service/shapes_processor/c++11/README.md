# Example Code: Routing Service Processor

Below there are the instructions to build and run this example. All the commands
and syntax used assume a Unix-based system. If you run this example in a
different architecture, please adapt the commands accordingly.

## Building the Example :wrench:

To build this example, first run CMake to generate the corresponding build
files. We recommend you use a separate directory to store all the generated
files (e.g., ./build).

```sh
mkdir build
cd build
cmake -DBUILD_SHARED_LIBS=ON ..
```

**Note**: when compiling on a Windows 64-bit machine you will need to add the
`-A x64` parameter to the call to CMake. See
[Customizing the Build](#customizing-the-build) for more details.

Once you have run CMake, you will find a number of new files in your build
directory (the list of generated files will depend on the specific CMake
Generator). To build the example, run CMake as follows:

```sh
cmake --build .
```

**Note**: if you are using a multi-configuration generator, such as Visual
Studio solutions, you can specify the configuration mode to build as follows:

```sh
cmake --build . --config Release|Debug
```

Alternatively, you can use directly the generated infrastructure (e.g.,
Makefiles or Visual Studio Solutions) to build the example. If you generated
Makefiles in the configuration process, run make to build the example. Likewise,
if you generated a Visual Studio solution, open the solution and follow the
regular build process.

Upon success it will create a shared library file in the build directory.

## Running the Example

To run this example you will need two instances of *RTI Shapes Demo* and a
single instance of *RoutingService*.

To run *RoutingService*, you will need first to set up your environment as
follows:

```sh
export RTI_LD_LIBRARY_PATH=$NDDSHOME/lib/<ARCH>
```

where `<ARCH>` shall be replaced with the target architecture you used to build
the example in the previous step.

### Aggregation

1.  Run one instance of *ShapesDemo* on domain 0. This will be the publisher
    application. Publish a blue squares and blue circles.

2.  Run the other instance of *ShapesDemo* on domain 1. This will be the
    subscriber application. Subscribe to squares, circles and triangles and
    observe how no data is received.

3.  Now run *RoutingService* to cause the aggregation data from the publisher
    application to the subscriber application.

    Run the following command from the example build directory for *Windows*:

    ```sh
    %NDDSHOME%\bin\rtiroutingservice ^
        -cfgFile ..\RsShapesAggregator.xml ^
        -cfgName RsShapesAggregator
    ```

    And for *Linux*:

    ```sh
    $NDDSHOME/bin/rtiroutingservice \
        -cfgFile ../RsShapesAggregator.xml \
        -cfgName RsShapesAggregator
    ```

    You should see how the subscriber application receives samples from squares,
    circles and triangles:

    -   The squares and circles samples are exactly the same generated by the
      publisher application.

    -   Triangles samples follow the same direction than the squares and their
      size changes based on the vertical position of the circles.

4.  Repeat the first step but publish other colors. You should observe the same
    behavior in the subscriber application for the new colors.

5.  In the publisher application, delete all the *DataWriters*. You should see
    the instances being disposed. Now delete all *DataReaders* from the
    subscriber application.

### Splitter

1.  Run one instance of *ShapesDemo* on domain 0. This will be the publisher
    application. Publish a blue squares.

2.  Run the other instance of *ShapesDemo* on domain 1. This will be the
    subscriber application.

    Subscribe to squares, circles and triangles and observe how no data is
    received.

3.  Now run *RoutingService* to cause the aggregation data from the publisher
    application to the subscriber application.

    Run the following command from the example build directory for *Windows*:

    ```sh
    %NDDSHOME%/bin/rtiroutingservice ^
        -cfgFile ../RsShapesSplitter.xml ^
        -cfgName RsShapesSplitter
    ```

    And for *Linux*:

    ```sh
    $NDDSHOME/bin/rtiroutingservice \
        -cfgFile ../RsShapesSplitter.xml \
        -cfgName RsShapesSplitter
    ```

    You should see how the subscriber application receives samples from squares,
    circles and triangles. The squares samples are exactly the same generated by
    the publisher application. The circles and triangles samples are the result
    of the route that contains the *ShapesSplitter*.

4.  Repeat the first step but publish other colors. You should observe the same
    behavior in the subscriber application for the new colors.

5.  In the publisher application, delete all the *DataWriters*. You should see
    the instances being disposed. Now delete all *DataReaders* from the
    subscriber application.

## Customizing the Build

### Configuring Build Type and Generator

By default, CMake will generate build files using the most common generator for
your host platform (e.g., Makefiles on Unix-like systems and Visual Studio
Solutions on Windows). You can use the following CMake variables to modify the
default behavior:

-   `-DCMAKE_BUILD_TYPE` - specifies the build mode. Valid values are `Release`
    and `Debug`. See the [CMake documentation for more details
    (Optional)](https://cmake.org/cmake/help/latest/variable/CMAKE_BUILD_TYPE.html).

-   `-DBUILD_SHARED_LIBS` - specifies the link mode. Valid values are `ON` for
    dynamic linking and `OFF` for static linking. See [CMake documentation for
    more details
    (Optional)](https://cmake.org/cmake/help/latest/variable/BUILD_SHARED_LIBS.html).

-   `-G` - CMake generator. The generator is the native build system used to
    build the source code. All the valid values are described in the CMake
    documentation for [CMake
    Generators](https://cmake.org/cmake/help/latest/manual/cmake-generators.7.html).

For example, to build an example in Debug/Dynamic mode run CMake as follows:

```sh
cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_SHARED_LIBS=ON .. -G "Visual Studio 15 2017" -A x64
```

### Configuring Connext Installation Path and Architecture

The CMake build infrastructure will try to guess the location of your Connext
installation and the Connext architecture based on the default settings
for your host platform. If you installed Connext in a custom location, you
can use the `CONNEXTDDS_DIR` variable to indicate the path to your RTI Connext
installation folder. For example:

```sh
cmake -DCONNEXTDDS_DIR=/home/rti/rti_connext_dds-x.y.z ..
```

Also, if you installed libraries for multiple target architectures on your system
(i.e., you installed more than one target `.rtipkg` file), you can use the
`CONNEXTDDS_ARCH` variable to indicate the architecture of the specific libraries
you want to link against. For example:

```sh
cmake -DCONNEXTDDS_ARCH=x64Linux3gcc5.4.0 ..
```

### CMake Build Infrastructure

The `CMakeListst.txt` script that builds this example uses a generic CMake
function called `connextdds_add_example` that defines all the necessary constructs
to:

1.  Run RTI Code Generator to generate the serialization/deserialization code
    for the types defined in the IDL file associated with the example.

2.  Build the corresponding Publisher and Subscriber applications.

3.  Copy the `USER_QOS_PROFILES.xml` file into the directory where the publisher
    and subscriber executables are generated.

You will find the definition of `connextdds_add_example`, along with detailed
documentation, in
[resources/cmake/rticonnextdds-cmake-utils/cmake/Modules/ConnextDdsAddExample.cmake
](https://github.com/rticommunity/rticonnextdds-cmake-utils/blob/main/cmake/Modules/ConnextDdsAddExample.cmake).

For a more comprehensive example on how to build an RTI Connext application
using CMake, please refer to the
[hello_world](../../../connext_dds/build_systems/cmake/) example, which includes
a comprehensive `CMakeLists.txt` script with all the steps and instructions
described in detail.