// CGAL's answers to the Spare Kit geometry kernel's mesh booleans, on Nef polyhedra over exact
// integers. This program is under the GPL, the licence of the CGAL packages it uses; it is run,
// never linked.
//
// One JSON job per line on standard input, as `oracle jobs cgal` writes them; one JSON answer per
// line on standard output, the regularised result's volume as an exact fraction in lowest terms.
// The volume is read straight off the result's boundary, so a body that is not a manifold (two
// pieces meeting along an edge or at a corner) is measured like any other.
//
// A solid is read shell by shell: its triangles fall into shells by the edges they share, each
// shell is its own polyhedral surface, and the shell's signed volume says which way it faces. A
// Nef polyhedron made from a polyhedral surface is the point set that surface encloses, whichever
// way it faces (CGAL's manual, Nef_3), so one made from every shell at once fills a cavity. So the
// shells are taken from the largest enclosed volume down: one facing out is added, one facing in
// (a cavity) is taken away. A shell lies inside only shells that enclose more, so each is taken
// after every shell around it, and an island inside a cavity is added back. That is the solid the
// kernel reads, where a point is inside when its winding number is above zero, for every solid
// whose shells do not cross; `tests/` holds the smallest such inputs with their exact volumes, and
// the image's build runs them.

#include <algorithm>
#include <array>
#include <cstddef>
#include <exception>
#include <iostream>
#include <map>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

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
using Triangle = std::array<std::size_t, 3>;
using Corner = std::array<long, 3>;

// One shell as a polyhedral surface: its own corners, and its triangles by them.
class Shell : public CGAL::Modifier_base<Halfedges> {
 public:
  Shell(const std::vector<Corner>& corners, const std::vector<Triangle>& triangles)
      : corners_(corners), triangles_(triangles) {}

  void operator()(Halfedges& halfedges) override {
    CGAL::Polyhedron_incremental_builder_3<Halfedges> build(halfedges, true);
    build.begin_surface(corners_.size(), triangles_.size());
    for (const auto& c : corners_) build.add_vertex(Point(c[0], c[1], c[2]));
    for (const auto& t : triangles_) {
      build.begin_facet();
      for (const auto v : t) build.add_vertex_to_facet(v);
      build.end_facet();
    }
    build.end_surface();
    if (build.error()) throw std::runtime_error("a shell that is not a polyhedral surface");
  }

 private:
  const std::vector<Corner>& corners_;
  const std::vector<Triangle>& triangles_;
};

// The root of `i` in `parent`, each step on the way pointed at its grandparent.
static std::size_t root(std::vector<std::size_t>& parent, std::size_t i) {
  while (parent[i] != i) {
    parent[i] = parent[parent[i]];
    i = parent[i];
  }
  return i;
}

// Six times the signed volume a shell encloses: the divergence theorem over its triangles.
static Integer six_volume(const std::vector<Corner>& corners,
                          const std::vector<Triangle>& triangles) {
  Integer six(0);
  for (const auto& t : triangles) {
    const Corner& a = corners.at(t[0]);
    const Corner& b = corners.at(t[1]);
    const Corner& c = corners.at(t[2]);
    const Integer ax(a[0]), ay(a[1]), az(a[2]);
    const Integer bx(b[0]), by(b[1]), bz(b[2]);
    const Integer cx(c[0]), cy(c[1]), cz(c[2]);
    six += ax * (by * cz - bz * cy) - ay * (bx * cz - bz * cx) + az * (bx * cy - by * cx);
  }
  return six;
}

static Nef nef(const json& s) {
  std::vector<Corner> corners;
  for (const auto& c : s.at("vertices")) {
    corners.push_back({c.at(0).get<long>(), c.at(1).get<long>(), c.at(2).get<long>()});
  }
  std::vector<Triangle> triangles;
  for (const auto& t : s.at("triangles")) {
    triangles.push_back({t.at(0).get<std::size_t>(), t.at(1).get<std::size_t>(),
                         t.at(2).get<std::size_t>()});
  }
  // The shells: triangles joined by each edge they share.
  std::vector<std::size_t> parent(triangles.size());
  std::iota(parent.begin(), parent.end(), 0);
  std::map<std::pair<std::size_t, std::size_t>, std::size_t> first_on;
  for (std::size_t i = 0; i < triangles.size(); ++i) {
    for (std::size_t k = 0; k < 3; ++k) {
      const std::size_t p = triangles[i][k];
      const std::size_t q = triangles[i][(k + 1) % 3];
      const auto edge = std::make_pair(std::min(p, q), std::max(p, q));
      const auto found = first_on.emplace(edge, i);
      if (!found.second) parent[root(parent, i)] = root(parent, found.first->second);
    }
  }
  std::map<std::size_t, std::vector<std::size_t>> members;
  for (std::size_t i = 0; i < triangles.size(); ++i) members[root(parent, i)].push_back(i);
  // Each shell on its own corners, with its signed volume and its Nef polyhedron.
  struct Part {
    Integer six;
    Nef solid;
  };
  std::vector<Part> parts;
  for (const auto& member : members) {
    std::map<std::size_t, std::size_t> renumbered;
    std::vector<Corner> own_corners;
    std::vector<Triangle> own_triangles;
    for (const std::size_t i : member.second) {
      Triangle own;
      for (std::size_t k = 0; k < 3; ++k) {
        const std::size_t v = triangles[i][k];
        const auto placed = renumbered.emplace(v, own_corners.size());
        if (placed.second) own_corners.push_back(corners.at(v));
        own[k] = placed.first->second;
      }
      own_triangles.push_back(own);
    }
    const Integer six = six_volume(own_corners, own_triangles);
    if (six == Integer(0)) throw std::runtime_error("a shell that encloses no volume");
    Polyhedron surface;
    Shell shell(own_corners, own_triangles);
    surface.delegate(shell);
    parts.push_back({six, Nef(surface)});
  }
  // From the largest enclosed volume down: out adds, in takes away.
  std::stable_sort(parts.begin(), parts.end(), [](const Part& x, const Part& y) {
    return CGAL::abs(x.six) > CGAL::abs(y.six);
  });
  Nef made(Nef::EMPTY);
  for (const auto& part : parts) {
    if (part.six > Integer(0)) {
      made = made + part.solid;
    } else {
      made = made - part.solid;
    }
  }
  return made;
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
