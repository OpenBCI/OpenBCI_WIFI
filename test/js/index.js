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

udpRx.on('message', (msg, rinfo) => {
  // console.log(`${msg}`);
  console.log(`udpRx got: ${msg} from ${rinfo.address}:${rinfo.port}`);
});

udpRx.on('listening', () => {
  var address = udpRx.address();
  console.log(`udpRx listening ${address.address}:${address.port}`);
});

udpRx.bind(udpRxPort);

var buf = new Buffer(`go`);
udpTx.send(buf,udpTxPort);
