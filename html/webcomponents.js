/*
 * Simple MQTT template for FrugalIoT
 */
import {EL, HTMLElementExtended, getUrl} from './node_modules/html-element-extended/htmlelementextended.js';
import mqtt from './node_modules/mqtt/dist/mqtt.esm.js'; // https://www.npmjs.com/package/mqtt

var mqtt_client;
var mqtt_subscriptions = [];
let unique_id = 1; // Just used as a label for auto-generated elements

function mqtt_subscribe(topic, cb) {
  console.log("Subscribing to ", topic);
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
      // console.log("XXX already started connection") // We expect this, probably one time
    }
  }
  publish(topic, msg, options) {
    console.log("Publishing", topic, " ", msg);
    mqtt_client.publish(topic, msg, options);
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

class MqttReceiver extends MqttElement {
  // constructor() { super(); }
  static get observedAttributes() { return ['topic',]; }
  // TODO - think this could be super() &&  !this.state.subscribed;
  //shouldLoadWhenConnected() { return !!mqtt_client && !this.state.subscribed; }
  shouldLoadWhenConnected() { return super.shouldLoadWhenConnected() && !this.state.subscribed; }
  loadContent() {
    this.state.subscribed = true;
    mqtt_subscribe(this.state.topic, this.message_received.bind(this));
  }
  valueSet() { } // Intended to be subclassed

  message_received(topic, message) {
    console.log("Setting ", topic, " to ", message );
    this.state.value = message;
    this.valueSet();
    this.renderAndReplace();
  }
}
class MqttText extends MqttReceiver {
  // constructor() { super(); }
  render() {
    return [
      EL('div', {},[
        EL('span',{class: 'demo', textContent: this.state.topic + ": "}),
        EL('span',{class: 'demo', textContent: this.state.value || ""}),
      ])
    ]
  }
}
customElements.define('mqtt-text', MqttText);

class MqttTransmitter extends MqttReceiver {
  static get observedAttributes() { return MqttReceiver.observedAttributes.concat(['retain', 'qos']); }
  static get integerAttributes() { return MqttReceiver.integerAttributes.concat(['retain', 'qos']) };
}

// TODO this will probably split to insert a MqttTransmitter as subclass of MqttReceiver
class MqttToggle extends MqttTransmitter {
  constructor() {
    super();
  }
  valueSet() {
    this.state.indeterminate = false;
  }
  static get observedAttributes() {
    return MqttTransmitter.observedAttributes.concat(['value']);
  }
  onChange(e) {
    //console.log("Changed"+e.target.checked);
    this.state.value = e.target.checked;
    mqtt_client.publish(this.state.topic, this.state.value, { retain: this.state.retain, qos: this.state.qos});
  }

  render() {
    //this.state.changeable.addEventListener('change', this.onChange.bind(this));
    return [
      EL('div', {},[
        EL('input', {type: 'checkbox', id: 'checkbox'+ (++unique_id) ,
          checked: !!this.state.value, indeterminate: typeof(this.state.value) == "undefined",
          onchange: this.onChange.bind(this)}),
        // TODO check how can test whether value is set and set indeterminate if not set (but not if false)
        EL('label', {for: 'checkbox'+unique_id }, [
           EL('slot', {}),
        ]),
      ]),
    ];
  }
}
customElements.define('mqtt-toggle', MqttToggle);
