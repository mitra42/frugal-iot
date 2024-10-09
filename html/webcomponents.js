/*
 * Simple MQTT template for FrugalIoT
 */
import {EL, HTMLElementExtended, getUrl} from './node_modules/html-element-extended/htmlelementextended.js';
import mqtt from './node_modules/mqtt/dist/mqtt.esm.js';

var mqtt_client;
var mqtt_subscriptions = [];

function mqtt_subscribe(topic, cb) {
  mqtt_subscriptions.push({topic, cb});
  mqtt_client.subscribe(topic, (err) => { if (err) console.error(err); })
}
class MqttClient extends HTMLElementExtended {
  // TODO consider reconnection - see mqtt's README.md
  static get observedAttributes() {
    return ['server',]; }

  setStatus(text) {
    this.state.status = text;
    this.renderAndReplace();
  }
  shouldLoadWhenConnected() { return !!this.state.server; } /* Only load when has a server specified */

  loadContent() {
    //console.log("loadContent", this.state.server);
    if (!mqtt_client) {
      // this.state.client = mqtt.connect(this.state.server); //
      // See https://stackoverflow.com/questions/69709461/mqtt-websocket-connection-failed
      //const URL = "ws://localhost:9012";

      this.setStatus("Connecting");
      mqtt_client = mqtt.connect(this.state.server, {
        connectTimeout: 5000,
        username: "public",
        password: "public",
        // Remainder dont appear to be needed
        //hostname: "127.0.0.1",
        //port: 9012, // Has to be configured in mosquitto configuration
        //path: "/mqtt",
      });
      mqtt_client.on("connect", () => {
        console.log("connected");
        this.setStatus("Connected");
        /*
        mqtt_client.subscribe("presence", (err) => {
          console.log("Subscribed to presence");
          if (!err) {
            mqtt_client.publish("presence", "Hello mqtt");
          }
        });
         */
        // TODO simple message sent by local client, index.html can watch for it to know the local route is working
        this.setStatus("Testing presence");
        mqtt_client.publish("presence", "Hello mqtt");
      });
      mqtt_client.on('error', function (error) {
        console.log(error);
        this.state.status = "Error:" + error.message;
      });
      mqtt_client.on("message", (topic, message) => {
        // message is Buffer
        let msg = message.toString();
        console.log("Received", topic, " ", msg);
        for (let o of mqtt_subscriptions) {
          console.log("Dispatch testing: ", topic)
          if (o.topic === topic) {
            console.log("Dispatching: ", topic, msg)
            o.cb(topic, msg)
          }
        }
        //mqtt_client.end();
      });
    } else {
      console.log("XXX already started connection")
    }
  }

  // TODO display some more about the client and its status.
  render() {
    console.log("render");
    return [
      EL('div', {},[
        EL('span',{class: 'demo', textContent: "MQTT Client"}),
        EL('span',{class: 'demo', textContent: "server: "+this.state.server}),
        EL('span',{class: 'demo', textContent: "   status:"+this.state.status}),
      ]),
    ];
  }
}
customElements.define('mqtt-client', MqttClient);

class MqttElement extends HTMLElementExtended {
  shouldLoadWhenConnected() { return !!mqtt_client; }
}

class MqttText extends MqttElement {
  constructor() {
    super();
    console.log("Creating MqttText")
  }
  static get observedAttributes() {
    return ['topic',]; }
  message_received(topic, message) {
    console.log("Setting ", topic, " to ", message );
    this.state.value = message;
    this.renderAndReplace();
  }
  shouldLoadWhenConnected() { return !!mqtt_client && !this.state.subscribed; }
  loadContent() {
    console.log("MqttText.loadContent");
    this.state.subscribed = true;
    mqtt_subscribe(this.state.topic, this.message_received.bind(this));
  }
  render() {
    console.log("Rendering MqttText");
    return [
      EL('div', {},[
        EL('span',{class: 'demo', textContent: this.state.topic + ": "}),
        EL('span',{class: 'demo', textContent: this.state.value || ""}),
      ])
    ]
  }
}
customElements.define('mqtt-text', MqttText);
