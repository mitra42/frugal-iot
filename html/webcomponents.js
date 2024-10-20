/*
 * Simple MQTT template for FrugalIoT
 */
import {EL, HTMLElementExtended, getUrl, toBool} from './node_modules/html-element-extended/htmlelementextended.js';
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
      // See https://stackoverflow.com/questions/69709461/mqtt-websocket-connection-failed

      this.setStatus("Connecting");
      mqtt_client = mqtt.connect(this.state.server, {
        connectTimeout: 5000,
        username: "public", //TODO-30 parameterize this
        password: "public", //TODO-30 parameterize this
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
      }.bind(this));
      mqtt_client.on("message", (topic, message) => {
        // message is Buffer
        let msg = message.toString();
        console.log("Received", topic, " ", msg);
        for (let o of mqtt_subscriptions) {
          // console.log("Dispatch testing: ", topic)
          if (o.topic === topic) {
            // console.log("Dispatching: ", topic, msg)
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
  static get observedAttributes() { return ['topic','value']; }
  // TODO - think this could be super() &&  !this.state.subscribed;
  //shouldLoadWhenConnected() { return !!mqtt_client && !this.state.subscribed; }
  shouldLoadWhenConnected() { return super.shouldLoadWhenConnected() && !this.state.subscribed; }
  loadContent() {
    this.state.subscribed = true;
    mqtt_subscribe(this.state.topic, this.message_received.bind(this));
  }
  valueSet(val) {
    this.state.value = val;
  } // Intended to be subclassed

  message_received(topic, message) {
    // console.log("Setting ", topic, " to ", message );
    this.valueSet(message);
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
  // TODO - make sure this doesn't get triggered by a message from server.
  valueGet() { // Needs to return an integer or a string
    return this.state.value
  } // Overridden for booleans
  onChange(e) {
    //console.log("Changed"+e.target.checked);
    this.state.value = e.target.checked;
    // super.onChange(e);
    mqtt_client.publish(this.state.topic, this.valueGet(), { retain: this.state.retain, qos: this.state.qos});
  }
}

class MqttToggle extends MqttTransmitter {
  valueSet(val) {
    super.valueSet(toBool(val));
    this.state.indeterminate = false;
  }
  valueGet() {
    return (+this.state.value).toString(); // Implicit conversion from bool to int then to String.
  }
  static get observedAttributes() {
    return MqttTransmitter.observedAttributes.concat(['checked','indeterminate']);
  }
  // TODO - make sure this doesn't get triggered by a message from server.
  onChange(e) {
    //console.log("Changed"+e.target.checked);
    this.state.value = e.target.checked; // Boolean
    super.onChange(e);
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

const MBstyle = `
.outer {border: 2px,black,solid; background-color: white;margin:5px;}
 .left {display:inline-block; text-align: right;}
 .right {background-color:white; display:inline-block;}
 .val {margin:5px;}
 `;
class MqttBar extends MqttTransmitter {
  // TODO - make sure this doesn't get triggered by a message from server.
  static get observedAttributes() { return MqttTransmitter.observedAttributes.concat(['value','min','max','color']); }
  static get floatAttributes() { return MqttTransmitter.floatAttributes.concat(['value','min','max']); }

  valueSet(val) {
    super.valueSet(Number(val));
  }
  onChange(e) {
    super.onChange(e);
  }

  render() {
    //this.state.changeable.addEventListener('change', this.onChange.bind(this));
    let width = 100*(this.state.value-this.state.min)/(this.state.max-this.state.min);
    return [
      EL('style', {textContent: MBstyle}), // Using styles defined above
      EL('div', {class: "outer",},[
        EL('span', {class: "left", style: `width:${width}%; background-color:${this.state.color};`},[
          EL('span', {class: "val", textContent: this.state.value}),
        ]),
        EL('span', {class: "right", style: "width:"+(100-width)+"%"}),
//        EL('span',{class: 'demo', textContent: this.state.value || ""}),
      ]),
    ];
  }
}
customElements.define('mqtt-bar', MqttBar);

