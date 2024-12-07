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
import yaml from 'js-yaml'; // https://www.npmjs.com/package/js-yaml
import { appendFile, mkdir, readFile } from "fs"; // https://nodejs.org/api/fs.html
import mqtt from 'mqtt'; // https://www.npmjs.com/package/mqtt

const htmldir = process.cwd() + "/..";
let config;
let clients = [];

const optionsHeaders = {
  'Access-Control-Allow-Origin': '*',
  // Probably Needs: GET, OPTIONS HEAD, do not believe can do POST, PUT, DELETE yet but could be wrong about that.
  'Access-Control-Allow-Methods': 'GET,HEAD,OPTIONS',
  // Needs: Range; User-Agent; Not Needed: Authorization; Others are suggested in some online posts
  'Access-Control-Allow-Headers': 'Cache-Control, Content-Type, Content-Length, Range, User-Agent, X-Requested-With',
};
/*
// Not currently used, might add back
const responseHeaders = {
  'Access-Control-Allow-Origin': '*',  // Needed if have CORS issues with things included
  server: 'express/frugaliot',         // May be worth indicating
  Connection: 'keep-alive',            // Helps with load, but since serving static it might not be useful
  'Keep-Alive': 'timeout=5, max=1000', // Up to 5 seconds idle, 1000 requests max
};
app.use((req, res, next) => {
  res.set(responseHeaders);
  if (req.url.length > 1 && req.url.endsWith('/')) { // Strip trailing slash
    req.url = req.url.slice(0, req.url.length - 1);
    console.log(`Rewriting url to ${req.url}`);
  }
  next(); });
*/
function readYamlConfig(inputFilePathOrDescriptor, cb) {
  // Read configuration file and return object
  async.waterfall([
      (cb) => readFile(inputFilePathOrDescriptor, 'utf8', cb),
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
// ================== MQTT Client embedded in server ========================

// Manages a connection to a broker - each organization needs its own connection
class MqttOrganization {
  constructor(config_org, config_mqtt)
  {
    this.config_org = config_org; // Config structure currently: { name, mqtt_password, projects[ {name, track[]}]}
    this.config_mqtt = config_mqtt; // { broker }
    this.mqtt_client = null; // Object from library
    this.subscriptions = []; // [{topic, qos, cb(topic, message)}]
    this.status = "constructing";
  }
  mqtt_status_set(k) {
    console.log('mqtt', this.config_org.name, k);
    this.status = k;
  }
  startClient() {
    if (!this.mqtt_client) {
      // See https://stackoverflow.com/questions/69709461/mqtt-websocket-connection-failed
      // TODO-41 handle multiple projects -> multiple mqtt sessions
      this.mqtt_status_set("connecting");
      // TODO go thru the options at https://www.npmjs.com/package/mqtt#client-connect and check optimal
      // noinspection JSUnresolvedReference
      this.mqtt_client = mqtt.connect(this.config_mqtt.broker, {
        connectTimeout: 5000,
        username: this.config_org.userid || this.config_org.name,
        password: this.config_org.mqtt_password,
        // Remainder do not appear to be needed
        //hostname: "127.0.0.1",
        //port: 9012, // Has to be configured in mosquitto configuration
        //path: "/mqtt",
      });
      this.mqtt_client.on("connect", () => {
        this.mqtt_status_set('connect');
        this.configSubscribe();
      });
      this.mqtt_client.on("reconnect", () => {
        this.mqtt_status_set('reconnect');
        this.resubscribe();
      });
      for (let k of ['disconnect', 'close', 'offline', 'end']) {
        this.mqtt_client.on(k, () => {
          this.mqtt_status_set(k);
        });
      }
      this.mqtt_client.on('error', (error) => {
        this.mqtt_status_set("Error:" + error.message);
      });
      this.mqtt_client.on("message", (topic, message) => {
        // message is Buffer
        let msg = message.toString();
        console.log("Received", topic, " ", msg);
        this.dispatch(topic,msg);
      });
    }
  }
  subErr(err, val) {
    if (err) {
      console.log("Subscription failed", val, err);
    }
  }
  mqtt_subscribe(topic, qos) {
    this.mqtt_client.subscribe(topic, {qos: qos}, this.subErr);
  }
  subscribe(topic, qos, cb) {
    this.mqtt_subscribe(topic, qos);
    this.subscriptions.push({topic, qos, cb});
  }
  configSubscribe() {
    // noinspection JSUnresolvedReference
    for (let p of this.config_org.projects) {
      for (let n of p.nodes) { // Note that node could have name of '+' for tracking all of them
        for (let t of n.track) {
          let topic = `${this.config_org.name}/${p.name}/${n.id}/${t}`;
          // TODO-server for now its a generic messageReceived - may need some kind of action - for example if Config had a "control" rule
          this.subscribe(topic, 0, this.messageReceived.bind(this)); // TODO-66 think about QOS, add optional in YAML
        }
      }
    }
  }
  resubscribe() {
    for (let sub of this.subscriptions) {
      this.mqtt_subscribe(sub.topic, sub.qos);
    }
  }
  dispatch(topic, message) {
    for (let sub of this.subscriptions) {
      if (sub.topic === topic) {
        sub.cb(topic, message);
      }
    }
  }
  messageReceived(topic, message) {
    this.log(topic, message);
  }
  log(topic, message) {
    // TODO sanitize topic - remove any leading '/' and any '..'
    let path = `data/${topic}`;
    let dateNow = new Date();
    let filename = `${dateNow.toISOString().substring(0,10)}.csv`
    this.appendPathFile(path, filename, `${dateNow.valueOf()},"${message}"\n`);
  }
  appendPathFile(path, filename, message) {
    mkdir (path, {recursive: true}, (err/*, val*/) => {
      if (err) {
        console.error(err);
      } else {
        appendFile(path + "/" +filename, message, (err) => {
          if (err) console.log(err);
        });
      }
    })
  }
}
function startClient() {
  // noinspection JSUnresolvedReference
  for (let o of config.organizations) {
    let c = new MqttOrganization(o, config.mqtt); // Will subscribe when connects
    clients.push(c);
    c.startClient();
  }
}
// ============ END of MQTT client ================

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
app.get('/config.json', (req, res) => {
    res.status(200).json(config);
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
    // Use a 1 day cache to keep traffic down TODO might want to override for /data/
    // Its important that frugaliot.css is cached, or the UX will flash while checking it hasn't changed.
    app.use(express.static(htmldir,{immutable: true, maxAge: 1000*60*60*24}));
    startServer();
    startClient();
    cb(null,null);
  }
  // TODO-9 read project specific configurations from config.d
], (err/*, unused*/) => {
  console.log("Server running in background callbacks",err)
});

