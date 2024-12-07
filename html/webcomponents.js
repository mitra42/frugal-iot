/*
 * Simple MQTT template for FrugalIoT
 */
// noinspection ES6PreferShortImport
import {EL, HTMLElementExtended, toBool, GET} from './node_modules/html-element-extended/htmlelementextended.js';
import mqtt from './node_modules/mqtt/dist/mqtt.esm.js'; // https://www.npmjs.com/package/mqtt
import yaml from './node_modules/js-yaml/dist/js-yaml.mjs'; // https://www.npmjs.com/package/js-yaml
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

let mqtt_client;
let mqtt_subscriptions = [];
let unique_id = 1; // Just used as a label for auto-generated elements
let graph;
let server_config;

/* Helpers of various kinds */

function mqtt_subscribe(topic, cb) {
  console.log("Subscribing to ", topic);
  mqtt_subscriptions.push({topic, cb});
  mqtt_client.subscribe(topic, (err) => { if (err) console.error(err); })
}
/* - not currently used as graph.js adjust automatically
function date_start_hour(x) {
  let y = x ? new Date(x) : new Date();
  y.setMinutes(0);
  y.setSeconds(0);
  y.setMilliseconds(0);
  return y;
}
*/

/* MQTT support */
class MqttClient extends HTMLElementExtended {
  // This appears to be reconnecting properly, but if not see mqtt (library I think)'s README
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
      this.setStatus("connecting");
      mqtt_client = mqtt.connect(this.state.server, {
        connectTimeout: 5000,
        username: "public", //TODO-30 parameterize this but needs multiple clients first see MqttWrapper.appendClient
        password: "public", //TODO-30 parameterize this but needs multiple clients first see MqttWrapper.appendClient
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
        this.state.status = "Error:" + error.message;
      }.bind(this));
      mqtt_client.on("message", (topic, message) => {
        // message is Buffer
        let msg = message.toString();
        console.log("Received", topic, " ", msg);
        for (let o of mqtt_subscriptions) {
          if (o.topic === topic) {
            o.cb(topic, msg)
          }
        }
        //mqtt_client.end();
      });
    } else {
      // console.log("XXX already started connection") // We expect this, probably one time
    }
  }
  // TODO-86 display some more about the client and its status.
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
  constructor() {
    super();
  }
  static get observedAttributes() { return ['topic', 'value', 'name', 'color']; }
  static get boolAttributes() { return []; }

  shouldLoadWhenConnected() { return super.shouldLoadWhenConnected() && !this.state.subscribed; }
  loadContent() {
    this.state.subscribed = true;
    mqtt_subscribe(this.state.topic, this.message_received.bind(this));
    if (!this.state.data) { this.state.data = []; }
  }
  valueSet(val) { // Note val can be of many types - it will be subclass dependent
    this.state.value = val;
    this.state.data.push({time: Date.now(), value: val}); // Same format as graph dataset expects
    if (this.state.dataset) {
      // noinspection JSValidateTypes
      this.state.dataset.dataChanged(); } // Will typically update graph
    return true; // Rerender
  } // Intended to be subclassed and subclass will often return false

  message_received(topic, message) {
    if (this.valueSet(message)) {
      this.renderAndReplace();
    }
  }

  get project() { // Note this will only work once the element is connected
    // noinspection CssInvalidHtmlTagReference
    return this.closest("mqtt-project");
  }
  /* Unused
  findNode() { // Note this will only work once the element is connected.
    // noinspection CssInvalidHtmlTagReference
    return this.closest("mqtt-node");
  }
   */
  get yaxisid() {
    let scaleNames = Object.keys(graph.state.scales);
    let yaxisid;
    let n = this.state.name.toLowerCase();
    let t = this.state.topic.split('/').pop().toLowerCase();
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
        color: this.state.color,
        display: true,
        text: this.state.name,
      },
      suggestedMin: this.state.min,
      suggestedMax: this.state.max,
    });
    return t;
  }
  // Event gets called when graph icon is clicked - adds a line to the graph
  opengraph(e) {
    console.log("Graph clicked", e, this);
    let graph = MqttGraph.findGraph(); // Side effect of creating if does not exist
    let yaxisid = this.yaxisid;
    // Figure out which scale to use, or build it

    // Create a graphdataset to put in the chart
    if (!this.state.dataset) {
      let ds = EL('mqtt-graphdataset', {
        // No topic since piggybacking off this.
        name: this.state.name, color: this.state.color,
        // TODO-46 yaxis should depend on type of graph BUT cant use name as that may end up language dependent
        min: this.state.min, max: this.state.max, yaxisid: yaxisid,
      });
      this.state.dataset = ds;
      // TODO-46 move next two lines to a method on MqttGraphdataset
      ds.state.data = this.state.data;  // Link, not copy
      ds.chartdataset.data = this.state.data; // Link, not copy
    }
    if (!graph.contains(this.state.dataset)) {
      graph.append(this.state.dataset);
    }
  }

}
// TODO turn more things into getters - check from here down
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
  static get integerAttributes() { return MqttReceiver.integerAttributes.concat(['qos']) };
  static get boolAttributes() { return MqttReceiver.boolAttributes.concat(['retain']) }
  constructor() {
    super();
    this.state.qos= 0; // Default to no QOS
    this.state.retain = false; // default to not retaining
  }

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
  static get observedAttributes() { return MqttReceiver.observedAttributes.concat(['value','min','max']); }
  static get floatAttributes() { return MqttReceiver.floatAttributes.concat(['value','min','max']); }

  constructor() {
    super();
  }
  // noinspection JSCheckFunctionSignatures
  valueSet(val) {
    super.valueSet(Number(val));
    return true; // Note shouldn't re-render children like a MqttSlider because these are inserted into DOM via a "slot"
  }
  render() {
    //this.state.changeable.addEventListener('change', this.onChange.bind(this));
    let width = 100*(this.state.value-this.state.min)/(this.state.max-this.state.min);
    return [
      EL('link', {rel: 'stylesheet', href: '/frugaliot.css'}),
      EL('div', {class: "outer"}, [
        EL('div', {class: "name"}, [ // TODO-30 maybe should use a <label>
          EL('span', {textContent: this.state.name}),
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
  static get observedAttributes() { return MqttTransmitter.observedAttributes.concat(['value','min','max','color','setpoint','continuous']); }
  static get floatAttributes() { return MqttTransmitter.floatAttributes.concat(['value','min','max', 'setpoint']); }
  static get boolAttributes() { return MqttTransmitter.boolAttributes.concat(['continuous'])}

  constructor() {
    super();
  }
  // noinspection JSCheckFunctionSignatures
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

const MDDstyle = `
.outer { border: 0px,black,solid;  margin: 0.2em; }
.name, .description, .nodeid { margin-left: 1em; margin-right: 1em; } 
`;

// TODO-43 rename as MqttTopic
class MqttDropdown extends MqttTransmitter {
  // options = "bool" for boolean topics (matches t.type on others)
  static get observedAttributes() { return MqttTransmitter.observedAttributes.concat(['type','options','value','project']); }

  constructor() {
    super();
    this.state.qos = 1; // Default to making sure it gets through
    this.state.retain = true; // And that node can access result if reconnects.
  }
  // TODO-43 may need to change findTopics to account for other selection criteria
  findTopics() {
    let project = this.state.project;
    let nodes = Array.from(project.children);
    return nodes.map(n => n.state.value.topics.filter( t => t.type === this.state.options).map(t=> { return({name: t.name, topic: n.state.topic + "/" + t.topic})}) ).flat();
  }
  // noinspection JSCheckFunctionSignatures
  valueSet(val) {
    super.valueSet(val);
    return true; // Rerenders on moving based on any received value to change selected topic
  }
  onchange(e) {
    //console.log("Dropdown onchange");
    this.state.value = e.target.value; // Want the value
    this.publish();
  }

  render() {
    return this.state.topic && this.state.project && this.state.options && [
      EL('style', {textContent: MDDstyle}), // Using styles defined above
      EL('div', {class: 'outer'}, [
        EL('label', {for: this.state.topic, textContent: this.state.name}),
        EL('select', {id: this.state.topic, onchange: this.onchange.bind(this)}, [
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
    // TODO-security at some point we'll need one client per project and to use username and password from config.yaml which by then should be in config.d
    // noinspection JSUnresolvedReference
    this.append(
      EL('mqtt-client', {slot: 'client', server: server_config.mqtt.broker}) // typically "ws://naturalinnovation.org:9012"
    )
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
      this.append(
        EL('mqtt-project', {topic: `${this.state.organization}/${this.state.project}/`, name: `${this.state.project}`}, [
          EL('mqtt-node', {topic: `${this.state.organization}/${this.state.project}/${this.state.node}`}),
        ]));
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
        this.append(EL('mqtt-project', {topic: `${this.state.organization}/${this.state.project}/`, name: `${this.state.project}`, discover: true}));
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
        this.renderAndReplace();
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
    this.state.nodes = [];
  }
  static get observedAttributes() { return MqttReceiver.observedAttributes.concat(['discover']); }
  static get boolAttributes() { return MqttReceiver.boolAttributes.concat(['discover'])}

  // noinspection JSCheckFunctionSignatures
  valueSet(val) {
    if (this.state.discover) {
      if (!this.state.nodes.includes(val)) {
        this.state.nodes.push(val);
        let id = val;
        let topic = this.state.topic + val;
        this.append(EL('mqtt-node', {id, topic, discover: this.state.discover},[]));
      }
    }
    return false; // Should not need to rerender
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
  static get observedAttributes() { return MqttReceiver.observedAttributes.concat(['id', 'discover']); }
  static get boolAttributes() { return MqttReceiver.boolAttributes.concat(['discover'])}
  constructor() {
    super(); // Will subscribe to topic
    this.state.topics = {};
  }

  elementFrom(t) {
    let topic = this.state.topic + "/" + t.topic;
    if (!this.state.topics[topic]) {
      let name = t.name;
      let el;
      if (t.display === "toggle") {
        // Assuming rw: rw, type: bool
        el = EL('mqtt-toggle', {topic, name, retain: true, qos: 1}, [name]);
      } else if (t.display === "bar") {
        // Assuming rw: r, type: float
        el = EL('mqtt-bar', {topic, name, max: t.max, min: t.min, color: t.color}, []);
      } else if (t.display === "text") {
        el = EL('mqtt-text', {name, topic}, []);
      } else if (t.display === "slider") {
        el = EL('mqtt-slider', {name, topic, min: t.min, max: t.max, value: (t.max + t.min) / 2}, [
          EL('span', {textContent: "△"}, []),
        ]);
      } else if (t.display === "dropdown") {
        el = EL('mqtt-dropdown', {
          name,
          topic,
          type: t.type,
          options: t.options,  // e.g. "bool" for must be boolean topic
          retain: true,
          qos: 1, // This message needs to get through to node
          project: this.project,
        }, []);
      } else {
        console.log("do not know how to display a ", t.display);
        //TODO add slider (MqttSlider), need to specify which other element attached to.
      }
      if (el) {
        this.append(el);
        this.state.topics[t.topic] = el; // Index into elements - e.g. used by MqttDropdown
      }
    } else {
      console.log("XXX Already have", topic);
    }
  }
  // noinspection JSCheckFunctionSignatures
  valueSet(val) {
    if (this.state.discover) { // If do not have "discover" set, then presume have defined what UI we want on this node
      this.state.discover = false; // Only want "discover" once, if change then need to get smart about not redrawing working UI as may be relying on data[]
      let obj = yaml.loadAll(val, {onWarning: (warn) => console.log('Yaml warning:', warn)});
      console.log(obj);
      let node = obj[0]; // Should only ever be one of them
      this.state.value = node; // Save the object for this node
      ['id', 'description', 'name'].forEach(k => this.state[k] = node[k]);
      while (this.childNodes.length > 0) this.childNodes[0].remove(); // Remove and replace
      node.topics.forEach(t => {
        this.elementFrom(t);
      });
      return true;
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

/* This could be part of MqttBar, but maybe better standalone */
class MqttGraph extends MqttElement {
  constructor() {
    super();
    this.datasets = []; // Child elements will add/remove here
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
    // TODO Move style to sheet
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
  render() {
    return ( [
      EL('link', {rel: 'stylesheet', href: '/frugaliot.css'}),
        // TODO see https://www.chartjs.org/docs/latest/configuration/responsive.html#important-note div should ONLY contain canvas
      EL("div", {class: 'outer'}, [ // TODO Move style to sheet
        EL('slot', {name: "chart"}), // TODO-46-line should just be the chart slot I think
        EL('slot', {}), // TODO-46-line should just be the chart slot I think
      ])
    );
  }
}
customElements.define('mqtt-graph', MqttGraph);

class MqttGraphDataset extends MqttElement {
  constructor() {
    super();
    // Do not make chartDataset here, as do not have attributes yet
  }
  static get observedAttributes() {
    return MqttReceiver.observedAttributes.concat(['name', 'color', 'min', 'max', 'yaxisid']); }
  static get integerAttributes() {
    return MqttReceiver.integerAttributes.concat(['min', 'max']) };
  /*
  shouldLoadWhenConnected() { return super.shouldLoadWhenConnected() ; }
   */
  makeChartDataset() {
    // Some other priorities that might be useful are at https://www.chartjs.org/docs/latest/samples/line/segments.html
    if (!this.chartdataset) {
      // Fields only defined once - especially data
      this.chartdataset = {
        data: this.state.data, // Should be pointer to receiver's data set in MqttReceiver.valueSet
        parsing: {
          xAxisKey: 'time',
          yAxisKey: 'value'
        },
      };
    }
    // Things that are changed by attributes
    this.chartdataset.label = this.state.name;
    this.chartdataset.borderColor = this.state.color; // also sets color of point
    this.chartdataset.yAxisID = this.state.yaxisid;
    // Should override display and position and grid of each axis used
  }
  changeAttribute(name, value) {
    super.changeAttribute(name, value); // Set on state
    this.makeChartDataset();
  }
  loadContent() { // Happens when connected
    this.chartEl = this.parentElement;
    this.chartEl.datasets.push(this.chartdataset);
    this.chartEl.makeChart();
  }
  dataChanged() { // Called when creating UX adds data.
    this.parentElement.chart.update();
  }
  render() {
    return EL('span', { textContent: this.state.name}); // TODO-46-line should be controls
  }
}
customElements.define('mqtt-graphdataset', MqttGraphDataset);