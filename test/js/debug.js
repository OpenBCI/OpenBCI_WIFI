const SerialPort = require('serialport');

// host: '/dev/tty.usbserial-DN0096JM'
let host = '/dev/tty.usbserial-DN00D1S2'
const serial = new SerialPort(host,{
  baudRate: 460800
})

serial.on('open',() => {
  console.log('open');
})

serial.on('data', data => {
  console.log(`data: ${data}`);
})

serial.on('close', () => {
  console.log(`closed`);
})
