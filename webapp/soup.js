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
  let soup = await d3.json(target_url);

  let table = d3.select("detailtable");
  console.log("table=",table);

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

  const width = d3.min([window.innerWidth * 0.5, window.innerHeight * 0.5]);
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

    // https://observablehq.com/@d3/line-with-tooltip
    const tooltip = bounds.append("g")
        .style("pointer-events", "none");

    const dots = dotsGroup
        .selectAll("circle")
        .data(soup)
        .enter()
        .append("circle")
        .attr("cx", (d) => xScale(xAccessor(d)))
        .attr("cy", (d) => yScale(yAccessor(d)))
        .attr("r", (d) => radiusmap[widthAccessor(d)])
        .attr("fill", "#444")
        .attr("fill-opacity", "0.4")
        .attr("cursor", "crosshair")
        .on("mouseenter", onMouseEnter)
        .on("mouseleave", onMouseLeave);

    function onMouseEnter(evt, datum) {
        //console.log("pt=", d3.pointer(evt));
        //console.log("target=", evt.currentTarget);
        console.log("d=", datum);

        const node = d3.select(this);
        console.log(node.attr("cx"), node.attr("cy"));

        // draw a line from 0,0 to our pointered circle
        bounds
          .append("line")
          .attr("x1", 0)
          .attr("y1", 0)
          .attr("x2", d3.select(this).attr("cx"))
          .attr("y2", d3.select(this).attr("cy"))
          .attr("stroke", "black")
          .attr("id", "linez");

        const X = d3.select(this).attr("cx");
        const Y = d3.select(this).attr("cy");
        console.log(`X=${X} Y=${Y}`);

        // https://observablehq.com/@d3/line-with-tooltip
//        tooltip.style("display", null);
//        tooltip.attr("transform", `translate(${X},${Y})`);
//        tooltip.attr("transform", `translate(${xScale(X[i])},${yScale(Y[i])})`);

//        const path = tooltip.selectAll("path")
//          .data([,])
//          .join("path")
//            .attr("fill", "white")
//            .attr("stroke", "black");
            
          const rect = tooltip.selectAll("rect")
              .join("rect")
                .attr("x", 100)
                .attr("y", 100)
                .attr("width", "20")
                .attr("height", "20")
                .attr("fill", "white")
                .attr("stroke", "black")
                ;

//<rect x="50" y="10" width="20" height="40"
//    style="fill: none; stroke: black;"/>

    }

  function onMouseLeave(evt, datum) {
    d3.select("#linez").remove();
    tooltip.style("display", "none");
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

  // https://observablehq.com/@d4/zoom-svg-rescaled?collection=@d3/d3-zoom

  let zoom = 
    d3
      .zoom()
      .extent([
        [0, 0],
        [dimensions.boundedWidth, dimensions.boundedHeight]
      ])
      .scaleExtent([1, 8])
      .on("zoom", zoomed)
      ;
  wrapper.call(zoom);

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

  const resetbutton = d3.select("#resetbutton");
  console.log("resetbutton=", resetbutton);
  resetbutton.on("click", function() { 
          console.log("click!"); 
//          zoomed({transform:{k:1, x:1, y:1}});
          wrapper.transition().duration(750).call(zoom.transform, d3.zoomIdentity);
         });

}

drawStuff();

console.log(d3.select("#app"));
//d3.select("#app").html("<h1>Welcome to the Soup</h1>");

