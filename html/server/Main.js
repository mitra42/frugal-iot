/*
  Simple server for FrugalIoT

  * Serves up UI files
  * Maintains a client that watches MQTT broker
  * Has data management and graphing functionality
 */
/* Notes for dev (to be deleted)
  * Deleted from template: "debug"; fs; sqlite;
 */
import express from 'express'; // http://expressjs.com/
import morgan from 'morgan'; // https://www.npmjs.com/package/morgan - http request logging
import async from 'async'; // https://caolan.github.io/async/v3/docs.html
import yaml from 'js-yaml';
import fs from "fs"; // https://www.npmjs.com/package/js-yamlc

//const htmldir = process.cwd() + "/.."; // TODO move this to point at wherever index.html relative to Main.js
const htmldir = "/Users/mitra/Documents/Arduino/frugal-iot/html"
let config;


const optionsHeaders = {
  'Access-Control-Allow-Origin': '*',
  // Probably Needs: GET, OPTIONS HEAD, do not believe can do POST, PUT, DELETE yet but could be wrong about that.
  'Access-Control-Allow-Methods': 'GET,HEAD,OPTIONS',
  // Needs: Range; User-Agent; Not Needed: Authorization; Others are suggested in some online posts
  'Access-Control-Allow-Headers': 'Cache-Control, Content-Type, Content-Length, Range, User-Agent, X-Requested-With',
};
const responseHeaders = {
  'Access-Control-Allow-Origin': '*',
  server: 'express/frugaliot',
  Connection: 'keep-alive',
  'Keep-Alive': 'timeout=5, max=1000', // Up to 5 seconds idle, 1000 requests max
};

function readYamlConfig(inputFilePathOrDescriptor, cb) {
  // Read configuration file and return object
  async.waterfall([
      (cb) => fs.readFile(inputFilePathOrDescriptor, 'utf8', cb),
      (yamldata, cb) => cb(null, yaml.loadAll(yamldata, {onWarning: (warn) => console.log('Yaml warning:', warn)})),
    ],
    cb
  );
}
function startServer() {
  const server = app.listen(config.server.port); // Intentionally same port as Python gateway defaults to, api should converge
  console.log('Server starting on port %s', config.server.port);
  server.on('error', (err) => {
    if (err.code === 'EADDRINUSE') {
      console.log('A server, probably another copy of this, is already listening on port %s', config.server.port);
    } else {
      console.log('Server hit error %o', err);
      throw (err); // Will be uncaught exception
    }
  });
}

function startClient() {
  console.log("MQTT client will go here")
}

const app = express();

// Respond to options - not sure if really needed, but seems to help in other servers.
app.options('/', (req, res) => {
  res.set(optionsHeaders);
  res.sendStatus(200);
});

// app.use(express.json()); // Uncomment if expecting Requests with a JSON body http://expressjs.com/en/5x/api.html#express.json


app.get('/echo', (req, res) => {
  res.status(200).json(req.headers);
});

// Main for server
async.waterfall([
  (cb) => readYamlConfig('./config.yaml', cb),
  (configobj, cb) => {
    config = configobj[0];
    console.log("Config=",config);
    // Could genericize config defaults
    if (!config.morgan) { config.morgan = ':method :url :req[range] :status :res[content-length] :response-time ms :req[referer]'}
    // Seems to be writing to syslog which is being cycled.
    app.use(morgan(config.morgan)); // see https://www.npmjs.com/package/morgan )
    console.log("Serving from",htmldir);
    app.use(express.static(htmldir,{}));
    startServer();
    startClient();
    cb(null,null);
  }
  // TODO read project specfic configurations from config.d
], (err, unused) => {});

