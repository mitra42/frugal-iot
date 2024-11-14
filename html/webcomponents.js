/*
 * Simple MQTT template for FrugalIoT
 */
import {EL, HTMLElementExtended, getUrl, toBool} from './node_modules/html-element-extended/htmlelementextended.js';
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
    return this._create(time).endOf(unit).valueOf();
  }
});
/* End of code copied from chartjs-adapter-luxon.esm.js */

var mqtt_client;
var mqtt_subscriptions = [];
let unique_id = 1; // Just used as a label for auto-generated elements

function mqtt_subscribe(topic, cb) {
  console.log("Subscribing to ", topic);
  mqtt_subscriptions.push({topic, cb});
  mqtt_client.subscribe(topic, (err) => { if (err) console.error(err); })
}
/* Helpers of various kinds */
function date_start_hour(x) {
  let y = x ? new Date(x) : new Date();
  y.setMinutes(0);
  y.setSeconds(0);
  y.setMilliseconds(0);
  return y;
}


/* MQTT support */
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
  constructor() { super(); }// Will subscribe to topic

  elementFrom(t) {
    let topic = this.state.topic + "/" + t.topic;
    let name = t.name;
    if (t.display === "toggle") {
      // Assuming rw: rw, type: bool
      this.append(EL('mqtt-toggle', {topic, name, retain: 1, qos: 1},[name]));
    } else if (t.display === "bar") {
      // Assuming rw: r, type: float
      this.append(EL('mqtt-bar', {topic, name, max: t.max, min: t.min, color: t.color},[]));
    } else if (t.display === "text") {
      this.append(EL('mqtt-text', {name, topic},[]));
    } else if (t.display === "slider") {
      this.append(EL('mqtt-slider', {name, topic, min: t.min, max: t.max, value: (t.max + t.min)/2 }, [
          EL('span', { textContent: "△"}, []),
      ]));
    } else {
      console.log("do not know how to display a ", t.display);
      //TODO add slider (MqttSlider), need to specify which other element attached to.
    }
  }
  valueSet(val) {
    this.state.value = val;
    let obj = yaml.loadAll(val,{ onWarning: (warn) => console.log('Yaml warning:', warn) });
    console.log(obj);
    let node = obj[0]; // Should only ever be one of them
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

/* This could be part of MqttBar, but maybe better standalone) */
class MqttGraph extends HTMLElementExtended {
  constructor() {
    super();
    this.datasets = []; // Child elements will add/remove here
  }

  // Note - makeChart is really fussy, the canvas must be inside something with a size.
  // For some reason this does not work by adding inside the render - i.e. to the virtual Dom.
  loadContent() {
    this.canvas = EL('canvas');
    this.append(EL('div', {style: "width: 80%;"},[this.canvas]));
    this.makeChart();
  }
  shouldLoadWhenConnected() {return true;}

  makeChart() {
    this.data = [
      {time: Date.now(), value: 50.0},
      {time: Date.now()+50*60*1000, value: 55.0},
      {time: Date.now()+60*60*1000, value: 53.3},
    ];
    this.datax = [
      {time: '2024-11-14T06:09:40.970Z', value: 50.0},
      {time: '2024-11-14T06:39:55.970Z', value: 55.0},
      {time: '2024-11-14T06:50:55.970Z', value: 53.3},
      {time: '2024-11-14T07:50:55.970Z', value: 48.3},
    ];
    //this.data = [40,50,60];
    // let suggestedMin = date_start_hour(this.data[0].time);
    // let suggestedMax = date_start_hour(this.data[this.data.length - 1].time);
    // console.log("from:", suggestedMin, suggestedMax);
      this.chart = new Chart(
      this.canvas,
      {
        type: 'line', // Really want it to be a line
        //labels: ['2024-11-14T06:09:40.970Z', '2024-11-14T06:39:55.970Z', '2024-11-14T06:50:55.970Z'],
        data: {
          //labels: data.map(row => row.year),
          //labels: [10,20,30], // TODO-46-line these need to come from data, but only if integral (e.g. 15 mins)
          //labels: this.datasets[0].map(row => row.time), // TODO-46-line placeholder, will want actual date and not every one
          datasets: [{ // Format needed for Chart TODO-46 check its the same for bar vs graph vs xy etc
              label: "XXX", // TODO-46-line Probably have to get this from setAttribute
              data: this.data,
              parsing: {
                  xAxisKey: 'time',
                  yAxisKey: 'value'
              },
            }],
          // this.datasets, // TODO-46 should be live
        },
        options: {
          //zone: "America/Denver", // Comment out to use system time
          scales: { // For some reason cant put this on a dataset
            xAxis: {
              // display: false,
              type: 'time',
              distribution: 'series',
              adapters: {
                date: {
                  // locale: 'en-US', // Comment out to Use systems Locale
                }
              },

              /* // Doesnt seem to work
              ticks: {
                suggestedMin: suggestedMin,
                suggestedMax: suggestedMax,
                //display: false //this will remove only the label
              },
              */
              /*  // Does stuff, but not sure useful
              time: {
                unit: 'hour',
                // displayFormats: { day: 'MMM DD, YYYY' } // or any desired format               }
              },
              */
              /*
              type: 'linear',
              ticks: {
                suggestedMin: date_start_hour(this.data[0].time).getMilliseconds(),
                suggestedMax: date_start_hour(this.data[this.data.length - 1].time+60*60*1000).getMilliseconds(),
                //display: false //this will remove only the label
              },
              */
            },
          }
        }
      }
    );
  }
  render() {
    return (
      EL("div", {style: "width: 800px; height: 100px;"}, [ // TODO Move style to sheet
        EL('slot', {}), // TODO-46-line should just be the chart slot I think
      ])
    );
  }
}
customElements.define('mqtt-graph', MqttGraph);

//TODO-46-multiline X axis wont work if different from each data set, may need interpolation ?
class MqttGraphDataset extends MqttReceiver {
  constructor() {
    super();
    /*
    this.data = [];
    this.chartdataset = { // Format needed for Chart TODO-46 check its the same for bar vs graph vs xy etc
      label: this.state.name, // TODO-46-line Probably have to get this from setAttribute
      data: this.data,
      options: { // TODO-46 this is where we want to specify keys
        parsing: {
          xAxisKey: 'time',
          yAxisKey: 'value'
        }
      },
    };
    this.chartEl = this.parentElement;
    this.chartEl.datasets.push(this.chartdataset);
    // TODO-46 this is sample data to separate out for testing
    this.data.push([
      {time: Date.now(), value: 50.0},
      {time: Date.now()+30000, value: 55.0},
      {time: Date.now()+60000, value: 53.3},
    ]);
    this.parentElement.chart.update();
    */
  }
  valueSet(val) {
    this.data.push({time: Date.now(), value: val});
    // TODO-46 need to add to both X labels and to data in dataset, not just this.data
    this.parent.chart.update();
  }
  render() {
    return EL('span', { textContent: this.state.name}); // TODO-46-line should be controls
  }
}
customElements.define('mqtt-graphdataset', MqttGraphDataset);