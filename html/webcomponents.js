/*
 * Simple MQTT template for FrugalIoT
 */
import {EL, HTMLElementExtended, getUrl, toBool} from './node_modules/html-element-extended/htmlelementextended.js';
import mqtt from './node_modules/mqtt/dist/mqtt.esm.js'; // https://www.npmjs.com/package/mqtt
import yaml from './node_modules/js-yaml/dist/js-yaml.mjs'; // https://www.npmjs.com/package/js-yaml

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
      // TODO need to check for disconnect and set status accordingly
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
        // TODO simple message sent by local client, index.html can watch for it to know the local route is working
        this.setStatus("Testing presence");
        mqtt_client.publish("presence", "Hello mqtt");
       */
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
  // TODO display some more about the client and its status.
  render() {
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
  static get observedAttributes() { return ['topic','value','name']; }
  // TODO - think this could be super() &&  !this.state.subscribed;
  //shouldLoadWhenConnected() { return !!mqtt_client && !this.state.subscribed; }
  shouldLoadWhenConnected() { return super.shouldLoadWhenConnected() && !this.state.subscribed; }
  loadContent() {
    this.state.subscribed = true;
    mqtt_subscribe(this.state.topic, this.message_received.bind(this));
  }
  valueSet(val) {
    this.state.value = val;
    return true; // Rerender
  } // Intended to be subclassed

  message_received(topic, message) {
    // console.log("Setting ", topic, " to ", message );
    if (this.valueSet(message)) {
      this.renderAndReplace();
    }
  }
  findProject() {
    return this.closest("mqtt-project");
  }
  findNode() {
    return this.closest("mqtt-node");
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
  publish() {
    // super.onChange(e);
    console.log("Publishing ", this.state.topic, this.valueGet(), this.state.retain ? "retain": "", " qos=", this.state.qos );
    mqtt_client.publish(this.state.topic, this.valueGet(), { retain: this.state.retain, qos: this.state.qos});
  }
}

class MqttToggle extends MqttTransmitter {
  valueSet(val) {
    super.valueSet(toBool(val));
    this.state.indeterminate = false;
    return true; // Rerender
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
    this.publish(e);
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
.outer {background-color: white;margin:5px; padding:5px;}
.bar {border: 1px,black,solid; background-color: white;margin: 0px;}
 .left {display:inline-block; text-align: right;}
 .right {background-color:white; display:inline-block;}
 .val {margin:5px;}
 `;
class MqttBar extends MqttReceiver {
  static get observedAttributes() { return MqttTransmitter.observedAttributes.concat(['value','min','max','color']); }
  static get floatAttributes() { return MqttTransmitter.floatAttributes.concat(['value','min','max']); }

  constructor() {
    super();
  }
  valueSet(val) {
    super.valueSet(Number(val));
    return true; // Note shouldn't re-render children like a MqttSlider.
  }
  render() {
    //this.state.changeable.addEventListener('change', this.onChange.bind(this));
    let width = 100*(this.state.value-this.state.min)/(this.state.max-this.state.min);
    let setpointwidth = 100*(this.state.setpoint-this.state.min)/(this.state.max-this.state.min);

    return [
      EL('style', {textContent: MBstyle}), // Using styles defined above
      EL('div', {class: "outer"}, [
        EL('div', {class: "name"}, [
          EL('span', {textContent: this.state.name}),
        ]),
        EL('div', {class: "bar",},[
          EL('span', {class: "left", style: `width:${width}%; background-color:${this.state.color};`},[
            EL('span', {class: "val", textContent: this.state.value}),
          ]),
          EL('span', {class: "right", style: "width:"+(100-width)+"%"}),
        ]),
        EL('slot',{}),
      ]),
    ];
  }
}
customElements.define('mqtt-bar', MqttBar);


const MSstyle = `
.outer {background-color: white;margin:5px; padding:5px;}
.pointbar {margin:0px; padding 0px;}
.val {margin:5px;}
.setpoint {
    position: relative;
    top: -5px;
    cursor: pointer;
    width: max-content;
    height: max-content;
  }
 `;

// TODO Add some way to do numeric display, numbers should change on mousemoves.
class MqttSlider extends MqttTransmitter {
  static get observedAttributes() { return MqttTransmitter.observedAttributes.concat(['value','min','max','color','setpoint']); }
  static get floatAttributes() { return MqttTransmitter.floatAttributes.concat(['value','min','max', 'setpoint']); }
  static get boolAttributes() { return MqttTransmitter.boolAttributes.concat(['continuous'])}

  constructor() {
    super();
  }
  valueSet(val) {
    super.valueSet(Number(val));
    this.thumb.style.left = this.leftOffset() + "px";
    return true; // Rerenders on moving based on any received value but not when dragged
  }
  valueGet() {
    return (this.state.value).toPrecision(3); // Conversion from int to String (for MQTT)
  }
  leftToValue(l) {
    return (l+this.thumb.offsetWidth/2)/this.slider.offsetWidth * (this.state.max-this.state.min) + this.state.min;
  }
  leftOffset() {
    return ((this.state.value-this.state.min)/(this.state.max-this.state.min)) * (this.slider.offsetWidth) - this.thumb.offsetWidth/2;
  }
  onmousedown(event) {
    event.preventDefault();
    let shiftX = event.clientX - this.thumb.getBoundingClientRect().left; // Pixels of mouse click from left
    let thumb = this.thumb;
    let slider = this.slider;
    let tt = this;
    let lastvalue = this.state.value;
    document.addEventListener('mousemove', onMouseMove);
    document.addEventListener('mouseup', onMouseUp);
    function onMouseMove(event) {
      let newLeft = event.clientX - shiftX - slider.getBoundingClientRect().left;
      // if the pointer is out of slider => lock the thumb within the bounaries
      newLeft = Math.min(Math.max( -thumb.offsetWidth/2, newLeft,), slider.offsetWidth - thumb.offsetWidth/2);
      tt.valueSet(tt.leftToValue(newLeft));
      if (tt.state.continuous && (tt.state.value != lastvalue)) { tt.publish(); lastvalue = tt.state.value; }
    }
    function onMouseUp(event) {
      tt.publish();
      document.removeEventListener('mouseup', onMouseUp);
      document.removeEventListener('mousemove', onMouseMove);
    }
    // shiftY not needed, the thumb moves only horizontally
  }
  renderAndReplace() {
    super.renderAndReplace();
    if (this.thumb) {
      this.thumb.style.left = this.leftOffset() + "px";
    }
  }
  render() {
    if ((!this.slider) && (this.children.length > 0)) {
      // Build once as don't want rerendered - but dont render till after children added (end of EL)
      this.thumb = EL('div', {class: "setpoint"}, this.children);
      this.slider = EL('div', {class: "pointbar",},[this.thumb]);
      this.slider.onmousedown = this.onmousedown.bind(this);
    }
    return [
      EL('style', {textContent: MSstyle}), // Using styles defined above
      EL('div', {class: "outer"}, [
        EL('div', {class: "name"}, [
          EL('span', {textContent: this.state.name}),
          EL('span', {class: "val", textContent: this.state.value}), // TODO restrict number of DP
        ]),
        this.slider,  // <div.setpoint><child></div
      ])
    ];
  }
}
customElements.define('mqtt-slider', MqttSlider);

class MqttDropdown extends MqttTransmitter {
  static get observedAttributes() { return MqttTransmitter.observedAttributes.concat(['type','options','value']); }

  constructor() {
    super();
  }
  findTopics() {
    // TODO-42 can collapse this once working
    let project = this.findProject();
    let nodes = project.children;
    let topics = nodes.map(n => { n.value.topics.filter ....

  }
  valueSet(val) {
    super.valueSet(val);
    // TODO-43 need to set which of dropdown are selected
    return true; // Rerenders on moving based on any received value but not when dragged
  }
  // super.valueGet fine as its text
}
customElements.define('mqtt-dropdown', MqttDropdown);

const MPstyle = `
.outer {border: 1px,black,solid;  margin: 0.2em; }
.projectname, .name { margin-left: 1em; margin-right: 1em; } 
`;

class MqttProject extends MqttReceiver {
  constructor() {
    super();
    this.state.nodes = [];
  }
  valueSet(val) {
    if (!this.state.nodes.includes(val)) {
      this.state.nodes.push(val);
      let id = val;
      let topic = this.state.topic + val;
      this.append(EL('mqtt-node', {id, topic},[]));
    }
  }
  render() {
    return [
      EL('style', {textContent: MPstyle}), // Using styles defined above
      EL('div', {class: "outer"}, [
        EL('div', {class: "title"},[
          EL('span',{class: 'projectname', textContent: this.state.topic}),
          EL('span',{class: 'name', textContent: this.state.name}),
        ]),
        EL('div', {class: "nodes"},[
          EL('slot', {}),
        ]),
      ])
    ];
  }
}
customElements.define('mqtt-project', MqttProject);

const MNstyle = `
.outer { border: 1px,black,solid;  margin: 0.2em; }
.name, .description, .nodeid { margin-left: 1em; margin-right: 1em; } 
`;

class MqttNode extends MqttReceiver {
  static get observedAttributes() { return MqttReceiver.observedAttributes.concat(['id']); }
  constructor() {
    super(); // Will subscribe to topic
    this.state.topics = {};
  }
  elementFrom(t) {
    let topic = this.state.topic + "/" + t.topic;
    let name = t.name;
    let el;
    if (t.display === "toggle") {
      // Assuming rw: rw, type: bool
      el = EL('mqtt-toggle', {topic, name, retain: 1, qos: 1},[name]);
    } else if (t.display === "bar") {
      // Assuming rw: r, type: float
      el = EL('mqtt-bar', {topic, name, max: t.max, min: t.min, color: t.color},[]);
    } else if (t.display === "text") {
      el = EL('mqtt-text', {name, topic},[]);
    } else if (t.display === "slider") {
      el = EL('mqtt-slider', {name, topic, min: t.min, max: t.max, value: (t.max + t.min)/2 }, [
          EL('span', { textContent: "△"}, []),
      ]);
    } else if (t.display === "dropdown") {
      el = EL('mqtt-dropdown', {name, topic, type: t.type, options: t.options}, []);
    } else {
      console.log("do not know how to display a ", t.display);
      //TODO add slider (MqttSlider), need to specify which other element attached to.
    }
    if (el) {
      this.append(el);
      this.state.topics[t.topic] = el; // Index into elements - e.g. used by MqttDropdown
    }
  }
  valueSet(val) {
    let obj = yaml.loadAll(val,{ onWarning: (warn) => console.log('Yaml warning:', warn) });
    console.log(obj);
    let node = obj[0]; // Should only ever be one of them
    this.state.value = node; // Save the object for this node
    ['id','description','name'].forEach(k => this.state[k] = node[k]);
    while (this.childNodes.length > 0) this.childNodes[0].remove(); // Remove and replace
    node.topics.forEach(t => { this.elementFrom(t); });
    return true;
  }
  /*
  shouldLoadWhenConnected() {
  // For now relying on retaintion of advertisement by broker
    return this.state.id && super.shouldLoadWhenConnected() ;
  }
 */
  render() {
    return [
      EL('style', {textContent: MNstyle}), // Using styles defined above
      EL('div', {class: "outer"}, [
        EL('div', {class: "title"},[
          EL('div', {},[
            EL('span',{class: 'name', textContent: this.state.name}),
            EL('span',{class: 'nodeid', textContent: this.state.id}),
            ]),
          EL('span',{class: 'description', textContent: this.state.description}),
        ]),
        EL('div', {class: "nodes"},[
          EL('slot', {}),
        ]),
      ])
    ]
  }
}
customElements.define('mqtt-node', MqttNode);
