"use strict"

// Usage Notes.
var usage = "node sws.js <port> <dir>"

// Imports.
var dgram    = require('dgram')
  , fs     = require('fs')
  , events = require('events');

// Local Variables.
var options = { port: null, dir: null }
  , socket  = dgram.createSocket('udp4');

// The main worker.
var main = function() {
  // Enough args?
  if (process.argv.length != 4) {
    console.log(usage);
    process.exit(1);
  };
  // Declare our options.
  options = {
    port: process.argv[2]
  , dir:  process.argv[3]
  };
  // Create a UDP socket to listen on.
  socket.bind(options.port);

  // Wait for EOF from stdin.
  process.stdin.resume();
}

// On error, bail.
socket.on("error", function(error) {
  console.log("Socket error:\n" + err.stack);
  socket.close();
  process.exit(1);
});

// When we get a message, we need to respond.
socket.on("message", function(message, remote) {
  var message = message.toString().split(" ")
  // Parse the file requested.
  var request = {
    method: message[0]
  , file:   message[1]
  }

  var response = {
    status:  "200 OK"
  , data:    ""
  , string:  ""
  , buffer:  function() { 
      return new Buffer(this.string);
    }
  }
  
  if (request.file.indexOf("/../") != -1) {
    // Bad Request.
    response.status = "400 BAD REQUEST";
  } else if (request.file.slice(-1) == "/") {
    // Index Request. We need to modify the string.
    request.file = request.file + "index.html"
  }
  
  // Open the file.
  fs.readFile(options.dir + request.file, function(err, data) {
    if (err) {
      response.status = "400 BAD REQUEST";
    } else if (data) {
      data = data.toString();
    }
    // Respond to the message.
    response.string = "HTTP/1.1 " + response.status + "\r\n\r\n"
    // Insert file data if neccessary.
    if (typeof data !== "undefined" && data !== null) {
      response.string += data
    }
    var buffer = response.buffer()
      , front  = 0;
    while (front < buffer.length) {
      var packet = buffer.slice(front);
      var end    = 576;
      if (packet.length < 576) {
        end = packet.length
      }
      socket.send(packet, 0, end, remote.port, remote.address);
      front += end;
    }
  });  
});

// Once we're connected, announce it.
socket.on("listening", function() {
  var address = socket.address();
  console.log("Socket listening on: " + address.address + ":" + address.port);
})

// Startup.
main();