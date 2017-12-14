# Styphoon

Prototype bindings to link [Node.js](http://nodejs.org/) and the CoreEl Typhoon SDK, enabling asynchronous capture from 
[CoreEl Typhoon](http://coreel.com/solutions/digital-video-solutions/audio-video-products/typhoon/) devices via a simple Javascript API.

This is prototype software and is not yet suitable for production use. Currently supported on the Windows platform.

## Installation

Styphoon has a number of prerequisites:

1. Install [Node.js](http://nodejs.org/) for your platform. This software has been developed against the long term stable (LTS) release.
2. Install the latest version of the Typhoon Drivers and SDK (available from CoreEl).
3. Install [node-gyp](https://github.com/nodejs/node-gyp) and make sure that you have the prerequisites for compiling Javascript addons for your platform. This requires a C/C++ development kit and python v2.7.

Styphoon is designed to be used as a module included into another project. To include Styphoon into your project:

    npm install --save styphoon (TODO: validate this works)

## Building

TODO: write better build instructions - these instructions have holes in, but are written to at least point you in the right direction...

1) Create a CoreEl directory directly below the Styphoon root directory containing TyphoonSDK.h TyphoonSDK_64.lib copied from a previously obained
   copy of the Typhoon SDK, so that the directory structure looks as follows:

styphoon
|-CoreEl
|	|-TyphoonSDK.h
|	|-TyphoonSDK_64.lib
|-build
|-scratch
|-src
|-test

2) From the styphoon root directory run the following:

	node-gyp configure --msvs_version=2013
	node-gyp build

## Using styphoon

The easiest way to use Styphoon is to run it within Node-RED in conjunction with the [node-red-contrib-dynamorse-sdi-typhoon-io](https://github.com/Streampunk/node-red-contrib-dynamorse-sdi-typhoon-io)
Node-RED node. Once both Styphoon and this latter package are installed, it will be possible to obtain compressed and uncompressed video from the Typhoon card into the
Node-RED ecosystem.

For more information about installing and configuring the Typhoon card, please see the [Supplementary Manual](https://github.com/Streampunk/styphoon/blob/master/TyphoonSupplementaryManual.docx) included
in this repo.

## License

This software is released under the Apache 2.0 license. Copyright 2017 Streampunk Media Ltd.

