// CGAL's answers to the Spare Kit geometry kernel's mesh booleans, on Nef polyhedra over exact
// integers. This program is under the GPL, the licence of the CGAL packages it uses; it is run,
// never linked.
//
// One JSON job per line on standard input, as `oracle jobs cgal` writes them; one JSON answer per
// line on standard output, the regularised result's volume as an exact fraction. The result is cut
// into convex pieces, so a body that is not a manifold is measured like any other.

#include <cstddef>
#include <exception>
#include <iostream>
#include <sstream>
#include <string>

#include <CGAL/Exact_integer.h>
#include <CGAL/Homogeneous.h>
#include <CGAL/Nef_polyhedron_3.h>
#include <CGAL/Polyhedron_3.h>
#include <CGAL/Polyhedron_incremental_builder_3.h>
#include <CGAL/convex_decomposition_3.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using Kernel = CGAL::Homogeneous<CGAL::Exact_integer>;
using Polyhedron = CGAL::Polyhedron_3<Kernel>;
using Halfedges = Polyhedron::HalfedgeDS;
using Nef = CGAL::Nef_polyhedron_3<Kernel>;
using Point = Kernel::Point_3;
using FT = Kernel::FT;

class Solid : public CGAL::Modifier_base<Halfedges> {
 public:
  explicit Solid(const json& s) : s_(s) {}

  void operator()(Halfedges& halfedges) override {
    CGAL::Polyhedron_incremental_builder_3<Halfedges> build(halfedges, true);
    const auto& corners = s_.at("vertices");
    const auto& triangles = s_.at("triangles");
    build.begin_surface(corners.size(), triangles.size());
    for (const auto& c : corners) {
      build.add_vertex(Point(c.at(0).get<long>(), c.at(1).get<long>(), c.at(2).get<long>()));
    }
    for (const auto& t : triangles) {
      build.begin_facet();
      for (const auto& v : t) build.add_vertex_to_facet(v.get<std::size_t>());
      build.end_facet();
    }
    build.end_surface();
  }

 private:
  const json& s_;
};

static Nef nef(const json& s) {
  Polyhedron shell;
  Solid solid(s);
  shell.delegate(solid);
  return Nef(shell);
}

static FT determinant(const Point& a, const Point& b, const Point& c) {
  return a.x() * (b.y() * c.z() - b.z() * c.y()) - a.y() * (b.x() * c.z() - b.z() * c.x()) +
         a.z() * (b.x() * c.y() - b.y() * c.x());
}

// Six times the volume of one closed convex piece.
static FT six_volume(const Polyhedron& piece) {
  FT six(0);
  for (auto facet = piece.facets_begin(); facet != piece.facets_end(); ++facet) {
    auto edge = facet->facet_begin();
    const Point first = edge->vertex()->point();
    ++edge;
    auto next = edge;
    ++next;
    for (; next != facet->facet_begin(); ++edge, ++next) {
      six += determinant(first, edge->vertex()->point(), next->vertex()->point());
    }
  }
  return CGAL::abs(six);
}

static std::string volume_of(Nef made) {
  CGAL::convex_decomposition_3(made);
  FT six(0);
  auto volume = made.volumes_begin();
  // The first volume is the outside.
  for (++volume; volume != made.volumes_end(); ++volume) {
    if (!volume->mark()) continue;
    Polyhedron piece;
    made.convert_inner_shell_to_polyhedron(volume->shells_begin(), piece);
    six += six_volume(piece);
  }
  const FT whole = six / FT(6);
  std::ostringstream written;
  written << CGAL::abs(whole.numerator()) << '/' << CGAL::abs(whole.denominator());
  return written.str();
}

static Nef result(const json& job) {
  const Nef a = nef(job.at("a"));
  const Nef b = nef(job.at("b"));
  const std::string op = job.at("op").get<std::string>();
  if (op == "union") return (a + b).regularization();
  if (op == "intersection") return (a * b).regularization();
  return (a - b).regularization();
}

int main() {
  std::string line;
  while (std::getline(std::cin, line)) {
    const json job = json::parse(line);
    json answer = {{"id", job.at("id")}, {"volume", nullptr}};
    try {
      answer["volume"] = volume_of(result(job));
    } catch (const std::exception&) {
      // Refused: the answer stays null, and the comparison sees a refusal.
    }
    std::cout << answer.dump() << '\n';
  }
  return 0;
}
