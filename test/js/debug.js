const SerialPort = require('serialport'),
bufferEqual = require('buffer-equal');

// host: '/dev/tty.usbserial-DN0096JM'
let host = '/dev/tty.usbserial-DN00D1S2'
const serial = new SerialPort(host,{
  baudRate: 230400
})

let last = 0;

serial.on('open',() => {
  console.log('open');
})

serial.on('data', data => {
  // console.log(data);
  for (var i = 0; i < data.length; i++) {
    console.log(data[i]);
  }
  // processBytes(data);
})

serial.on('close', () => {
  console.log(`closed`);
})

let startByte = 0x41;
let stopByte = 0xC0;
let packetSize = 33;
let buffer = new Buffer(1024);

const OBCIPacketPositionChannelDataStart   = 2;  // 0:startByte | 1:sampleNumber | [2:4] | [5:7] | [8:10] | [11:13] | [14:16] | [17:19] | [21:23] | [24:26]
const OBCIPacketPositionChannelDataStop    = 25; // 24 bytes for channel data
const OBCIPacketPositionSampleNumber       = 1;
const OBCIPacketPositionStartByte          = 0;  // first byte
const OBCIPacketPositionStopByte           = 32; // [32]
const OBCIPacketPositionStartAux           = 26; // [26,27]:Aux 1 | [28,29]:Aux 2 | [30,31]:Aux 3
const OBCIPacketPositionStopAux            = 31; // - - - [30,31]:Aux 3 | 32: Stop byte
const OBCIPacketPositionTimeSyncAuxStart   = 26;
const OBCIPacketPositionTimeSyncAuxStop    = 28;
const OBCIPacketPositionTimeSyncTimeStart  = 28;
const OBCIPacketPositionTimeSyncTimeStop   = 32;
const OBCIStreamPacketStandardAccel      = 0; // 0000
const OBCIStreamPacketStandardRawAux     = 1; // 0001
const OBCIStreamPacketUserDefinedType    = 2; // 0010
const OBCIStreamPacketAccelTimeSyncSet   = 3; // 0011
const OBCIStreamPacketAccelTimeSynced    = 4; // 0100
const OBCIStreamPacketRawAuxTimeSyncSet  = 5; // 0101
const OBCIStreamPacketRawAuxTimeSynced   = 6; // 0110
const OBCIStreamPacketTimeByteSize = 4;

var processBytes = data => {
  // Concat old buffer
  let oldDataBuffer = null;
  if (buffer) {
    oldDataBuffer = buffer;
    data = Buffer.concat([buffer,data],data.length + buffer.length);
  }

  buffer = processDataBuffer(data);

  if (buffer && oldDataBuffer) {
    if (bufferEqual(buffer,oldDataBuffer)) {
      buffer = null;
    }
  }

};

let timeOfPacketArrival = 0;

var processDataBuffer = dataBuffer => {
  if (!dataBuffer) return null;
  var bytesToParse = dataBuffer.length;
  // Exit if we have a buffer with less data than a packet
  if (bytesToParse < packetSize) return dataBuffer;

  var parsePosition = 0;
  // Begin parseing
  while (parsePosition <= bytesToParse - packetSize) {
    // Is the current byte a head byte that looks like 0xA0
    if (dataBuffer[parsePosition] === startByte) {
      // Now that we know the first is a head byte, let's see if the last one is a
      //  tail byte 0xCx where x is the set of numbers from 0-F (hex)
      if (isStopByte(dataBuffer[parsePosition + packetSize - 1])) {
        /** We just qualified a raw packet */
        // This could be a time set packet!
        timeOfPacketArrival = Date.now();
        // Grab the raw packet, make a copy of it.
        let rawPacket = new Buffer(dataBuffer.slice(parsePosition, parsePosition + packetSize));

        // Emit that buffer
        // this.emit('rawDataPacket',rawPacket);
        // Submit the packet for processing
        processQualifiedPacket(rawPacket);
        // Overwrite the dataBuffer with a new buffer
        var tempBuf;
        if (parsePosition > 0) {
          tempBuf = Buffer.concat([dataBuffer.slice(0,parsePosition),dataBuffer.slice(parsePosition + packetSize)],dataBuffer.byteLength - packetSize);
        } else {
          tempBuf = dataBuffer.slice(packetSize);
        }
        if (tempBuf.length === 0) {
          dataBuffer = null;
        } else {
          dataBuffer = new Buffer(tempBuf);
        }
        // Move the parse position up one packet
        parsePosition = -1;
        bytesToParse -= packetSize;
      }
    }
    parsePosition++;
  }

  return dataBuffer;
};

var isStopByte = byte => {
  return (byte & 0xF0) === stopByte;
};

var getRawPacketType = stopByte => {
  return stopByte & 0xF;
}

var processQualifiedPacket = rawDataPacketBuffer => {
  if (!rawDataPacketBuffer) return;
  if (rawDataPacketBuffer.byteLength !== packetSize) return;
  var packetType = getRawPacketType(rawDataPacketBuffer[OBCIPacketPositionStopByte]);
  switch (packetType) {
    case OBCIStreamPacketStandardAccel:
    case OBCIStreamPacketStandardRawAux:
      sampleNumberTest(rawDataPacketBuffer);
      break;
    case OBCIStreamPacketAccelTimeSyncSet:
    case OBCIStreamPacketRawAuxTimeSyncSet:
    case OBCIStreamPacketAccelTimeSynced:
    case OBCIStreamPacketRawAuxTimeSynced:
      timeTest(rawDataPacketBuffer);
      break;
    default:
    // Don't do anything if the packet is not defined
    break;
  }
};

var sampleNumberTest = dataBuf => {

  let sampleNumber = dataBuf[OBCIPacketPositionSampleNumber];

  if (last + 1 != sampleNumber) {
    console.log(`missed packet: expected ${last + 1} got ${sampleNumber}`);
    last = sampleNumber;
  } else {
    last++;
  }
  if (last >= 255) last = -1;

}


var timeTest = dataBuf => {
  // Ths packet has 'A0','00'....,'00','00','FF','FF','FF','FF','C3' where the 'FF's are times
  const lastBytePosition = packetSize - 1; // This is 33, but 0 indexed would be 32 minus 1 for the stop byte and another two for the aux channel or the

  let time = dataBuf.readUInt32BE(lastBytePosition - OBCIStreamPacketTimeByteSize);

  console.log(time);
  // if (time - last < 3 || time - last > 5) {
  //   console.log(`missed packet: expected ${time - last} to equal ${4}`);
  // }
  // last = time;
}
