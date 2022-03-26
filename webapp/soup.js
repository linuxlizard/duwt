// https://codesandbox.io/s/festive-boyd-nrmwh?file=/src/index.js
// 
// graph a bubble plot of scan survey results
//
// davep@mbuf.com

// TODO run standalone. Saving a copy here in case I lose codesandbox.io

//import "./styles.css";
//import * as d3 from "d3";

async function drawStuff() {
  let source_url = new URL(document.URL);
  let target_url = source_url.origin + "/api/survey";
  var soup = await d3.json(target_url);

  // only 2.4 GHz bands
  //soup = d3.filter(soup, (d) => d.freq < 4000);
  // only 5Ghz bands
  soup = d3.filter(soup, (d) => d.freq > 4000);

  const xAccessor = (d) => d.freq;
  const yAccessor = (d) => d.dbm;
  const widthAccessor = (d) => d.chwidth;
  const radiusmap = { "20": 2, "20/40": 4, "20/40/80": 8 };
  //console.log(soup[0]);
  //console.log(radiusmap[widthAccessor(soup[0])]);

  // 2. Create chart dimensions

  const width = d3.min([window.innerWidth * 0.9, window.innerHeight * 0.9]);
  let dimensions = {
    width: width,
    height: width,
    margin: {
      top: 10,
      right: 10,
      bottom: 50,
      left: 50
    }
  };
  dimensions.boundedWidth =
    dimensions.width - dimensions.margin.left - dimensions.margin.right;
  dimensions.boundedHeight =
    dimensions.height - dimensions.margin.top - dimensions.margin.bottom;

  // 3. Draw canvas

  const wrapper = d3
    .select("#app")
    .append("svg")
    .attr("width", dimensions.width)
    .attr("height", dimensions.height);

  const bounds = wrapper
    .append("g")
    .style(
      "transform",
      `translate(${dimensions.margin.left}px, ${dimensions.margin.top}px)`
    );

  // 4. Create scales

  const xScale = d3
    .scaleLinear()
    .domain(d3.extent(soup, xAccessor))
    .range([0, dimensions.boundedWidth])
    .nice();

  const yScale = d3
    .scaleLinear()
    .domain(d3.extent(soup, yAccessor))
    .range([0, dimensions.boundedHeight])
    .nice();

  // https://observablehq.com/d/5ae88dea8969869a
  // create clip path
  bounds
    .append("clipPath")
    .attr("id", "border")
    .append("rect")
    .attr("width", dimensions.boundedWidth)
    .attr("height", dimensions.boundedHeight);

  // draw grid
  const grid = bounds.append("g");

  let yLines = grid.append("g").selectAll("line");
  let xLines = grid.append("g").selectAll("line");

  function drawGridLines(x, y) {
    yLines = yLines
      .data(y.ticks())
      .join("line")
      .attr("stroke", "#d3d3d3")
      .attr("x1", 0)
      .attr("x2", dimensions.boundedWidth)
      .attr("y1", (d) => 0.5 + y(d))
      .attr("y2", (d) => 0.5 + y(d));

    xLines = xLines
      .data(x.ticks())
      .join("line")
      .attr("stroke", "#d3d3d3")
      .attr("x1", (d) => 0.5 + x(d))
      .attr("x2", (d) => 0.5 + x(d))
      .attr("y1", (d) => 0)
      .attr("y2", (d) => dimensions.boundedHeight);
  }

  drawGridLines(xScale, yScale);

  // 5. Draw data
  const dotsGroup = bounds.append("g").attr("clip-path", "url(#border)");

  const dots = dotsGroup
    .selectAll("circle")
    .data(soup)
    .enter()
    .append("circle")
    .attr("cx", (d) => xScale(xAccessor(d)))
    .attr("cy", (d) => yScale(yAccessor(d)))
    .attr("r", (d) => radiusmap[widthAccessor(d)])
    .attr("fill", "#444")
    .attr("cursor", "crosshair")
    .on("mouseenter", onMouseEnter)
    .on("mouseleave", onMouseLeave);

  //drawGridLines(xScale, yScale);

  function onMouseEnter(evt, datum) {
    //console.log("pt=", d3.pointer(evt));
    //console.log("target=", evt.currentTarget);
    console.log("d=", datum);
    const node = d3.select(this);
    console.log(node.attr("cx"), node.attr("cy"));
    bounds
      .append("line")
      .attr("x1", 0)
      .attr("y1", 0)
      .attr("x2", d3.select(this).attr("cx"))
      .attr("y2", d3.select(this).attr("cy"))
      .attr("stroke", "black")
      .attr("id", "linez");
  }

  function onMouseLeave(evt, datum) {
    d3.select("#linez").remove();
  }

  // A better zoom
  // https://observablehq.com/d/5ae88dea8969869a

  // 6 draw axes
  const xAxisGenerator = d3.axisBottom().scale(xScale);

  const xAxis = bounds
    .append("g")
    .call(xAxisGenerator)
    .style("transform", `translateY(${dimensions.boundedHeight}px)`)
    .call((g) => g.selectAll(".domain"));

  const yAxisGenerator = d3.axisLeft().scale(yScale).ticks(6);

  const yAxis = bounds
    .append("g")
    .call(yAxisGenerator)
    .call((g) => g.selectAll(".domain"));

  // https://observablehq.com/@d3/zoom-svg-rescaled?collection=@d3/d3-zoom

  wrapper.call(
    d3
      .zoom()
      .extent([
        [0, 0],
        [dimensions.boundedWidth, dimensions.boundedHeight]
      ])
      .scaleExtent([1, 8])
      .on("zoom", zoomed)
  );

  const circle = bounds.selectAll("circle");
  console.log("circle=", circle);

  function zoomed({ transform }) {
    console.log("zoomed transform=", transform);

    const xNew = transform.rescaleX(xScale);
    const yNew = transform.rescaleY(yScale);
    drawGridLines(xNew, yNew);

    dots
      .attr("cx", (d) => xNew(xAccessor(d)))
      .attr("cy", (d) => yNew(yAccessor(d)))
      .attr("r", (d) => radiusmap[widthAccessor(d)] * transform.k);

    // update the axes
    xAxis.call(xAxisGenerator.scale(xNew)).call((g) => g.selectAll(".domain"));

    yAxis.call(yAxisGenerator.scale(yNew)).call((g) => g.selectAll(".domain"));

    // var foo = {
    //   k: 1.109569472067845,
    //   x: -17.859823947058743,
    //   y: -67.39983101795247
    // };
    //console.log("foo=", transform.apply([foo.x, foo.y]));
    //console.log("dots=", dots);
    //console.log("this=", this);
    //console.log("select=", d3.select(this));
    //console.log(circle.attr("cx"));
    //dots.each((d) => console.log("circle=", d3.select(this)));

    // https://github.com/d3/d3-zoom/tree/v2.0.0
    // circle.attr(
    //   "transform",
    //   "translate(" +
    //     transform.x +
    //     "," +
    //     transform.y +
    //     ") scale(" +
    //     transform.k +
    //     ")"
    //);

    //dots.attr("transform", (d) => `translate(${transform.apply(d)})`);
  }
}

drawStuff();

console.log(d3.select("#app"));
//d3.select("#app").html("<h1>Welcome to the Soup</h1>");

