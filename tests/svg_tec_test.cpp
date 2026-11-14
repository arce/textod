#include "svg_tec.hpp"
#include <iostream>
#include <clocale>

static int fails = 0;
#define CHECK(cond) do { if (!(cond)) { ++fails; std::cout << "FAIL line " << __LINE__ << ": " #cond "\n"; } } while (0)
#define EQ(a, b) do { auto x = (a); auto y = (b); if (x != y) { ++fails; std::cout << "FAIL line " << __LINE__ << ": got [" << x << "] expected [" << y << "]\n"; } } while (0)
#define THROWS(expr) do { bool t = false; try { expr; } catch (const std::exception&) { t = true; } if (!t) { ++fails; std::cout << "FAIL line " << __LINE__ << ": no exception\n"; } } while (0)
static bool has(const std::string& s, const std::string& sub) { return s.find(sub) != std::string::npos; }

int main() {
  std::setlocale(LC_ALL, "de_DE.UTF-8");
  std::locale::global(std::locale::classic());

  EQ(SVG::num(0), "0");
  EQ(SVG::num(-0.0), "0");
  EQ(SVG::num(-0.001), "0");
  EQ(SVG::num(1), "1");
  EQ(SVG::num(1.5), "1.5");
  EQ(SVG::num(0.125), "0.12");
  EQ(SVG::num(0.375), "0.38");
  EQ(SVG::num(2.675), "2.67");
  EQ(SVG::num(12345.678), "12345.68");
  EQ(SVG::num(-3.14159), "-3.14");
  EQ(SVG::num(100), "100");
  EQ(SVG::num(1e21), "1000000000000000000000");
  THROWS(SVG::num(std::nan("")));
  THROWS(SVG::num(INFINITY));

  EQ(SVG::esc("A & B < C > \"D\" 'E'"), "A &amp; B &lt; C &gt; &quot;D&quot; &apos;E&apos;");
  EQ(SVG::esc(""), "");

  EQ(SVG::Color(300, -5, 16).toHex(), "#ff0010");
  EQ(SVG::Color("#2c81ba").toHex(), "#2c81ba");
  THROWS(SVG::Color("#12"));

  SVG::Svg svg(200, 100);
  svg.viewBox(0, 0, 200, 100).title("A & B").style("text { fill: red }");
  svg.text(10, 20, "x < 5 & y").fill("#333333").font("Arial", 14).anchor("middle").bold().italic().cls("lbl").id("t1");
  svg.rect(1, 2, 3, 4, 5).fill("#ff0000").stroke(2).cap(SVG::Svg::ROUND).opacity(0.5).attr("data-k", "a\"b");
  std::string out = svg.toString();
  CHECK(has(out, "viewBox=\"0 0 200 100\""));
  CHECK(has(out, "<title>A &amp; B</title>"));
  CHECK(has(out, "x &lt; 5 &amp; y</text>"));
  CHECK(has(out, "text-anchor=\"middle\""));
  CHECK(has(out, "font-weight=\"bold\""));
  CHECK(has(out, "id=\"t1\" class=\"lbl\""));
  CHECK(has(out, "data-k=\"a&quot;b\""));
  CHECK(has(out, "opacity=\"0.5\""));

  CHECK(has(svg.rect(0, 0, 1, 1).cap("round").stroke(3, "4 2").toSVG(), "stroke-width=\"3\" stroke-dasharray=\"4 2\" stroke-linecap=\"round\""));

  auto& p = svg.path().moveTo(0, 0).lineTo(10.126, 5).arcTo(5, 5, 0, true, false, 20, 0).quadTo(1, 2, 3, 4).curveTo(1, 2, 3, 4, 5, 6).close().stroke(1.5).noFill();
  EQ(p.toSVG(), "<path d=\"M 0 0 L 10.13 5 A 5 5 0 1 0 20 0 Q 1 2 3 4 C 1 2 3 4 5 6 Z\" fill=\"none\" stroke=\"#000000\" stroke-width=\"1.5\"/>");

  auto& g = svg.group().translate(5, 6).rotate(45, 10, 10).stroke(1);
  g.circle(1, 1, 1);
  g.polygon({{0, 0}, {1, 0}, {1, 1}}).fill("#00ff00");
  EQ(g.toSVG().substr(0, 40), std::string("<g stroke=\"#000000\" stroke-width=\"1\" tra"));
  CHECK(has(g.toSVG(), "transform=\"translate(5,6) rotate(45,10,10)\""));

  svg.wedge(50, 50, 0, 20, 0, 90);
  svg.quadCurve(0, 0, 5, 5, 10, 0);
  svg.cubicCurve(0, 0, 1, 1, 2, 2, 3, 3);
  svg.raw("<!-- raw -->");

  THROWS(svg.save("/nonexistent_dir/x.svg"));
  svg.save("/tmp/svg_tec_test.svg");

  std::cout << (fails ? "FAILED\n" : "all tests passed\n");
  return fails ? 1 : 0;
}
