/* Copyright 2017 Streampunk Media Ltd.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

var H = require('highland');
var fs = require('fs');
var typhoon = require('../index.js');

var writeFile = H.wrapCallback(fs.writeFile);

var recorder = new typhoon.Capture(0, typhoon.bmdModeHD1080i50, typhoon.bmdFormat10BitYUV);

console.log(recorder);

var count = 0;
const zeros = '00000000';
var basePath = (process.argv[2]) ? process.argv[2] : 'test_';
var frames = (process.argv[3] && !isNaN(+process.argv[3])) ? +process.argv[3] : 250;

H('frame', recorder)
  .take(frames)
  .map(x => {
    var countess = `${count++}`;
    var name = `${basePath}_${zeros.slice(-(8 - countess.length))}${countess}.v210`;
    console.log(name);
    return writeFile(name, x);
  })
  .parallel(4)
  .done(() => {
    recorder.stop();
    console.log("Recording finished.");
    process.exit();
  });

recorder.on('error', console.error);

recorder.start();

process.on('SIGINT', () => {
  recorder.stop();
  process.exit();
});
