// CGAL's answers to the Spare Kit geometry kernel's mesh booleans, on Nef polyhedra over exact
// integers. This program is under the GPL, the licence of the CGAL packages it uses; it is run,
// never linked.
//
// One JSON job per line on standard input, as `oracle jobs cgal` writes them; one JSON answer per
// line on standard output, the regularised result's volume as an exact fraction in lowest terms.
// The volume is read straight off the result's boundary, so a body that is not a manifold (two
// pieces meeting along an edge or at a corner) is measured like any other.

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
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using Integer = CGAL::Exact_integer;
using Kernel = CGAL::Homogeneous<Integer>;
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

// The volume by the divergence theorem over the boundary: every halffacet that faces the inside
// (its own volume marked, its twin's not), each of its cycles fanned from its first corner. A
// halffacet's cycles all run one way about its plane, holes against the outline, and every such
// halffacet faces the inside, so the sum is six times the volume, of one sign throughout.
static std::string volume_of(const Nef& made) {
  FT six(0);
  for (auto facet = made.halffacets_begin(); facet != made.halffacets_end(); ++facet) {
    if (!facet->incident_volume()->mark() || facet->twin()->incident_volume()->mark()) continue;
    for (auto cycle = facet->facet_cycles_begin(); cycle != facet->facet_cycles_end(); ++cycle) {
      // A cycle that is a lone loop has no corners, and bounds no area.
      if (!cycle.is_shalfedge()) continue;
      Nef::SHalfedge_const_handle start(cycle);
      Nef::SHalfedge_around_facet_const_circulator edge(start), end(edge);
      const Point first = edge->source()->center_vertex()->point();
      ++edge;
      auto next = edge;
      ++next;
      for (; next != end; ++edge, ++next) {
        six += determinant(first, edge->source()->center_vertex()->point(),
                           next->source()->center_vertex()->point());
      }
    }
  }
  // In lowest terms, so the fraction stays as short as the volume.
  const Integer numerator = six.numerator();
  const Integer denominator = six.denominator();
  const Integer six_times = denominator * Integer(6);
  const Integer top = CGAL::abs(numerator);
  const Integer bottom = CGAL::abs(six_times);
  const Integer common = CGAL::gcd(top, bottom);
  const Integer reduced_top = top / common;
  const Integer reduced_bottom = bottom / common;
  std::ostringstream written;
  written << reduced_top << '/' << reduced_bottom;
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
