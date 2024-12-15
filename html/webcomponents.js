/*
 * Simple MQTT template for FrugalIoT
 *
 */
// noinspection ES6PreferShortImport
import {EL, HTMLElementExtended, toBool, GET} from './node_modules/html-element-extended/htmlelementextended.js';
import mqtt from './node_modules/mqtt/dist/mqtt.esm.js'; // https://www.npmjs.com/package/mqtt
import yaml from './node_modules/js-yaml/dist/js-yaml.mjs'; // https://www.npmjs.com/package/js-yaml
//Fails - reported as https://github.com/caolan/async/issues/2010
//import { each } from './node_modules/async-es/index.js'; // https://caolan.github.io/async/v3/docs.html
import async from './node_modules/async/dist/async.mjs'; // https://caolan.github.io/async/v3/docs.html
import { parse } from "csv-parse"; // https://csv.js.org/parse/distributions/browser_esm/
import { Chart, registerables, _adapters } from './node_modules/chart.js/dist/chart.js'; // "https://www.chartjs.org"
//import 'chartjs-adapter-luxon';
Chart.register(...registerables); //TODO figure out how to only import that chart types needed
/* This is copied from the chartjs-adapter-luxon, I could not get it to import - gave me an error every time */
/*!
 * chartjs-adapter-luxon v1.3.1
 * https://www.chartjs.org
 * (c) 2023 chartjs-adapter-luxon Contributors
 * Released under the MIT license
 */
import { DateTime } from 'luxon';

const FORMATS = {
  datetime: DateTime.DATETIME_MED_WITH_SECONDS,
  millisecond: 'h:mm:ss.SSS a',
  second: DateTime.TIME_WITH_SECONDS,
  minute: DateTime.TIME_SIMPLE,
  hour: {hour: 'numeric'},
  day: {day: 'numeric', month: 'short'},
  week: 'DD',
  month: {month: 'short', year: 'numeric'},
  quarter: "'Q'q - yyyy",
  year: {year: 'numeric'}
};

// noinspection JSCheckFunctionSignatures
_adapters._date.override({
  _id: 'luxon', // DEBUG

  /**
   * @private
   */
  _create: function(time) {
    return DateTime.fromMillis(time, this.options);
  },

  init(chartOptions) {
    if (!this.options.locale) {
      this.options.locale = chartOptions.locale;
    }
  },

  formats: function() {
    return FORMATS;
  },

  parse: function(value, format) {
    const options = this.options;

    const type = typeof value;
    if (value === null || type === 'undefined') {
      return null;
    }

    if (type === 'number') {
      value = this._create(value);
    } else if (type === 'string') {
      if (typeof format === 'string') {
        value = DateTime.fromFormat(value, format, options);
      } else {
        value = DateTime.fromISO(value, options);
      }
    } else if (value instanceof Date) {
      value = DateTime.fromJSDate(value, options);
    } else if (type === 'object' && !(value instanceof DateTime)) {
      value = DateTime.fromObject(value, options);
    }

    return value.isValid ? value.valueOf() : null;
  },

  format: function(time, format) {
    const datetime = this._create(time);
    return typeof format === 'string'
      ? datetime.toFormat(format)
      : datetime.toLocaleString(format);
  },

  add: function(time, amount, unit) {
    const args = {};
    args[unit] = amount;
    return this._create(time).plus(args).valueOf();
  },

  diff: function(max, min, unit) {
    return this._create(max).diff(this._create(min)).as(unit).valueOf();
  },

  startOf: function(time, unit, weekday) {
    if (unit === 'isoWeek') {
      weekday = Math.trunc(Math.min(Math.max(0, weekday), 6));
      const dateTime = this._create(time);
      return dateTime.minus({days: (dateTime.weekday - weekday + 7) % 7}).startOf('day').valueOf();
    }
    return unit ? this._create(time).startOf(unit).valueOf() : time;
  },

  endOf: function(time, unit) {
    // noinspection JSCheckFunctionSignatures
    return this._create(time).endOf(unit).valueOf();
  }
});
/* End of code copied from chartjs-adapter-luxon.esm.js */

// TODO mqtt_client should be inside the MqttClient class
let mqtt_client; // MQTT client - talking to server
// TODO mqtt_subscriptions should be inside the MqttClient class
let mqtt_subscriptions = [];   // [{topic, cb(message)}]
let unique_id = 1; // Just used as a label for auto-generated elements
let graph;  // Will hold a MqttGraph once user chooses to graph anything
let server_config;

/* Helpers of various kinds */

// Subscribe to a topic (no wild cards as topic not passed to cb),
function mqtt_subscribe(topic, cb) { // cb(message)
  console.log("Subscribing to ", topic);
  mqtt_subscriptions.push({topic, cb});
  mqtt_client.subscribe(topic, (err) => { if (err) console.error(err); })
}

/* MQTT support */
class MqttClient extends HTMLElementExtended {
  // This appears to be reconnecting properly, but if not see mqtt (library I think)'s README
  static get observedAttributes() { return ['server']; }

  setStatus(text) {
    this.state.status = text;
    this.renderAndReplace();
    // TODO Could maybe just sent textContent of a <span> sitting in a slot ?
  }
  shouldLoadWhenConnected() { return !!this.state.server; } /* Only load when has a server specified */

  loadContent() {
    //console.log("loadContent", this.state.server);
    if (!mqtt_client) {
      // See https://stackoverflow.com/questions/69709461/mqtt-websocket-connection-failed
      this.setStatus("connecting");
      mqtt_client = mqtt.connect(this.state.server, {
        connectTimeout: 5000,
        username: "public", //TODO-30 parameterize this - read config.json then use password from there
        password: "public",
        // Remainder dont appear to be needed
        //hostname: "127.0.0.1",
        //port: 9012, // Has to be configured in mosquitto configuration
        //path: "/mqtt",
      });
      for (let k of ['connect','disconnect','reconnect','close','offline','end']) {
        mqtt_client.on(k, () => {
          this.setStatus(k);
        });
      }
      mqtt_client.on('error', function (error) {
        console.log(error);
        this.setStatus("Error:" + error.message);
      }.bind(this));
      mqtt_client.on("message", (topic, message) => {
        // message is Buffer
        let msg = message.toString();
        console.log("Received", topic, " ", msg);
        for (let o of mqtt_subscriptions) {
          if (o.topic === topic) {
            o.cb(msg);
          }
        }
        //mqtt_client.end();
      });
    } else {
      // console.log("XXX already started connection") // We expect this, probably one time
    }
  }
  // TODO-86 display some more about the client and its status, but probably under an "i"nfo button on Org
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
  // shouldLoadWhenConnected() { return !!mqtt_client; } // TODO check that don't subscribe before connected.
}

class MqttReceiver extends MqttElement {
  static get observedAttributes() { return ['value','color']; }
  static get boolAttributes() { return []; }

  valueSet(val) {
    // Note val can be of many types - it will be subclass dependent
    this.state.value = val;
    return true; // Rerender by default - subclass will often return false
  }

  get project() { // Note this will only work once the element is connected
    // noinspection CssInvalidHtmlTagReference
    return this.closest("mqtt-project");
  }
  /* Unused
  get node() { // Note this will only work once the element is connected.
    // noinspection CssInvalidHtmlTagReference
    return this.closest("mqtt-node");
  }
   */
// Event gets called when graph icon is clicked - asks topic to add a line to the graph
  opengraph(e) {
    this.mt.createGraph();
  }
}

class MqttText extends MqttReceiver {
  // constructor() { super(); }
  render() {
    return [
      EL('div', {},[
        EL('span',{class: 'demo', textContent: this.mt.topic + ": "}),
        EL('span',{class: 'demo', textContent: this.state.value || ""}),
      ])
    ]
  }
}
customElements.define('mqtt-text', MqttText);

class MqttTransmitter extends MqttReceiver {
  // TODO - make sure this doesn't get triggered by a message from server.
  valueGet() { // Needs to return an integer or a string
    return this.state.value
    // TODO could probably use a switch in MqttNode rather than overriding in each subclass
  } // Overridden for booleans

  publish() {
    this.mt.publish(this.valueGet());
  }
}

class MqttToggle extends MqttTransmitter {
  valueSet(val) {
    super.valueSet(val);
    this.state.indeterminate = false; // Checkbox should default to indeterminate till get a message
    return true; // Rerender // TODO could set values on input instead of rerendering
  }
  valueGet() {
    // TODO use Mqtt to convert instead of subclassing
    return (+this.state.value).toString(); // Implicit conversion from bool to int then to String.
  }
  static get observedAttributes() {
    return MqttTransmitter.observedAttributes.concat(['checked','indeterminate']);
  }
  // TODO - make sure this doesn't get triggered by a message from server.
  onChange(e) {
    //console.log("Changed"+e.target.checked);
    this.state.value = e.target.checked; // Boolean
    this.publish();
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

class MqttBar extends MqttReceiver {
  static get observedAttributes() { return MqttReceiver.observedAttributes.concat(['min','max']); }
  static get floatAttributes() { return MqttReceiver.floatAttributes.concat(['value','min','max']); }

  constructor() {
    super();
  }
  // noinspection JSCheckFunctionSignatures
  valueSet(val) {
    super.valueSet(val); // TODO could get smarter about setting with in span rather than rerender
    return true; // Note will not re-render children like a MqttSlider because these are inserted into DOM via a "slot"
  }
  render() {
    //this.state.changeable.addEventListener('change', this.onChange.bind(this));
    let width = 100*(this.state.value-this.state.min)/(this.state.max-this.state.min);
    return !this.isConnected ? null : [
      EL('link', {rel: 'stylesheet', href: '/frugaliot.css'}),
      EL('div', {class: "outer"}, [
        EL('div', {class: "name"}, [ // TODO-30 maybe should use a <label>
          EL('span', {textContent: this.mt.name}),
          EL('img', {class: "icon", src: 'images/icon_graph.svg', onclick: this.opengraph.bind(this)}),
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
  static get observedAttributes() { return MqttTransmitter.observedAttributes.concat(['min','max','color','setpoint','continuous']); }
  static get floatAttributes() { return MqttTransmitter.floatAttributes.concat(['value','min','max', 'setpoint']); }
  static get boolAttributes() { return MqttTransmitter.boolAttributes.concat(['continuous'])}

  // noinspection JSCheckFunctionSignatures
  valueSet(val) {
    super.valueSet(val);
    this.thumb.style.left = this.leftOffset() + "px";
    return true; // Rerenders on moving based on any received value but not when dragged
    // TODO could get smarter about setting with rather than rerendering
  }
  valueGet() {
    // TODO use mqttTopic for conversion instead of subclassing
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
      // if the pointer is out of slider => lock the thumb within the boundaries
      newLeft = Math.min(Math.max( -thumb.offsetWidth/2, newLeft,), slider.offsetWidth - thumb.offsetWidth/2);
      tt.valueSet(tt.leftToValue(newLeft));
      // noinspection JSUnresolvedReference
      if (tt.state.continuous && (tt.state.value !== lastvalue)) { tt.publish(); lastvalue = tt.state.value; }
    }
    // noinspection JSUnusedLocalSymbols
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
      // Build once as don't want re-rendered - but do not render till after children added (end of EL)
      this.thumb = EL('div', {class: "setpoint"}, this.children);
      this.slider = EL('div', {class: "pointbar",},[this.thumb]);
      this.slider.onmousedown = this.onmousedown.bind(this);
    }
    return !this.isConnected ? null : [
      EL('style', {textContent: MSstyle}), // Using styles defined above
      EL('div', {class: "outer"}, [
        EL('div', {class: "name"}, [ //TODO maybe use a label
          EL('span', {textContent: this.mt.name}),
          EL('span', {class: "val", textContent: this.state.value}), // TODO restrict number of DP
        ]),
        this.slider,  // <div.setpoint><child></div
      ])
    ];
  }
}
customElements.define('mqtt-slider', MqttSlider);

const MDDstyle = `
.outer { border: 0px,black,solid;  margin: 0.2em; }
.name, .description, .nodeid { margin-left: 1em; margin-right: 1em; } 
`;

// TODO-43 rename as MqttChooseTopic
class MqttDropdown extends MqttTransmitter {
  // options = "bool" for boolean topics (matches t.type on others)
  static get observedAttributes() { return MqttTransmitter.observedAttributes.concat(['options','project']); }

  // TODO-43 may need to change findTopics to account for other selection criteria
  findTopics() {
    let project = this.state.project;
    let nodes = Array.from(project.children);
    // Note the nodes value is its config
    return nodes.map(n => n.topicsByType(this.state.options)).flat();
  }
  // noinspection JSCheckFunctionSignatures
  valueSet(val) {
    super.valueSet(val);
    // TODO get smarter about setting "selected" instead of rerendering
    return true; // Rerenders on moving based on any received value to change selected topic
  }
  onchange(e) {
    //console.log("Dropdown onchange");
    this.state.value = e.target.value; // Want the value
    this.publish();
  }

  render() {
    return !this.isConnected ? null : [
      EL('style', {textContent: MDDstyle}), // Using styles defined above
      EL('div', {class: 'outer'}, [
        EL('label', {for: this.mt.topic, textContent: this.mt.name}),
        EL('select', {id: this.mt.topic, onchange: this.onchange.bind(this)}, [
          EL('option', {value: "", textContent: "Unused", selected: !this.state.value}),
          this.findTopics().map( t => // { name, type etc. }
            EL('option', {value: t.topic, textContent: t.name, selected: t.topic === this.state.value}),
          ),
        ]),
      ]),
    ];
  }
  // super.valueGet fine as its text
}
customElements.define('mqtt-dropdown', MqttDropdown);

// TODO merge all the styles into a stylesheet and load that and reference in each class

// Outer element of the client - Top Level logic
// If specifies org / project / node then believe it and build to that
// otherwise get config from server
// Add appropriate internals

// Functions on the configuration object returned during discovery
function oHasProject(o, project) {
  // noinspection JSUnresolvedReference
  return o.projects.some(p => p.name === project);
}
function pHasNode(p, nodename) {
  return p.nodes.some(n => n.name === nodename);
}
function o2ProjectWithNode(o, nodename) {
  // noinspection JSUnresolvedReference
  return o.projects.find(p => pHasNode(p, nodename)); // returns a project
}
function nodeName2OrgProject(nodename) {
  // noinspection JSUnresolvedReference
  for ( let o in server_config.organizations) {
    let p;
    // noinspection JSAssignmentUsedAsCondition
    if ( p = o2ProjectWithNode(nodename)) {
      return [o.name, p.name];
    }
  }
  return [null, null];
}
class MqttWrapper extends HTMLElementExtended {
  static get observedAttributes() { return MqttReceiver.observedAttributes.concat(['organization','project','node']); }
  // Maybe add 'discover' but think thru interactions
  //static get boolAttributes() { return MqttReceiver.boolAttributes.concat(['discover'])}

  // Note this is not using the standard connectedCallBack which loads content and re-renders,
  // it is going to instead add things to the slot

  onOrganization(e) {
    this.state.organization = e.target.value;
    this.appender();
  }
  onProject(e) {
    this.state.project = e.target.value;
    this.appender();
  }
  appendClient() {
    // TODO-security at some point we'll need one client per org and to use username and password from config.yaml which by then should be in config.d
    // TODO-security but that should be trivial if only ever display one org
    // noinspection JSUnresolvedReference
    this.append(
      EL('mqtt-client', {slot: 'client', server: server_config.mqtt.broker}) // typically "ws://naturalinnovation.org:9012"
    )
  }
  addProject(discover) {
    let topic = `${this.state.organization}/${this.state.project}/`;
    let elProject = EL('mqtt-project', {discover, name: `${this.state.project}`}, []);
    let mt = new MqttTopic();
    mt.type = "text";
    mt.topic = topic;
    mt.element = elProject;
    elProject.mt = mt;
    mt.subscribe();
    this.append(elProject);
    return elProject;
  }
  appender() {
    // At this point could have any combination of org project or node
    if (this.state.node) { // n
      if (!this.state.organization || !this.state.project) {   // n, !(o,p)
        let [o,p] = nodeName2OrgProject(this.state.node);
        if (!o) {
          console.error("Unable to find node=", this.state.node);
          // TODO-69 display error to user, not just console
          return;
        } else {
          this.state.organization = o;
          this.state.project = p;
        }
      } // Drop through with o & p
      this.addProject(false);
      elProject.valueSet(this.state.node, true); // Create node on project along with its MqttNode
    } else { // !n
      if (!this.state.project)  { // !n !p ?o
        if (!this.state.organization) { // !n !p !o
          // noinspection JSUnresolvedReference
          this.append(
            EL('div', {class: 'dropdown'}, [
              EL('label', {for: 'organizations', textContent: "Organization"}),
              EL('select', {id: 'organizations', onchange: this.onOrganization.bind(this)}, [
                EL('option', {value: null, textContent: "Not selected", selected: !this.state.value}),
                // noinspection JSUnresolvedReference
                server_config.organizations.map( o =>
                  EL('option', {value: o.name, textContent: o.name, selected: false}),
                ),
              ]),
            ]));
        } else { // !n !p o  // TODO-69 maybe this should be a blank project ?
          // noinspection JSUnresolvedReference
          this.append(
            EL('div', {class: 'dropdown'}, [
              EL('label', {for: 'projects', textContent: "Project"}),
              EL('select', {id: 'projects', onchange: this.onProject.bind(this)}, [
                EL('option', {value: null, textContent: "Not selected", selected: !this.state.value}),
                // noinspection JSUnresolvedReference
                server_config.organizations.find(o => o.name === this.state.organization).projects.map( p =>
                  EL('option', {value: p.name, textContent: p.name, selected: false}),
                ),
              ]),
            ]));
        }
      } else { // !n p ?o
        // noinspection JSUnresolvedReference
        if (!this.state.organization) {
          // noinspection JSUnresolvedReference
          let o = server_config.organizations.find(o => oHasProject(o, this.state.project));
          if (!o) {
            console.error("Unable to find project:", this.state.project);
            // TODO-69 display error to user, not just console
            return;
          } else {
            this.state.organization = o.name;
          }
        } // drop through with !n p o
        // TODO-69 need to have a human-friendly name, and short project id - will be needed in configuration and elsewhere.
        this.addProject(true);
      }
    }
  }
  connectedCallback() {
      // TODO-69 security this will be replaced by a subset of config.yaml,
      //  that is public, but in the same format, so safe to build on this for now
      GET("/config.json", {}, (err, json) => {
        if (err) {
          console.error(err);
          // TODO-69 display error to user, not just console
          return;
        } else { // got config
          server_config = json;
          this.loadAttributesFromURL();
          this.appendClient();
          this.appender();
        }
        this.renderAndReplace(); // TODO check, but shouldnt need to renderAndReplace as render is (currently) fully static
      });
      //super.connectedCallback(); // Not doing as finishes with a re-render.
  }
  render() {
    return [
      EL('link', {rel: 'stylesheet', href: '/frugaliot.css'}),
      EL('div', {class: 'outer'}, [
          EL('slot', {name: 'client'}),
          EL('slot'),
        ]),
    ];

  }
}
customElements.define('mqtt-wrapper', MqttWrapper);

const MPstyle = `
.outer {border: 1px,black,solid;  margin: 0.2em; }
.projectname, .name { margin-left: 1em; margin-right: 1em; } 
`;

class MqttProject extends MqttReceiver {
  constructor() {
    super();
    this.state.nodes = [];  // [ MqttNode ]
  }
  static get observedAttributes() { return MqttReceiver.observedAttributes.concat(['discover']); }
  static get boolAttributes() { return MqttReceiver.boolAttributes.concat(['discover'])}

  // noinspection JSCheckFunctionSignatures
  valueSet(val, force) {  //TODO-REFACTOR maybe dont use "force", (only used by wrapper)
    if (this.state.discover || force) {
      if (!this.state.nodes.includes(val)) {
        this.state.nodes.push(val);
        let id = val;
        let topic = this.mt.topic + val;
        let elNode = EL('mqtt-node', {id, topic, discover: this.state.discover},[]);
        let mt = new MqttTopic();
        mt.type = "yaml";
        mt.topic = topic;
        mt.element = elNode;
        elNode.mt = mt;
        this.append(elNode);
        mt.subscribe(); // Subscribe to get Discovery
     }
    }
    return false; // Should not need to rerender
  }
  render() {
    return  !this.isConnected ? null : [
      EL('style', {textContent: MPstyle}), // Using styles defined above
      EL('div', {class: "outer"}, [
        EL('div', {class: "title"},[
          EL('span',{class: 'projectname', textContent: this.mt.topic}),
          EL('span',{class: 'name', textContent: this.mt.name}),
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


class MqttTopic {
  // Note this intentionally does NOT extend HtmlElement or MqttElement etc
  // Encapsulate a single topic, mostly this will be built during discovery process,
  // but could also be built by hard coded UI if doesn't exist
  // Should be indexed in MqttNode

  constructor() {
    this.data = [];
    this.qos = 0; // Default to send and not care if received
    this.retain = false; // Default to not retain
  }

  fromDiscovery(discoveredTopic, node) {
    // topic, name, type, display, rw, min, max, color, options,
    Object.keys(discoveredTopic).forEach((k) => { this[k] = discoveredTopic[k]; });
    this.topic = node.mt.topic + "/" + discoveredTopic.topic; // Expand the topic
    this.node = node;
  }

  get project() { return this.node.project; }

  // Create the UX element that displays this
  createElement() {
    if (!this.element) {
      let name = this.name;
      let el;
      let topic = this.topic;
      switch (this.display) {
        case 'toggle':
          el = EL('mqtt-toggle', {}, [name]);
          this.retain = true;
          this.qos = 1;
          break;
        case 'bar':
          el = EL('mqtt-bar', {max: this.max, min: this.min, color: this.color}, []);
          break;
        case 'text':
          el = EL('mqtt-text', {}, []);
          break;
        case 'slider':
          el = EL('mqtt-slider', {min: this.min, max: this.max, value: (this.max + this.min) / 2}, [
            EL('span', {textContent: "△"}, []),
          ]);
          break;
        case 'dropdown':
          el = EL('mqtt-dropdown', {options: this.options, project: this.project});
          this.retain = true;
          this.qos = 1; // This message needs to get through to node
          break;
        default:
          console.log("do not know how to display a ", this.display);
      }
      if (el) el.mt = this;
      this.element = el;
    }
    return this.element;
  }

  subscribe() {
    if (!mqtt_client) {
      console.error("Trying to subscribe before connected")
    }
    if (!this.subscribed) {
      this.subscribed = true;
      mqtt_subscribe(this.topic, this.message_received.bind(this));
    }
  }

  // TODO add opposite - return string or int based on argument, then look at valueGet subclassed many places
  valueFromText(message) {
    switch(this.type) {
      case "bool":
        return toBool(message);
      case "float":
        return Number(message)
      case "int":
        return Number(message)
      case "topic":
        return message;
      case "text":
        return message;
      case "yaml":
        return yaml.loadAll(message, {onWarning: (warn) => console.log('Yaml warning:', warn)});
      default:
        console.error(`Unrecognized message type: ${this.type}`);
    }
  }

  message_received(message) {
    let value = this.valueFromText(message);
    this.data.push({value, time: Date.now()}); // Same format as graph dataset expects
    if (this.element) {
      if (this.element.valueSet(value)) {
        this.element.renderAndReplace(); // TODO note gradually replacing need to rerender by smarter valueSet() on different subclasses
      }
    }
    if (this.graphdataset) { // instance of MqttGraphdataset
      this.graphdataset.dataChanged();
    }
  }

  get yaxisid() {
    let scaleNames = Object.keys(graph.state.scales);
    let yaxisid;
    let n = this.name.toLowerCase();
    let t = this.topic.split('/').pop().toLowerCase();
    if (scaleNames.includes(n)) { return n; }
    if (scaleNames.includes(t)) { return t; }
    // noinspection JSAssignmentUsedAsCondition
    if ( yaxisid = scaleNames.find(tt => tt.includes(n) || n.includes(tt)) ) { return yaxisid; }
    // noinspection JSAssignmentUsedAsCondition
    if ( yaxisid = scaleNames.find(tt => tt.includes(n) || n.includes(tt)) ) { return yaxisid; }
    // TODO-46 - need to turn axis on, and position when used.
    // Not found - lets make one - this might get more parameters (e.g. linear vs exponential could be a attribute of Bar ?
    graph.addScale(t, {
      // TODO-46 add color
      type: 'linear',
      display: true,
      title: {
        color: this.color,  // May need to vary so not all e.g. humidity same color
        display: true,
        text: this.name,
      },
      suggestedMin: this.min,
      suggestedMax: this.max,
    });
    return t;
  }

  // Event gets called when graph icon is clicked - adds a line to the graph
  createGraph() {
    let graph = MqttGraph.findGraph(); // Side effect of creating if does not exist
    let yaxisid = this.yaxisid;
    // Figure out which scale to use, or build it

    // Create a graphdataset to put in the chart
    if (!this.graphdataset) {
      this.graphdataset = EL('mqtt-graphdataset', {
        name: this.name, color: this.color,
        // TODO-46 yaxis should depend on type of graph BUT cant use name as that may end up language dependent
        min: this.min, max: this.max, yaxisid: yaxisid,
      });
      this.graphdataset.mt = this;
      this.graphdataset.makeChartDataset(); // Links to data above
    }
    if (!graph.contains(this.graphdataset)) {
      graph.append(this.graphdataset); // calls GDS.loadContent which adds dataset to Graph
    }
  }
  publish(val) {
    // super.onChange(e);
    console.log("Publishing ", this.topic, val, this.retain ? "retain": "", this.qos ? `qos=${this.qos}` : "");
    mqtt_client.publish(this.topic, val, { retain: this.retain, qos: this.qos});
  }
  // TODO would be better if caller updated chart when all complete. Needs Promise.all or similar.
  addDataFrom(filename, first, cb) {
    //TODO this location may change
    let filepath = `/server/data/${this.topic}/${filename}`;
    let self = this;
    fetch(filepath)
      .then(response => response.text())
      .then(csvData => {
        parse(csvData, (err, newdata) => {
          if (err) {
            console.error(err);
          } else {
            console.log("retrieved new records", newdata.length);
            newdata.forEach(r => {
              r[0] = parseInt(r[0]);
              r[1] = parseFloat(r[1]); // TODO-72 need function for this as presuming its float
            });
            // self.state.data and self.parentElement.datasets[x] are same actual data,
            // can't set one to this data as wont affect the other
            // 25k 10 1227
            // 305k 73ms 243seconds
            // 121k 97ms 485secs
            let xxx1 = Date.now();
            let olddata = this.data.splice(0, Infinity);
            for (let dd of newdata) { this.data.push(dd); }
            if (!first) { // If its the first, dont put data back as will already be in newdata
              for (let dd of olddata) {
                this.data.push(dd);
              }
            }
            /*
            // 25k 41 3711
            // 304k 24428 crashed
            this.data.splice(0,first ? Infinity: 0); // TODO-72 dont remove (replace Infinity with 0) for next day
            for (let i = data.length-1; i >=0; i--) {
              this.data.unshift(data[i]);
            }
             */
            if (this.data.length > 1000) {
              this.graphdataset.parentElement.chart.options.animations = false; // Disable animations get slow at scale
            }
            let xxx2 = Date.now();
            console.log("XXX72 splice took", xxx2-xxx1);
            cb();
          }
        })
      })
      .catch(err => {
        console.error(err);
        cb(null); // Dont break caller
      }); // May want to report filename here
  }

}

class MqttNode extends MqttReceiver {
  static get observedAttributes() { return MqttReceiver.observedAttributes.concat(['id', 'discover']); }
  static get boolAttributes() { return MqttReceiver.boolAttributes.concat(['discover'])}
  static get integerAttributes() { return MqttReceiver.integerAttributes.concat(['days'])}

  constructor() {
    super(); // Will subscribe to topic
    this.state.topics = {}; // Index of MqttTopic
    this.state.days = 0;
  }
  topicsByType(type) {
    return this.state.value.topics.filter( t => t.type === type).map(t=> { return({name: t.name, topic: this.mt.topic + "/" + t.topic})});
  }
  // noinspection JSCheckFunctionSignatures
  valueSet(obj) { // Val is object converted from yaml
    if (this.state.discover) { // If do not have "discover" set, then presume have defined what UI we want on this node
      this.state.discover = false; // Only want "discover" once, if change then need to get smart about not redrawing working UI as may be relying on data[]
      console.log(obj); // Useful for debugging to see this
      let nodediscover = obj[0]; // Should only ever be one of them
      this.state.value = nodediscover; // Save the object for this node
      ['id', 'description', 'name'].forEach(k => this.state[k] = nodediscover[k]); // Grab top level properties from discover
      while (this.childNodes.length > 0) this.childNodes[0].remove(); // Remove and replace any existing nodes
      nodediscover.topics.forEach(t => {
        if (!this.state.topics[t.topic]) { // Have we done this already
          let x1 = new MqttTopic();
          x1.fromDiscovery(t, this);
          this.state.topics[t.topic] = x1;
          x1.subscribe();
          this.append(x1.createElement());
        }
      });
      return true; // because change name description etc
    } else {
      return false;
    }
  }
  /*
  shouldLoadWhenConnected() {
  // For now relying on retention of advertisement by broker
    return this.state.id && super.shouldLoadWhenConnected() ;
  }
 */
  render() {
    return !this.isConnected ? null : [
      EL('style', {textContent: MNstyle}), // Using styles defined above
      EL('div', {class: "outer"}, [
        EL('div', {class: "title"},[
          EL('div', {},[
            EL('span',{class: 'name', textContent: this.mt.name}),
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

/* This could be part of MqttBar, but maybe better standalone */
class MqttGraph extends MqttElement {
  constructor() {
    super();
    this.datasets = []; // Child elements will add/remove chartjs datasets here
    this.state.dataFrom = null;
    this.state.yAxisCount = 0; // 0 left, 1 right
    this.state.scales = { // Start with an xAxis and add axis as needed
      xAxis: {
        // display: false,
        type: 'time',
        distribution: 'series',
        axis: 'x',
        adapters: {
          date: {
            // locale: 'en-US', // Comment out to Use systems Locale
          },
        },
      }
    };
  }
  static findGraph() { // TODO-46 probably belongs in MqttReceiver
    if (!graph) {
      graph = EL('mqtt-graph');
      document.body.append(graph);
    }
    return graph;
  }

  // Note - makeChart is really fussy, the canvas must be inside something with a size.
  // For some reason this does not work by adding inside the render - i.e. to the virtual Dom.
  loadContent() {
    this.canvas = EL('canvas');
    this.append(EL('div', {slot: "chart", style: "width: 80vw; height: 60vw; position: relative;"},[this.canvas]));
    this.makeChart();
  }
  shouldLoadWhenConnected() {return true;}
  addScale(id, o) {
    o.grid = { drawOnChartArea: !this.state.yAxisCount } // only want the grid lines for one axis to show u
    o.position = ((this.state.yAxisCount++) % 2) ? 'right' : 'left';
    this.state.scales[id] = o;
  }
  makeChart() {
    if (this.chart) {
      this.chart.destroy();
    }
    this.chart = new Chart(
      this.canvas,
      {
        type: 'line', // Really want it to be a line
        data: {
          datasets: this.datasets,
        },
        options: {
          //zone: "America/Denver", // Comment out to use system time
          responsive: true,
          scales: this.state.scales,
        }
      }
    );
  }
  // noinspection JSUnusedLocalSymbols
  graphnavleft(e) {
    // TODO If not first go back x days
    let first = !this.state.dateFrom; // null or date
    if (first) {
      this.state.dateFrom = new Date();
    } else {
      this.state.dateFrom.setDate(this.state.dateFrom.getDate()-1); // Note this rolls over between months ok
    }
    let filename = this.state.dateFrom.toISOString().substring(0,10) + ".csv";
    console.log("XXX72: adding from", filename);
    this.addDataFrom(filename, first);
  }
  addDataFrom(filename, first) {
    // TODO-72 should change arrow to hourglass until complete then switch back - but has to be at Graph level
    async.each(this.children, ((ds,cb) => {
      if (ds.addDataFrom) {
        ds.addDataFrom(filename, first, cb);
      } else {
        cb();
      }
    }),(err) => {
      console.log("XXX72 done all");
      let xxx2 = Date.now(); // TODO-72 remove when done
      this.chart.update();
      console.log("XXX72 update took", Date.now()-xxx2);
      // TODO-72 turn arrow back from hourglass here.
    } );
    /*
    for (let ds of this.children) {
      if (ds.addDataFrom) {
        ds.addDataFrom(filename, first);
      }
    }
     */
  }

  // Called when data on one of the datasets has changed, can do an update, (makeChart is for more complex changes)
  dataChanged() {
    this.chart.update();
  }
  addDataset(chartdataset) {
    this.datasets.push(chartdataset);
    this.makeChart();
  }
  render() {
    return ( [
      EL('link', {rel: 'stylesheet', href: '/frugaliot.css'}),
        // TODO see https://www.chartjs.org/docs/latest/configuration/responsive.html#important-note div should ONLY contain canvas
      EL("div", {class: 'outer'}, [ // TODO Move style to sheet
        EL('div',{class: 'leftright'}, [
          EL('span', {class: "graphnavleft", textContent: "⬅︎", onclick: this.graphnavleft.bind(this)}),
          EL('slot', {name: "chart"}), // This is <div><canvas></div>
        ]),
        EL('slot', {}), // This is the slot where the GraphDatasets get stored
      ])
    ] );
  }
}
customElements.define('mqtt-graph', MqttGraph);

class MqttGraphDataset extends MqttElement {
  /*
  chartdataset: { data[{value, time}], parsing: { xAixKey: 'time', yAxisKey: 'value' }
  chartEL: MqttGraph
  state: { data[{value, time}], name, color, min, max, yaxisid }
   */

  constructor() {
    super();
    // Do not make chartDataset here, as do not have attributes yet
  }
  // TODO clean up observedAttributes etc as this is not the superclass
  static get observedAttributes() {
    return MqttReceiver.observedAttributes.concat(['color', 'min', 'max', 'yaxisid']); }
  static get integerAttributes() {
    return MqttReceiver.integerAttributes.concat(['min', 'max']) };

  makeChartDataset() {
    // Some other priorities that might be useful are at https://www.chartjs.org/docs/latest/samples/line/segments.html
    if (this.chartdataset) {
      console.error("Trying to create chartdataset twice");
    } else {
      // Fields only defined once - especially data
      this.chartdataset = {
        data: this.mt.data, // Should be pointer to receiver's data set in MqttReceiver.valueSet
        parsing: {
          xAxisKey: 'time',
          yAxisKey: 'value'
        },
      };
    }
    // Things that are changed by attributes
    this.chartdataset.label = this.mt.name;
    this.chartdataset.borderColor = this.state.color; // also sets color of point
    this.chartdataset.yAxisID = this.state.yaxisid;
    // Should override display and position and grid of each axis used
  }
  shouldLoadWhenConnected() {return true;}
  loadContent() { // Happens when connected
    this.chartEl = this.parentElement;
    this.chartEl.addDataset(this.chartdataset);
  }
  // noinspection JSUnusedGlobalSymbols
  dataChanged() { // Called when creating UX adds data.
    this.chartEl.dataChanged();
  }
  // Note this wont update the chart, but the caller will be fetching multiple data files and update all.
  addDataFrom(filename, first, cb) {
    //TODO this location may change
    this.mt.addDataFrom(filename, first, cb);
  }

  render() {
    return !this.isConnected ? null :
      EL('span', { textContent: this.mt.name}); // TODO-46-line should be controls
  }
}
customElements.define('mqtt-graphdataset', MqttGraphDataset);