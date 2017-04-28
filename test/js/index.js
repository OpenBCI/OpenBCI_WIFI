const dgram = require('dgram');

const udpRx = dgram.createSocket('udp4');
const udpTx = dgram.createSocket('udp4');

const udpRxPort = 2391;
const udpTxPort = 2390;


///////////////////////////////////////////////////////////////
// UDP Rx "Server"                                           //
///////////////////////////////////////////////////////////////

udpRx.on('error', (err) => {
  console.log(`server error:\n${err.stack}`);
  udpRx.close();
});

let last = 0;
let packetSize = 33;

udpRx.on('message', (msg, rinfo) => {
  let good = true;

  console.log(`${msg.length}`);
  // if (last + 1 != msg[0]) {
  //   console.log(`missed packet: expected ${last + 1} got ${msg[0]}`);
  //   last = msg[0];
  //   good = false;
  // }
  // if (last + 2 != msg[packetSize]) {
  //   console.log(`missed packet: expected ${last + 2} got ${msg[packetSize]}`);
  //   last = msg[packetSize];
  //   good = false;
  // }
  // if (last + 3 != msg[packetSize * 2]) {
  //   console.log(`missed packet: expected ${last + 3} got ${msg[packetSize * 2]}`);
  //   last = msg[packetSize * 2];
  //   good = false;
  // }

  if (good) {
    last += 3;
  }

  if (last >= 255) last -= 255;
  // console.log(`udpRx got: ${msg} from ${rinfo.address}:${rinfo.port}`);
});

udpRx.on('listening', () => {
  var address = udpRx.address();
  console.log(`udpRx listening ${address.address}:${address.port}`);
});

udpRx.bind(udpRxPort);

var buf = new Buffer(`go`);
udpTx.send(buf,udpTxPort,"192.168.0.176");
